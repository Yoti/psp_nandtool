#ifndef USB_H
#define USB_H

#define MS_USB_MOUNT 0
#define HOST_USB_MOUNT 1
#define USB_STOP 0
#define USB_START 1

// bitmasks
/*hard.usbEnable (capabilities):
	0000 = no USB support at all (0x0)
	0001 = standard usb support enabled (0x1)
	0010 = standard usb support + usbhost enabled */
#define NO_USB_CAP 0x00
#define MS_USB_CAP 0x01
#define HOST_USB_CAP 0x02

/*hard.usbStat (currently enabled)
	0000 = not enabled (0x0)
	0001 = usb MS loaded (0x1)
	0010 = host loaded (0x2)
	0011 = MS usb loaded, host temp unloaded (0x3, 0x1&0x2)
	1000 = usb drivers loaded (0x8)*/
#define USB_EN_NONE 0x00
#define USB_EN_MS 0x01
#define USB_EN_HOST 0x02
#define USB_EN_HMS 0x03 // MS usb loaded over host support
#define USB_EN_DRIV 0x04 // initial driver already started

void setupUsb(void); // starts usb prx's and gets the capability of the USB config
void toggleUsb(int mode); // 0 mounts MS, 1 mounts host

#endif //  USB_H 
