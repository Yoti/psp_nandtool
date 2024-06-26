nandTool 0.4 Final NEO Aug 20 2008 (Des.Cem.M8 - friend of Despertar_Cementario) 
----------------------
**IMPORTANT: if you ever get "fixed bad block mark in data" it is likely the dump you are flashing is corrupt! **
** logical repartition should now work on all PSPs except those with *alot* of bad blocks (more than 80)**


Disclaimer
----------
The author is not responsible in any way for any problems that may arise from the use of the included program.
Using the program constitutes your acceptance of responsibility for anything it may do.
No warranties expressed or implied - by anyone or for anything.


The most likely common use of this app will be to repair lflash partitions
	(aka: pandora/des.cem freezes up at formatting flash1)
Here is the step by step for this particular use:
-------------------------------------------------
1 - backup your NAND using the dump options
2 - use other/lflash format
	optional - use other/format partitions (you will need to place act.dat in flash2 and 1seg eboot on flash3 yourself)
3 - reboot with pandora or des.cem V3-5 to install firmware.
4 - done.

** it can be used as a resurrection.elf (DC1-6) replacement as well as a resurrection.prx (DC6-7) replacement. To use with DC6-7
you will need to extract data.psar from the eboot and use that as the resurrection.prx. ELF loading is currently NOT supported
in the eboot or DC6/7 methods.


menu options rundown
--------------------
- Load ELF
	lets you load elf files in a similar manner to jas0nuk's ELF menu, from the ms0:/elf folder
		currently available but not supported by eboot/DC6/DC7
- Dump from NAND
	dumps to a file with a name specific to the PSP it is dumped from, if it exists already it will ask you if you want to overwrite
		or increment (increment means it will tack _000 onto the filename and go again until you either tell it to overwrite or it finds a non existant file name)
- *Write to NAND RAW
	allows you to write raw blocks to NAND from any properly sized file in ms0:/nandTool_dumps/
- Partition Tools
	- Integrity check
		will scan the logical flash area of NAND for valid partition records and report whether there is damage
	-*repartition lflash
		similar in nature to writing a cleaned dump to NAND, will remap lflash so it will work even if bad blocks are present
	-*format partitions
		formats flash0/1/2/3 to contain files (does not fix partition damage, will not place flash1 directories)
			see note below regarding formatting partitions
- Other
	- bad block check
		will scan your NAND and report any blocks marked as bad
	- dump idstore keys
		similar to dump from NAND and idstore manager by Chilly Willy, will dump your idstore to key files in a custom dir in /nandTool_dump
	-*write idstore keys
		lets you pick the key directory from /nandTool_dump to restore keys from
			~note: it is better to raw write your original idstore wrather than restore from key files, firmware reinstall may be necissary after
				altering idstore.
		bad blocks in idstore area will be worked around
- Quit/shutdown
	kinda obvious... (shutdown will only be presented if the program is launched as a resurrection.elf replacement)
	
* - these options are the "dangerous" ones. I have tested them as best I can and all has gone well so far.
**- if you have installed the USB prx's from 1.50 to ms:/elf/prx the select key	in the main menu will allow you to mount MS to USB, the option will not appear if the prx's aren't installed.
**- if you have placed usbhostfs.prx into ms:/elf/prx the start key in the main menu will allow you to redirect dumping and writing operations to usbhost (host0) - the PC side must be running.

		
regarding Write to NAND RAW options:
------------------------------------
You must have a file of the correct size to enter the sub menu. It will have these options:
-write IPL only
	writes the area that normally contains IPL data (initial program loader)
-write lflash only
	writes the area that contains the logical disk which holds firmware files
-write everything but idstore
	same as writing IPL and lflash consecutively
-write idstore only
	writes the area responsible for storing per-psp information
	(if you lose the data that belongs to a particular PSP, there is no way to retreive it at this time)
-write full image
	writes from the very beginning to the very end of the NAND (pre IPL area which is normally unused, to very end)


about writing keys
-------------------
Don't expect miracles, this will not magically fix idstore if you ignored all warnings and did not back up your NAND before altering anything on it.
But, it may well make some PSP's more functional if you can get a key dump from another unit. DC7 also offers the option to create a fresh
idstore, but this option will also invalidate any PSN or PS3 linking you may have done.

The method is consistent with what official idstore drivers expect, but various data is not reproduced as original. Thus, if possible
it is always recommended that idstore be RAW restored from a dump on your own PSP. Testing has shown on a PSP slim, that the key files
from a different PSP slim will allow homebrew and UMD iso (noumd only) to run from XMB without an error code in CFW. Without your original
idstore keys, most secured functions will not work (ie: DNAS, UMD games).

	
about writing RAW
-----------------
in some cases writing data back to the PSP in a RAW form may not work. If the data you are writing needs it's data to
be placed in a block that is bad on your PSP, that data will not get written.


about bad blocks
----------------
Bad blocks aren't errors. They are intended by design so that the device does not fail when an area of your NAND becomes
unreliable. Such blocks are common, the only way they become a problem is if they are not dealt with properly. PSP's system
software handles such blocks transparently; the lflash format and idstore write from key options of nandTool will map the disk
space around any bad blocks present (hopefully) safely and effectively, provided there aren't far too many of them.


about "resetting" bad blocks
----------------------------------
A truly bad block should NEVER BE RESET. Once you write the data that would reset the block status to place it into the good
block pool, if it is truly bad (and has become so since the NAND left the manufacturer), you may never be able to set that status
back to bad again. Meaning, the PSP would always think it is reliable, and keep trying to store data there - can you imagine what
that would do to a PSP, if it kept placing data into a block and getting different data back when it read it? It would be only so
much junk that could never be fixed again.


Installation
-------------
There are other methods than these, but people keep asking and refuse to look elsewhere for answers, here is a couple options which
should be available to most people:

With Des.Cem V3, the simplest method to run nand tool:
-install jas0nuk's elf menu following it's instructions (0.2a can be found here: http://forums.maxconsole.net/showthread.php?t=83119 )
-put the elf and prx in the elf folder, run from the elf menu

With original pandora, the simplest method to run nand tool:
-on your memory stick, rename :/kd/backup.elf to something like :/kd/backup_orig.elf
-Take nandtool.elf and rename it backup.elf
-place in kd folder
-put ntbridge.prx into ms0:/elf/prx/ntbridge.prx
-when in pandora, select the backup nand option, and nandtool will run instead

Eboot
- I don't recommend using any nand writing app from an active firmware running from NAND, but the eboot version should
work under any of the current custom firmwares.

	
USB PRX Installation
---------------------
Use one of the various PSAR dumpers to get a dump of 1.50 firmware (use: crypted no sig), copy the following files to ms0:/elf/prx/ folder
usb.prx
semawm.prx
usbstor.prx
usbstorboot.prx
usbstormgr.prx
usbstorms.prx

If you place usbhostfs.prx here as well as all the above, usbhost support will be enabled in the application.

Formatting partitions
---------------------
Currently only supports custom firmware installations on slim and fat PSP
- the following files must be present for this option to appear
	ms:/elf/prx/lfatfs_updater.prx
	ms:/elf/prx/lflash_fatfmt_updater.prx
	ms:/elf/prx/nand_updater.prx
Copy the files from your target version firmware to this directory. For example, if you are using des.cem v5, copy them from
ms:/TM/DC5/kd/ - if you are using previous versions of des.cem, they will be found in ms:/kd

It is IMPORTANT that you match the files to the version of firmware you expect to run on the PSP, as sector scrambles chang on many major (0.10) updates
	
These files can also be extracted and decrypted (they must be decrypted) using jas0nuk's prxdecrypter from an updater's data.psp

credits:
---------
- thanks to 3r14nd for doing some impromptu bug testing during 0.4's cycle
- Chilly Willy for releasing source to his apps (3.xx examples), and for the key waiting and file/folder naming routines used in this app
- Everyone involved with Prometheus for creating and releasing the Pandora project (including but not limited to Noobz)
- Dark_Alex for proving CFW was possible, then going 1000 steps further and giving so much to everyone with a PSP
- jas0nuk, ELF menu is a godsend, getting tachyon/baryon and proper PSP model... priceless
- royginald, without your USB app making this tool would be even slower
- certain folks at lan.st - without your encouragement certain bugs would have never been squashed, and some features would not exist
- thanks to SilverSpring, we have the code to find the ids seed and know how to set/use it :)
- Everyone behind the forums and toolchains at PS2dev.org <- without them, PSP homebrew just... wouldn't be.
- anyone else I may have forgotten (as I am so great with names and all).......

If anyone feels I have abused the privelidges of their source releases, let me know and I will try to rectify it.
Source will be availble for the time being by request only. If you have a use for it, just let me know that is and I will take it into consideration.


Known Bugs or errata
---------------------
- disconnecting USB cable while using host redirection is not monitored, stop usbhost redirection before disconnecting the cable for a more pleasant experience all around.
	
If you come across something that is not working as expected, or have a really good suggestion for an addition, please let me know.


Contact Info
-------------
If you have general questions like "what is NAND" or "how does this program work", you can try me
	but they will likely be ignored as Google is a better friend than I will be if you ask.

cory1492 @ gmail . com


Donations:
----------
I know noobz.eu and darkalex.org both accept $$ donations and in my opinion deserve them much more than myself.
In my case, most often a thanks will do and can sometimes be considered overpayment ;)
