/*
 * Copyright © 2018 C. Ansel Horn <ansel@horn.name>
 * Copyright © 2026 Thilo Fromm <kontakt@thilo-fromm.de>
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

static const struct TEMPerdev_types {
    unsigned short vend;
    unsigned short prod;
    const char * desc;
} devtypes[] = {
    { 0x3553, 0xa001, "PCSensor TEMPer Gold (2026)" },
    { 0x413d, 0x2107, "PCSensor TEMPer Gold v3.1" }
};

/*
 * Queries the TEMPer device for the current temperature, in degrees Celsius,
 * and returns it.
 */
void query_temp(hid_device* dev)
{
    // Send the temperature query to the device
    unsigned char query[QUERY_LEN] = QUERY;
    if ( hid_write( dev, query, QUERY_LEN ) < QUERY_LEN )
    {
        fprintf( stderr, "Error: could not write to TEMPer device\n" );
        return;
    }
    // Receive the raw temperature data response
    unsigned char data[DATA_LEN];
    if ( hid_read_timeout( dev, data, DATA_LEN, 1000 ) < DATA_LEN )
    {
        fprintf( stderr, "Error: could not read from TEMPer device\n" );
        return;
    }
    // Parse the raw response into a single temperature value
    float temp = (float) (data[2] << 8 | data[3]) / 100.0;
    printf( "%.2f", temp );
}


hid_device * open_device(struct hid_device_info * info)
{
    hid_device * d = hid_open_path( info->path );
    if ( ! d )
    {
        fprintf( stderr, " Error opening device %s:", info->path);
        fwprintf(stderr, hid_error( d ) );
        fprintf( stderr, "\n");
        return NULL;
    } 

    return d;
}

/*
 * Initializes the TEMPer device using its vendor and product ID.
 */
void query_temps_devtype(const struct TEMPerdev_types * type)
{
    // Attempt to open and return TEMPer device
    struct hid_device_info *infos = hid_enumerate( type->vend, type->prod );
    if ( infos )
    {
        struct hid_device_info *info;
        for ( info = infos; info; info = info->next )
        {
            if ( info->interface_number == 1 )
            {
                hid_device *dev;
                if ( NULL == (dev = open_device(info)) )
                    continue;

                printf( "%s: ", info->path);
                query_temp(dev);
                printf( " (%s)\n", type->desc);
            }
        }
        hid_free_enumeration( infos );
    }
}

void query_all_temps()
{
    unsigned int i;
    for ( i=0; i<( sizeof(devtypes)/sizeof(devtypes[0]) ); i++)
    {
        query_temps_devtype(&devtypes[i]);
    }
}

int main(int argc, char** argv)
{
    // Initialize HIDAPI library
    if ( hid_init() != 0 )
    {
        fprintf( stderr, "Error: could not initialize HIDAPI\n" );
        return 1;
    }

    query_all_temps();
    return 0;
}
