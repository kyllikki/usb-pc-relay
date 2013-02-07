/* USB PC Relay Board HIDDev setting program
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2012 Vincent Sanders <vince@kyllikki.org>
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hiddev.h>

#define DEVICE_VENDOR (0x12bf)
#define DEVICE_PRODUCT (0xff03)

int main(int argc, char **argv)
{
    char *dev_filename; /* device node file name */
    int dev_fd; /* device node file descriptor */
    uint8_t set_value = 0;
    uint8_t set_idx = 0;
    unsigned int optval;
    struct hiddev_devinfo device_info; /* device information */
    struct hiddev_report_info report_info; /* report information */
    struct hiddev_usage_ref usage; /* report data */
    const char *err_txt = "Hiddev";
    int opt;

    dev_filename = strdup("/dev/usb/hiddev0");

    /* command line args */
    while ((opt = getopt(argc, argv, "d:v:i:")) != -1) {
        switch (opt) {
        case 'd':
            free(dev_filename);
            dev_filename = strdup(optarg);
            break;

        case 'v':
            optval = atoi(optarg);
            if ((optval) > 255) {
                fprintf(stderr, "Value %d must be between 0 and 255\n", optval);
                exit(EXIT_FAILURE);
            }
            set_value = optval;
            break;

        case 'i':
            optval = atoi(optarg);
            if ((optval) > 7) {
                fprintf(stderr, "Index %d must be between 0 and 7\n", optval);
                exit(EXIT_FAILURE);
            }
            set_idx = optval;
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d device] [-v value]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }


    dev_fd = open(dev_filename, O_RDONLY);
    if (dev_fd == -1) {
        perror("Opening");
        return 1;
    }

    /* device information */
    if (ioctl(dev_fd, HIDIOCGDEVINFO, &device_info) == -1) {
        err_txt = "Device was not a Hiddev\n";
        goto hiddev_error;
    }

    /* check vendor */
    if (device_info.vendor != DEVICE_VENDOR) {
        fprintf(stderr, 
                "Device vendor was 0x%x expecting 0x%x\n", 
                device_info.vendor,
                DEVICE_VENDOR);
        close(dev_fd);
        return 1;
    }

    /* check product */
    if ((device_info.product & 0xffff) != DEVICE_PRODUCT) {
        fprintf(stderr, 
                "Device product was 0x%x expecting 0x%x\n", 
                device_info.product,
                DEVICE_PRODUCT);
        close(dev_fd);
        return 2;
    }

    /* get the output report info */
    report_info.report_type = HID_REPORT_TYPE_OUTPUT;
    report_info.report_id = HID_REPORT_ID_FIRST;
    if (ioctl(dev_fd, HIDIOCGREPORTINFO, &report_info) == -1) {
        err_txt = "Error filling report info";
        goto hiddev_error;
    }

    /* setup the value */
    usage.report_type = report_info.report_type;
    usage.report_id = report_info.report_id;
    usage.field_index = 0;
    usage.usage_index = set_idx;
    usage.value = set_value;

    if (ioctl(dev_fd,HIDIOCGUCODE , &usage) < 0) {
        err_txt = "Error getting usage code";
        goto hiddev_error;
    }

    /* set the value in the report */
    if (ioctl(dev_fd, HIDIOCSUSAGE, &usage) < 0 ) {
        err_txt = "Error setting usage";
        goto hiddev_error;
    }

    report_info.num_fields = 1;
    if (ioctl(dev_fd, HIDIOCSREPORT, &report_info) != 0) {
        err_txt = "Error sending report";
        goto hiddev_error;
    }

    close(dev_fd);

    return 0;

hiddev_error:

    perror(err_txt);

    close(dev_fd);

    return 10;
}
