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
#include <stdlib.h>
#include <string.h>

#include <hidapi/hidapi.h>

#define FWID_QUERY     {0x01, 0x86, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00}
#define DATA_QUERY     {0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00}
#define QUERY_LEN 8
#define DATA_LEN  8


/*
 * Initial multi device type support, largely untested.
 * Sadly, the vendor uses proprietary Firmware IDs and product strings to
 * identify devices (which vary in capabilities and need to be queried
 * accordingly).
 */
static const struct TEMPerdev_types {
    unsigned short vend;
    unsigned short prod;
    float div;
    unsigned char data_len;
    unsigned char int_temp;
    unsigned char int_hum;
    unsigned char ext_temp;
    unsigned char ext_hum;
    const char * prod_name;
    const char * fw_id;
} devtypes[] = {
 /* See https://github.com/ccwienk/temper#supported-devices,
  *     https://github.com/ccwienk/temper/blob/master/temper.py#L198 ff.
  * Note the space at the end of the firmware ID string.
  */
{ 0x0c45, 0x7401, 256.0, 1, 2, 0, 0, 0, "TEMPer", "TEMPerF1.2 " },    // NOT tested
{ 0x0c45, 0x7401, 256.0, 1, 2, 0, 0, 0, "TEMPer", "TEMPerF1.4 " },    // NOT tested

{ 0x413d, 0x2107, 100.0, 1, 2, 0, 0, 0, "TEMPerGold", "TEMPerGold_V3.1 " }, // Ansel's sensor
{ 0x1a86, 0xe025, 100.0, 1, 2, 0, 0, 0, "TEMPerGold", "TEMPerGold_V3.3 " }, // NOT tested
{ 0x1a86, 0xe025, 100.0, 1, 2, 0, 0, 0, "TEMPerGold", "TEMPerGold_V3.4 " }, // NOT tested
{ 0x3553, 0xa001, 100.0, 1, 2, 0, 0, 0, "TEMPerGold", "TEMPerGold_V3.5 " }, // tested

{ 0x413d, 0x2107, 100.0, 1, 2, 4,10,12, "TEMPerHUM", "TEMPerX_V3.1 " }, // NOT tested
{ 0x1a86, 0xe025, 100.0, 1, 2, 3,10, 0, "TEMPerHUM", "TEMPerHUM_V3.8 " }, // NOT tested, https://github.com/ccwienk/temper/pull/45
{ 0x1a86, 0xe025, 100.0, 1, 2, 3,10, 0, "TEMPerHUM", "TEMPerHUM_V3.9 " }, // NOT tested

/* Not supported, values aren't byte aligned and need special conversion.
 * { 0x0c45, 0x7402, 1.0, ... "TEMPerHUM", "TEMPer1F_H1V1.5F " }, 
 */

{ 0x413d, 0x2107, 100.0, 1, 2, 4,10,12, "TEMPer2", "TEMPerX_V3.3 " }, // NOT tested
{ 0x1a86, 0xe025, 100.0, 1, 2, 0,10, 0, "TEMPer2", "TEMPer2_V3.7 " }, // NOT tested
{ 0x1a86, 0xe025, 100.0, 1, 2, 0,10, 0, "TEMPer2", "TEMPer2_V3.9 " }, // NOT tested
{ 0x1a86, 0xe025, 256.0, 1, 2, 0, 4, 0, "TEMPer2", "TEMPer2_M12_V1.3 " }, // NOT tested
{ 0x3553, 0xa001, 100.0, 1, 2, 0,10, 0, "TEMPer2", "TEMPer2_V4.1 " }, // NOT tested

{ 0x413d, 0x2107, 100.0, 1, 2, 4,10,12, "TEMPer1F", "TEMPerX_V3.3 " }, // NOT tested
{ 0x1a86, 0xe025, 100.0, 1, 2, 0, 0, 0, "TEMPer1F", "TEMPer1F_V3.9 " }, // NOT tested

/* Not supported, accessible via TTY commands only.
{ 0x1a86, 0x5523, 1.0,   xxx "TEMPerX232", "TEMPerX232_V2.0 " }, */

{ 0x0c45, 0x7401, 256.0, 1, 0, 2, 0, 0,   "TEMPer1V1.1", "TEMPer1F1.1Per1F" } // NOT tested
};


/*
 * Helper struct for extracting unique USB IDs from the above.
 */
struct usb_id {
    unsigned short vend;
    unsigned short prod;
};

/*
 * Send a query to the device and read the response.
 */
int query_device( hid_device *dev,
      unsigned char *query, unsigned char *res, unsigned int len, char *desc )
{
    // Send the query
    if ( hid_write( dev, query, QUERY_LEN ) < QUERY_LEN )
    {
        fprintf( stderr,
                "Error: %s: could not write to TEMPer device.\n", desc );
        return 1;
    }
    // Read the response
    unsigned int i;
    for ( i=0; i<len; i++) {
        if ( hid_read_timeout(
                    dev, res + DATA_LEN * i, DATA_LEN, 1000 ) < DATA_LEN )
        {
            fprintf( stderr,
                    "Error: %s: could not read from TEMPer device.\n", desc);
            return 1;
        }
    }

    return 0;
}

/*
 * Helper function for comparing wchar_t string to regular char string.
 */
int wcharcmp( const char *s1, const wchar_t *w1 ) {
    int size = wcstombs( NULL, w1, 0 ) + 1;
    char * sw1 = calloc( size, 1 );

    wcstombs( sw1, w1, size );
    int ret = strcmp( sw1, s1 );
    free (sw1);

    return ret;
}

/*
 * Check if data is included (offset > 0) for device type, then print.
 */
void print_data( char* desc, unsigned char *data, unsigned char offs, float div)
{
    if (! offs)
        return;

    float val = (float) (data[offs] << 8 | data[offs+1]) / div;
    printf( ",%s:%.2f", desc, val );
}

/*
 * Queries the TEMPer device for the current temperature, in degrees Celsius,
 * and returns it.
 */
void query_data( hid_device *dev, const struct TEMPerdev_types *type )
{
    // Send the temperature query to the device
    unsigned char query[QUERY_LEN] = DATA_QUERY;
    unsigned char data[DATA_LEN*2] = {0}; //max data len is 16

    if ( query_device( dev, query, data, type->data_len, "Query Data" ) )
        return;

    print_data("int-temp", data, type->int_temp, type->div);
    print_data("int-hum",  data, type->int_hum,  type->div);
    print_data("ext-temp", data, type->ext_temp, type->div);
    print_data("ext-hum",  data, type->ext_hum,  type->div);
}

/*
 * Queries the device firmware ID and matches fw_id and USB product string
 * against known devices.
 */
const struct TEMPerdev_types* query_devtype (
        hid_device* dev, struct hid_device_info *info)
{
    // Send the FWID query to the device
    unsigned char query[QUERY_LEN] = FWID_QUERY;
    unsigned char data[DATA_LEN*2+1] = { 0 };

    if ( query_device( dev, query, data, 2, "Query Firmware ID") )
        return NULL;

    // Match firmware ID and product string
    unsigned int i;
    for ( i=0; i<( sizeof(devtypes)/sizeof(devtypes[0]) ); i++ )
        if (    ( 0 == strcmp(devtypes[i].fw_id, data) )
             && ( 0 == wcharcmp(devtypes[i].prod_name, info->product_string) ) )
            return &devtypes[i];

   fprintf( stderr, "Error: Unknown device '%s'", data);
   return NULL;
}

/*
 * Open the HID device; print error on failure.
 */
hid_device * open_device( struct hid_device_info * info )
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
 * Query data from all devices that use a gived USB ID (vendor+product).
 */
int query_data_by_usbid(struct usb_id *usb_id)
{
    int ret = 0;
    // Attempt to open and return TEMPer device
    struct hid_device_info *infos = hid_enumerate( usb_id->vend, usb_id->prod );
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

                const struct TEMPerdev_types* type = query_devtype( dev, info );
                if (! type)
                {
                    fprintf( stderr,
                            " USB ID %x:%x\n", usb_id->vend, usb_id->prod );
                    return ret;
                }

                printf("%s,%s,%s", info->path, type->prod_name, type->fw_id);
                query_data(dev, type);
                printf("\n");

                ret++;
            }
        }
        hid_free_enumeration( infos );
    }

    return ret;
}

/*
 * Generate a list of unique USB IDs (vendor+product) shared across devices.
 */
unsigned int extract_unique_usbids(struct usb_id* ids)
{
    int in_array(
            struct usb_id* ids, unsigned short vend, unsigned short prod) {
        unsigned int i;
        for ( i=0; i<( sizeof(devtypes)/sizeof(devtypes[0]) ); i++)
            if ( (ids[i].vend == vend) && (ids[i].prod == prod) )
                return 1;
        return 0;
    }

    unsigned int i,j=0;
    for ( i=0; i<( sizeof(devtypes)/sizeof(devtypes[0]) ); i++) {
        if (in_array(ids, devtypes[i].vend, devtypes[i].prod))
            continue;
        ids[j].vend = devtypes[i].vend;
        ids[j].prod = devtypes[i].prod;
        j++;
    }

    return j;
}

int main(int argc, char** argv)
{
    // Initialize HIDAPI library
    if ( hid_init() != 0 )
    {
        fprintf( stderr, "Error: could not initialize HIDAPI\n" );
        return 1;
    }

    unsigned int len;
    struct usb_id unique_ids[ sizeof(devtypes)/sizeof(devtypes[0]) ] = {{0}};
    len = extract_unique_usbids(unique_ids);

    unsigned int i, found=0;
    for ( i=0; i<len; i++ )
        found += query_data_by_usbid(&unique_ids[i]);

    if ( ! found ) {
        fprintf( stderr, "No usable devices found. Sorry :-/\n");
        return 2;
    }

    return 0;
}
