#include "main.h"
#include "usb.h"
#include <pspusb.h>
#include <pspusbstor.h>

extern hard_info hard;

char usbPaths150[7][30] =
{
	"ms0:/elf/prx/usb.prx",
	"ms0:/elf/prx/semawm.prx",
	"ms0:/elf/prx/usbstor.prx",
	"ms0:/elf/prx/usbstormgr.prx",
	"ms0:/elf/prx/usbstorms.prx",
	"ms0:/elf/prx/usbstorboot.prx",
	"ms0:/elf/prx/usbhostfs.prx"
};

char usbPaths4xx[7][28] =
{
	"flash0:/kd/chkreg.prx",   
	"flash0:/kd/npdrm.prx",
	"flash0:/kd/semawm.prx",
	"flash0:/kd/usbstor.prx",
	"flash0:/kd/usbstormgr.prx",
	"flash0:/kd/usbstorms.prx",
	"flash0:/kd/usbstorboot.prx"
};

void setupUsb(void) // loads USB modules to determine availability for hard.usbEnable
{
	int retVal, i;
	printf("devkit ver: 0x%08x\n", hard.dkver);
	int mod = ntKernelFindModuleByName("sceUSB_Driver");
	if (mod) // check to see if usb.prx is already loaded by the cnf file
	{
		cprintf(C_GREEN, "sceUSB_Driver found\n");
		swap();
		i = 1;
	}
	else
		i = 0;

	if(hard.dkver <= 0x01050001) // early DC based on 1.50 kernel
	{
		for(; i < 6; i++)
		{
			printf("loading %s", usbPaths150[i]);
			retVal = LoadStartModule(usbPaths150[i]);
			if(retVal != 0)
			{
				cprintf(C_RED, " load error %08x\n", retVal);
				hard.usbEnable = 0;
				if(i<=1)
				{
					printf("USB disabled!\n\n");
					swap();
					if(getKeys() & 0x008000) // PSP_CTRL_SQUARE - square button held so pause to read the message
					{
						cprintf(C_GREEN,"Press X to continue\n");
						swap();
						while (confirm() != 1){;}
					}
				}
				swap();
				return;
			}
			else
				cprintf(C_GREEN, " OK!\n");
			swap();
		}
	}
	else // 4xx kernels
	{
		for(; i < 7; i++)
		{
			printf("loading %s", usbPaths4xx[i]);
			retVal = LoadStartModule(usbPaths4xx[i]);
			if(retVal != 0)
			{
				cprintf(C_RED, " load error %08x\n", retVal);
				hard.usbEnable = 0;
				if(i<=1)
				{
					printf("USB disabled!\n\n");
					swap();
					if(getKeys() & 0x008000) // PSP_CTRL_SQUARE - square button held so pause to read the message
					{
						cprintf(C_GREEN,"Press X to continue\n");
						swap();
						while (confirm() != 1){;}
					}
				}
				swap();
				return;
			}
			else
				cprintf(C_GREEN, " OK!\n");
			swap();
		}
	}
	
	// resolve function pointers, helps when usb drivers arent loaded but the app is being linked by loadcore
	ntResolveUSB();
	
	printf("loading ms0:/elf/prx/usbhostfs.prx");
//	sceKernelDelayThread(1000*1000);
	retVal = LoadStartModule(usbPaths150[6]);
	if(retVal == 0)
	{
		cprintf(C_GREEN, " OK!\n");
		hard.usbEnable = HOST_USB_CAP;
	}
	else
	{
		cprintf(C_RED, " Unable to load host support.\n");
		hard.usbEnable = MS_USB_CAP;
	}
	hard.usbStat = NO_USB_CAP;
	if(getKeys() & 0x008000) // PSP_CTRL_SQUARE - square button held so pause to read the message
	{
		cprintf(C_GREEN,"Press X to continue\n");
		swap();
		while (confirm() != 1){;}
	}
	fadeTo(C_BLACK, 31);
//	swap();
//while (confirm() != 1){;}
}

int usbToggleHost(int mode)
{
	int retVal;

	if(mode == USB_START)
	{
		retVal = ntUsbStart("USBHostFSDriver", 0, 0);
		if (retVal != 0)
		{
			printf("Error starting USBHostFS driver (0x%08X)\n Press X to continue\n", retVal);
			swap();
			while (confirm() != 1){};
			return 0;
		}
		ntUsbActivate(0x1c9);
		strcpy(hard.pathBase[DUMP_PATH], BASE_DUMP_PATH_HOST);
		hard.usbStat = USB_EN_HOST|USB_EN_DRIV;
		sceKernelDelayThread(1*1000*1000);
		verifyCreateDumpDir(); // since I'm messing with the base path, make sure the dump dir exists
	}
	else if(mode == USB_STOP)
	{
		ntUsbDeactivate(0x1c9);
		retVal = ntUsbStop("USBHostFSDriver", 0, 0);
		if (retVal != 0)
		{
			printf("Error stopping USBHostFS driver (0x%08X)\n Press X to continue\n", retVal);
			swap();
			while (confirm() != 1){};
			return 0;
		}
		strcpy(hard.pathBase[DUMP_PATH], BASE_DUMP_PATH);
		hard.usbStat = USB_EN_NONE|USB_EN_DRIV;
		verifyCreateDumpDir(); // since I'm messing with the base path, make sure the dump dir exists
	}
	return 1;
}

int usbToggleMS(int mode)
{
	int retVal;

	if(mode == USB_START)
	{
		retVal = ntUsbStart(PSP_USBSTOR_DRIVERNAME, 0, 0); // SCE_USB_ERROR_INPROGRESS 0x80243006 // in progress		if (retVal != 0)
/*		if(retVal == 0x80243006)
		{
			ntUsbDeactivate(0x1c8);
			ntUsbStop(PSP_USBSTOR_DRIVERNAME, 0, 0);
			retVal = ntUsbStart(PSP_USBSTOR_DRIVERNAME, 0, 0);
		}*/
		if (retVal != 0)
		{
			if(retVal == 0x80243006)
				printf("Error USBStor in use!!\n (0x%08X)", retVal);
			else
				printf("Error starting USBStor!\n (0x%08X)\n", retVal);
			cprintf(C_RED,"USB deactivated\n");
			printf("Press X to continue\n");
			swap();
			while (confirm() != 1){};
			return 0;
		}
		ntUsbActivate(0x1c8);
		if (ntUsbstorBootSetCapacity != NULL)
			ntUsbstorBootSetCapacity(0x800000);
	}
	else if(mode == USB_STOP)
	{
		ntUsbDeactivate(0x1c8);
		sceIoDevctl("fatms0:", 0x0240D81E, NULL, 0, NULL, 0 ); //Flush USB device to disk
		retVal = ntUsbStop(PSP_USBSTOR_DRIVERNAME, 0, 0);
		if (retVal != 0)
		{
			printf("Error stopping USBStor driver (0x%08X)\n Press X to continue\n", retVal);
			swap();
			while (confirm() != 1){};
			return 0;
		}
	}
	return 1;
}

void showUsbState(int contbut, int mode)
{
	int state, i = 1000;
	setc(0,0);
	if(mode == USB_EN_MS)
		cprintf(C_GREEN,"Mount MS to USB\n\n");
	else if (mode == USB_EN_HOST)
		cprintf(C_GREEN,"Starting Host support\n\n");
	else
		cprintf(C_GREEN,"Restarting Host support\n\n");
	cprintf(C_LT_BLUE," USB Driver    :\n");
	cprintf(C_LT_BLUE," USB Cable     :\n");
	cprintf(C_LT_BLUE," USB Connection:\n\n");
	if(contbut)
		cprintf(C_PURPLE,"Press X to disconnect.");
	else
		cprintf(C_PURPLE,"Press X to cancel.");
	swap();
	while(1)
	{
		sceKernelDelayThread(10*1000);
		if(i >= 1000) // we don't want to be bothering the USB too often, so only check it once every second or so
		{
			state = ntUsbGetState();
			setc(17,2);
			if(state&PSP_USB_ACTIVATED)
				cprintf(C_GREEN,"Active       ");
			else
				cprintf(C_RED,"ERR: NO USB!!"); // not likely to ever see this...
			setc(17,3);
			if(state & PSP_USB_CABLE_CONNECTED)
				cprintf(C_GREEN,"Connected    ");
			else
				cprintf(C_RED,"Connect cable");
			setc(17,4);
			if(state&PSP_USB_CONNECTION_ESTABLISHED)
			{
				cprintf(C_GREEN,"Established  ");
				if (!contbut)
					return;
			}
			else
				cprintf(C_RED,"Waiting...   ");
			swap();
		}
		if(getKeys() & 0x004000) //PSP_CTRL_CROSS
		{
			wait_release(0x004000);
			break;
		}
		i++;
	}
	if(contbut)
	{
		usbToggleMS(USB_STOP); // MS mount always waits for keys, so it wont hit the return above at established connections and will shut down USB MS access when it hits here
	}
	else // else we jumped out of starting usbhost
	{
		usbToggleHost(USB_STOP);
	}
	return;
}

void toggleUsb(int mode)
{
    int retVal;
	showPopupBox(17, 12, 32, 7, C_YORANGE, C_BLACK, C_BLACK, 25);

	if(!(hard.usbStat & USB_EN_DRIV)) // if the driver isnt started
	{
	    retVal = ntUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
	    if ((retVal != 0) && (retVal != 0x80243001)) // if it is already started trying to start it again will give  SCE_USB_ERROR_ALREADY 0x80243001
		{
			printf("Error starting USB Bus driver\n (0x%08X)\n", retVal);
			hard.usbEnable = 0;
			cprintf(C_PURPLE,"Press X to continue.");
			swap();
			while (confirm() != 1){};
			return;
	    }
		hard.usbStat = USB_EN_DRIV;
	}

	if (mode == MS_USB_MOUNT)
	{
		if(hard.usbStat & USB_EN_HOST)
		{
			usbToggleHost(USB_STOP);
			hard.usbStat = USB_EN_HMS|USB_EN_DRIV;
			sceKernelDelayThread(100*1000);
		}
		if(usbToggleMS(USB_START) == 0)
		{
			hard.usbEnable = 0;
			hidePopupBox();
			return;
		}
		showUsbState(1, USB_EN_MS);
	}
	else if (mode == HOST_USB_MOUNT)
	{
		if (hard.usbStat & USB_EN_HOST)
			usbToggleHost(USB_STOP);
		else
		{
			usbToggleHost(USB_START);
			showUsbState(0, USB_EN_HOST);
		}
		hidePopupBox();
		return;
	}
	
	if (hard.usbStat & USB_EN_HOST) // restart host support if it was stopped to mount the memory stick, won't get here when toggling host support
	{
		usbToggleHost(USB_START);
		showUsbState(0, USB_EN_HMS);
		hard.usbStat = USB_EN_HOST|USB_EN_DRIV;
	}
	hidePopupBox();
	return;
}
