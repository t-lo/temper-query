/*
 * Copyright © 2018 C. Ansel Horn <ansel@horn.name>
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See the
 * COPYING file for more details.
 */

#include <stdio.h>

#include <hidapi/hidapi.h>

#define QUERY     {0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00}
#define QUERY_LEN 8
#define DATA_LEN  8

static const struct TEMPerdevs {
    unsigned short vend;
    unsigned short prod;
    const char * desc;
} devs[] = 
{
    { 0x3553, 0xa001, "PCSensor TEMPer Gold (2026)" },
    { 0x413d, 0x2107, "PCSensor TEMPer Gold v3.1" }
};


/*
 * Initializes the TEMPer device using its vendor and product ID.
 */
hid_device* init_device(unsigned short vend, unsigned short prod)
{
    // Attempt to open and return TEMPer device
    hid_device *dev;
    struct hid_device_info *infos = hid_enumerate( vend, prod );
    if ( infos )
    {
        struct hid_device_info *info;
        for ( info = infos; info; info = info->next )
        {
            if ( info->interface_number == 1 )
            {
                dev = hid_open_path( info->path );
            }
        }
        hid_free_enumeration( infos );
    }
    if ( ! dev )
    {
        fwprintf( stderr, hid_error( dev ) );
        fprintf( stderr, "  No accessible device found.\n" );
    }
    return dev;
}

/*
 * Iterate through supported TEMPer devices. Return first match or NULL.
 */
hid_device * find_device()
{
    int i;
    for ( i=0; i<( sizeof(devs)/sizeof(devs[0]) ); i++)
    {
        fprintf( stderr, "Probing %s: ", devs[i].desc );
        hid_device* ret = init_device(devs[i].vend, devs[i].prod);
        if (ret)
        {
            fprintf( stderr, "Success!\n" );
            return ret;
        }
    }
    fprintf( stderr, "No supported device found.\n" );
    return NULL;
}

/*
 * Queries the TEMPer device for the current temperature, in degrees Celsius,
 * and returns it.
 */
float query_temp(hid_device* dev)
{
    // Send the temperature query to the device
    unsigned char query[QUERY_LEN] = QUERY;
    if ( hid_write( dev, query, QUERY_LEN ) < QUERY_LEN )
    {
        fprintf( stderr, "Error: could not write to TEMPer device\n" );
        return 0.0;
    }
    // Receive the raw temperature data response
    unsigned char data[DATA_LEN];
    if ( hid_read_timeout( dev, data, DATA_LEN, 1000 ) < DATA_LEN )
    {
        fprintf( stderr, "Error: could not read from TEMPer device\n" );
        return 0.0;
    }
    // Parse the raw response into a single temperature value
    float temp = (float) (data[2] << 8 | data[3]) / 100.0;
    return temp;
}


int main(int argc, char** argv)
{
    // Initialize HIDAPI library
    if ( hid_init() != 0 )
    {
        fprintf( stderr, "Error: could not initialize HIDAPI\n" );
        return 1;
    }
    // Initialize the TEMPer device
    hid_device *dev = find_device();
    if ( !dev )
    {
        return 2;
    }
    // Query the TEMPer device for the current temperature and print the result
    float temp = query_temp( dev );
    printf( "%.2f\n", temp );
    return 0;
}
