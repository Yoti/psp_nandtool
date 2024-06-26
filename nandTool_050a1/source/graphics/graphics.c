/*
 * Modified version of PSPSDK debug printf stuffs - cory1492, Dec 24 2007
	info below from original header
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * scr_printf.c - Debug screen functions.
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 *
 * $Id: scr_printf.c 2319 2007-09-30 15:58:31Z tyranid $
 */
#include <stdio.h>
#include <psptypes.h>
#include <pspkernel.h>
#include <pspdisplay.h>
//#include <pspsysclib.h>
#include <pspge.h>
#include <stdarg.h>
#include <string.h>
#include "graphics.h"

#define PSP_SCREEN_WIDTH 480
#define PSP_SCREEN_HEIGHT 272
#define PSP_LINE_SIZE 512
#define CHAR_HEIGHT 8
#define CHAR_WIDTH 7
#define SCREEN_CHAR_WIDTH 68
#define SCREEN_CHAR_HEIGHT 34
#define PERC_CHARS 40 // number of characters for the percent bar, should be a multiple of 10's

#define COLOR_HIDE 0x00000000 // the color of the overall background for hiding special characters

int sceDisplayIsVblank(void);
void  _myScreenClearLine( int Y);

static int X = 0, Y = 0;
static int MaxX=SCREEN_CHAR_WIDTH, MaxY=SCREEN_CHAR_HEIGHT, MinX=0, MinY=0;
static int storMaxX, storMaxY, storMinX, storMinY, storX, storY; // for temporary unbounding
static int savMaxX, savMaxY, savMinX, savMinY, savX, savY;// for popup saving
static u32 savBg_col = 0;// for popup saving
static u32 bg_col = 0, fg_col = 0xFFFFFFFF, perfg = 0xFFFFFFFF, perbg = 0 , pertx = 0;
static int bg_enable = 1;
static void* g_vram_base;
static void* fbuffer;
static int g_vram_mode = PSP_DISPLAY_PIXEL_FORMAT_8888;
static int swapped = 0;
static int perc_x = 0, perc_y = 0;
static int init = 0;
__attribute__((aligned(64))) static unsigned int fadebuff[512*272];
__attribute__((aligned(64))) static unsigned int bbuffer[512*272];

__attribute__((aligned(64))) static unsigned int imgbuf[480*272];

static void memcpy32(void *dst, const void *src, int len)
{
	const u32 *usrc = (const u32 *) src;
	u32 *udst = (u32 *) dst;

	while(len > 0)
	{
		*udst++ = *usrc++;
		len--;
	}
	return;
}

static void memfill32(void *dst, u32 val, int len)
{
	u32 *udst = (u32 *) dst;
	while(len > 0)
	{
		*udst++ = val;
		len--;
	}
	return;	
}

static void fadeCpy32(int count1, int count2)
{
	int pos;
	u32* dest = fbuffer;
	u32 red, green, blue, dred, dblue, dgreen;
	for(pos = 0; pos < (512*272);pos++)
	{
		red = bbuffer[pos] & 0xFF;
		green = (bbuffer[pos]>>8) & 0xFF;
		blue = (bbuffer[pos]>>16) & 0xFF;
		dred = fadebuff[pos] & 0xFF;
		dgreen = (fadebuff[pos]>>8) & 0xFF;
		dblue = (fadebuff[pos]>>16) & 0xFF;
		red = (((red*count1)>>6)+((dred*count2)>>6))>>1;
		green = (((green*count1)>>6)+((dgreen*count2)>>6))>>1;
		blue = (((blue*count1)>>6)+((dblue*count2)>>6))>>1;
		dest[pos] = red | (green << 8) | (blue << 16);
	}
	return;
}	

// fades out to color 'fadeto' from 0 to 31 degrees
void fadeTo(u32 color, int comp)
{
	int count1 = 128, count2 = 0, count3 = 0;
	if (comp > 31) comp = 31;
	else if (comp < 0) comp = 0;
//	memfill32(fadebuff, (color + (color<<8) + (color << 16)), 512*272); // fill the fade buffer with the destination color
	memfill32(fadebuff, color, 512*272); // fill the fade buffer with the destination color
	for(; count3 < comp; count3+=2)
	{
		count1 -=8;
		count2 +=8;
		fadeCpy32(count1, count2);
	}
	memcpy32(fadebuff, bbuffer, 512*272); // put the initial data into the back buffer for a restore from fade case
	memcpy32(bbuffer, fbuffer, 512*272); // make sure the final result is on the front buffer when done ?
	return;
}

// fades from whatever is currently on the screen to whatever is in the backbuffer
void fadeIn(void)
{
	memcpy32(fadebuff, fbuffer, 512*272); // copy vram into the fade buffer
	int count1 = 0,	count2 = 128, count3 = 0;
	for(; count3 < 16; count3++)
	{
		count1 +=8;
		count2 -=8;
		fadeCpy32(count1, count2);
	}
	memcpy32(bbuffer, fbuffer, 512*272); // make sure the final result is on the front buffer when done ?
}

void restoreFade(void)
{
	memcpy32(bbuffer, fadebuff, 512*272); // put fadebuffer data into the back buffer
	fadeIn(); // fade to the restored data
}

void enableRollUpArea(int xmin, int ymin, int xmax, int ymax)
{
	MinX = xmin;
	MinY = ymin;
	MaxX = xmax;
	MaxY = ymax;
	myScreenSetXY(0, 0);
	return;
}

void resetRollUpArea(void)
{
	MaxX = SCREEN_CHAR_WIDTH; //68;
	MaxY = SCREEN_CHAR_HEIGHT; //34;
	MinX = 0;
	MinY = 0;
}

static void rollUp(void)
{
	u32* vram = g_vram_base;
	u32 src, dest, i, j;
	
	for(i = MinY*CHAR_HEIGHT; i<(MaxY*CHAR_HEIGHT); i++) // hack method, copy pixels up
	{
		for(j = MinX*CHAR_WIDTH; j<(MaxX*CHAR_WIDTH); j++)
		{
			dest = j+(i * PSP_LINE_SIZE);
			src = dest+(PSP_LINE_SIZE*CHAR_HEIGHT);
			*(vram+dest) = *(vram+src);
		}
	}
	return;
}

static void putPixel(int x, int y, u32 color)
{
	u32* vram = g_vram_base;
	vram += x;
	vram += (y * PSP_LINE_SIZE);
	*vram = color;
	return;
}

void clearBoxChars(void)
{
	int y, i;

	for(y=MinY;y<MaxY;y++)
	{
		for (i=MinX; i < MaxX; i++)
		{
			myScreenPutChar( i * CHAR_WIDTH , Y * CHAR_HEIGHT, bg_col, 219);
		}
	}
	myScreenSetXY(0,0);
}

void clearBox(void)
{
	int i, j;
	int topx = MinX*CHAR_WIDTH-3;
	int topy = MinY*CHAR_HEIGHT-3;
	int botx = MaxX*CHAR_WIDTH+3;
	int boty = MaxY*CHAR_HEIGHT+2;

	for(i = topy; i<=boty; i++)
	{
		for(j=topx; j<=botx; j++)
			putPixel(j, i, bg_col);
	}
	myScreenSetXY(0,0);
}

void drawBox(int topx, int topy, int botx, int boty, u32 color)
{
	int i, j;
	if(init == 0)
		return;
	for(i = topy; i<=boty; i++)
	{
		if((i==topy) || (i==boty)) // top and bottom lines
		{
			for(j=topx; j<=botx; j++)
				putPixel(j, i, color);
		}
		else // all other lines
		{
			putPixel(topx, i, color);
			putPixel(botx, i, color);
		}
	}
	return;
}

void showPopupBox(int topx, int topy, int width, int height, u32 lineColor, u32 intColor, u32 fadeColor, u8 fade)
{
	// save some variables
	savMinX = MinX;
	savMinY = MinY;
	savMaxX = MaxX;
	savMaxY = MaxY;
	savX = X;
	savY = Y;
	savBg_col = bg_col;
	// fade out what is going to be the background
	fadeTo(fadeColor, fade);
	if(topx <= 0)
	{
		topx = 1;
		width = width - 1;
	}
	if(topy <= 0)
	{
		topy = 1;
		height = height - 1;
	}
	if((width+topx) >= SCREEN_CHAR_WIDTH)	width = (SCREEN_CHAR_WIDTH-1)-topx;
	if((height+topy) >=SCREEN_CHAR_HEIGHT)	height = (SCREEN_CHAR_HEIGHT-1)-topy;
	enableRollUpArea(topx, topy, topx+width, topy+height);
	drawBox((topx*CHAR_WIDTH)-4, (topy*CHAR_HEIGHT)-4,  ((topx+width)*CHAR_WIDTH)+4, ((topy+height)*CHAR_HEIGHT)+3, lineColor);
	bg_col = intColor;
	clearBox();
	return;
}

void hidePopupBox(void)
{
	// restore variables
	MinX = savMinX;
	MinY = savMinY;
	MaxX = savMaxX;
	MaxY = savMaxY;
	X = savX;
	Y = savY;
	bg_col = savBg_col;
	// put back what was previously on the screen
	restoreFade();
	return;
}

void drawTextBox(int topx, int topy, int width, int height, u32 color)
{
	if(topx <= 0)
	{
		topx = 1;
		width = width - 1;
	}
	if(topy <= 0)
	{
		topy = 1;
		height = height - 1;
	}
	if((width+topx) >= SCREEN_CHAR_WIDTH)	width = (SCREEN_CHAR_WIDTH-1)-topx;
	if((height+topy) >=SCREEN_CHAR_HEIGHT)	height = (SCREEN_CHAR_HEIGHT-1)-topy;
	enableRollUpArea(topx, topy, topx+width, topy+height);
	drawBox((topx*CHAR_WIDTH)-4, (topy*CHAR_HEIGHT)-4,  ((topx+width)*CHAR_WIDTH)+4, ((topy+height)*CHAR_HEIGHT)+3, color);
	return;
}

void updatePercent(int top, int bottom)
{
	int i, j;
	// save the current settings so we don't break whatever else is going on
	u32 currbg = bg_col;
	u32 currfg = fg_col;
	int percent = (top*100)/bottom;
	// error check the percent number
	if(percent < 0) percent = 0;
	if(percent >100) percent = 100;
	// some calcs that need only be done once
	j = (percent*PERC_CHARS)/100;
	removeBound();
	bg_enable = 1;
	bg_col = perfg;
	Y = perc_y;
	// draw the percent bar colors to clear the bar to whatever percent we are at
	for(i = 0; i<j; i++)
	{
		X = perc_x+i;
		myScreenPrintf(" ");
	}
	bg_col = perbg;
	for(i = j; i<PERC_CHARS; i++)
	{
		X = perc_x+i;
		myScreenPrintf(" ");
	}
	// place the text over top of the percent bar
	bg_enable = 0;
	fg_col = pertx;
	Y = perc_y;
	X = perc_x+((PERC_CHARS>>1)-2); // to give us a nicely centered percent
	if(percent != 100)
		X += 1;
	myScreenPrintf("%i%%", percent);
	// restore the stuff we saved previously
	bg_enable = 1;
	restoreBound();
	bg_col = currbg;
	fg_col = currfg;
//	myScreenSwapBuffer();
}	

void showPercentBar(int orgx, int orgy, u32 boxcolor, u32 bgcolor, u32 fgcolor, u32 txcolor)
{
	if(orgx <= 0) orgx = 1;
	if(orgy <= 0) orgy = 1;
	if((orgx+PERC_CHARS)>=SCREEN_CHAR_WIDTH)
		orgx = (SCREEN_CHAR_WIDTH-1)-PERC_CHARS;
	if(orgy>=SCREEN_CHAR_HEIGHT)
		orgy = (SCREEN_CHAR_HEIGHT-1);
	perc_x = orgx;
	perc_y = orgy;
	perbg = bgcolor;
	perfg = fgcolor;
	pertx = txcolor;
	drawBox((orgx*CHAR_WIDTH)-1, (orgy*CHAR_HEIGHT)-1,  ((orgx+PERC_CHARS)*CHAR_WIDTH)+1, ((orgy*CHAR_HEIGHT)+8), boxcolor);
	updatePercent(0,100);
	return;
}

void drawLine(int x, int y, int len, u32 color)
{
	int i;
	if(init == 0)
		return;
	for(i=0; i<len; i++)
		putPixel((i+x), y, color);
	return;
}

void removeBound()
{
	storMaxX = MaxX; storMaxY = MaxY; storMinX = MinX; storMinY = MinY;
	storX = X; storY = Y;
	MaxX = SCREEN_CHAR_WIDTH; MaxY = SCREEN_CHAR_HEIGHT; MinX = 0; MinY = 0;
	return;
}

void restoreBound()
{
	MaxX = storMaxX; MaxY = storMaxY; MinX = storMinX; MinY = storMinY;
	X = storX; Y = storY;
	return;
}

static void clear_screen(u32 color)
{
	int x;
	u32 *vram = g_vram_base;

	for(x = 0; x < (PSP_LINE_SIZE * PSP_SCREEN_HEIGHT); x++)
	{
		*vram++ = color; 
	}
}

void myScreenInit(void)
{
	X = Y = 0;
	swapped = 0;

	fbuffer = (void*) (0x40000000 | (u32) sceGeEdramGetAddr());
	g_vram_base = (void*) fbuffer; // render to the back buffer until a buf swap
	g_vram_mode = PSP_DISPLAY_PIXEL_FORMAT_8888;
	clear_screen(bg_col);
	g_vram_base = (void*) bbuffer;
	clear_screen(bg_col);

	sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
	sceDisplaySetFrameBuf((void *) fbuffer, PSP_LINE_SIZE, g_vram_mode, PSP_DISPLAY_SETBUF_NEXTFRAME);

	resetRollUpArea();
	init = 1;
}

void myScreenSwapBuffer(void)
{
	sceDisplayWaitVblankStart();
	memcpy32((u32*)fbuffer, bbuffer, PSP_LINE_SIZE*PSP_SCREEN_HEIGHT);
	//sceDisplayWaitVblank();
	//sceDisplaySetFrameBuf((void *) render, PSP_LINE_SIZE, g_vram_mode, PSP_DISPLAY_SETBUF_NEXTFRAME);
}

void myScreenEnableBackColor(int enable) 
{
	bg_enable = enable;
}

void myScreenSetBackColor(u32 colour)
{
	bg_col = colour;
}

void myScreenSetTextColor(u32 colour)
{
	fg_col = colour;
}

int myScreenGetX(void)
{
	return X;
}

int myScreenGetY(void)
{
	return Y;
}

void myScreenClear(void)
{
	int y;

	if(!init)
	{
		return;
	}

	for(y=MinY;y<MaxY;y++)
	{
		_myScreenClearLine(y);
	}

	resetRollUpArea();
	myScreenSetXY(0,0);
	clear_screen(bg_col);
}

void myScreenSetXY(int x, int y)
{
	x = x + MinX; // keeping it relative to the bounds
	y = y + MinY;
	if( x<MaxX && x>=MinX )
		X=x;
	else if (x > MaxX)
		X = MaxX;
	else if (x < MinX)
		X = MinX;
	if( y<MaxY && y>=MinY )
		Y=y;
	else if (y > MaxY)
		Y = MaxY;
	else if (y < MinY)
		Y = MinY;
}

extern u8 msx[];
void myScreenPutChar( int x, int y, u32 color, u8 ch)
{
	int i,j;
	u8	*font;
	u32 *vram_ptr;
	u32 *vram;

	if(!init)
	{
		return;
	}

	vram = g_vram_base;
	vram += x;
	vram += (y * PSP_LINE_SIZE);
	
	font = &msx[ (int)ch * CHAR_HEIGHT];
	for (i=0; i < CHAR_HEIGHT; i++, font++)
	{
		vram_ptr  = vram;
		for (j=0; j < CHAR_HEIGHT; j++)
		{
			if ((*font & (0x80 >> j)))  //#define BIT(n) (1 << (n))
				*vram_ptr = color; 
			else if(bg_enable)
				*vram_ptr = bg_col; 
			vram_ptr++;
		}
		vram += PSP_LINE_SIZE;
	}
}

extern u8 spChars[];
void putSChar(int ch, int x, int y)
{
	int i, j, hide;
	u32* vram;
	u32* vram_ptr;
	u8* sp = ((u8*)spChars)+(ch*56);

	if(!init) return;
	
	if(ch < 0)
	{
		hide = 1;
		ch = ch * -1;
	}
	
	if(x < 0)
		x = X;
	if(y < 0)
		y = Y;

	vram = g_vram_base;
	vram += (x * CHAR_WIDTH);
	vram += (y * PSP_LINE_SIZE * CHAR_HEIGHT);

	for(i=0; i<CHAR_HEIGHT; i++)
	{
		vram_ptr = vram;
		for(j=0; j<CHAR_WIDTH; j++)
		{
			if (*sp == 1)
			{
				if(hide != 0)
					*vram_ptr = bg_col;
				else
					*vram_ptr = COLOR_HIDE;
			}
//			else if(bg_enable)
//				*vram_ptr = bg_col;
			sp++;
			vram_ptr++;
		}
		vram += PSP_LINE_SIZE;
	}
	
	X++;
	if (X == MaxX)
	{
		X = MinX;
		Y++;
		if (Y == MaxY)
		{
			Y = MaxY-1;
			rollUp();
		}
		_myScreenClearLine(Y);
	}
	return;
}

void myScreenPutSChar(int ch)
{
	putSChar(ch, -1, -1);
	return;
}

void  _myScreenClearLine(int Y)
{
	int i;
	if(bg_enable)
	{
		for (i=MinX; i < MaxX; i++)
		{
			myScreenPutChar( i * CHAR_WIDTH , Y * CHAR_HEIGHT, bg_col, 219);
		}
	}
}

/* Print non-nul terminated strings */
int myScreenPrintData(const char *buff, int size)
{
	int i;
	int j;
	char c;

	if(!init)
	{
		return 0;
	}

	for (i = 0; i < size; i++)
	{
		c = buff[i];
		// for some reason, switches just don't work well in my code...
		switch(c)
		{
			case '\n':
				X = MinX;
				Y ++;
				if (Y == MaxY)
				{
					Y = MaxY-1;
					rollUp();
				}
				_myScreenClearLine(Y);
				break;
			case '\t':
				for (j = 0; j < 5; j++)
				{
					myScreenPutChar( X*CHAR_WIDTH , Y * CHAR_HEIGHT, fg_col, ' ');
					X++;
				}
				break;
			default:
				myScreenPutChar( X*CHAR_WIDTH , Y * CHAR_HEIGHT, fg_col, c);
				X++;
				if (X == MaxX)
				{
					X = MinX;
					Y++;
					if (Y == MaxY)
					{
						Y = MaxY-1;
						rollUp();
					}
					_myScreenClearLine(Y);
				}
				break;
		}
	}
/*
		if(c == '\n')
		{
			X = MinX;
			Y ++;
			if (Y == MaxY)
			{
				Y = MaxY-1;
				rollUp();
			}
			_myScreenClearLine(Y);
		}
*/
/* shouldn't need tab support in this app
		else if(c == '\t')
		{
			for (j = 0; j < 5; j++)
			{
				myScreenPutChar( X*CHAR_WIDTH , Y * CHAR_HEIGHT, fg_col, ' ');
				X++;
				if (X == MaxX)
				{
					X = MinX;
					Y++;
					if (Y == MaxY)
					{
						Y = MaxY-1;
						rollUp();
					}
					_myScreenClearLine(Y);
				}
			}
		}

		else if(c == '_') // underscores are displayed as spaces
		{
			myScreenPutChar( X*CHAR_WIDTH , Y * CHAR_HEIGHT, fg_col, ' ');
			X++;
			if (X == MaxX)
			{
				X = MinX;
				Y++;
				if (Y == MaxY)
				{
					Y = MaxY-1;
					rollUp();
				}
				_myScreenClearLine(Y);
			}
		}
*/
/*
		else
		{
			myScreenPutChar( X*CHAR_WIDTH , Y * CHAR_HEIGHT, fg_col, c);
			X++;
			if (X == MaxX)
			{
				X = MinX;
				Y++;
				if (Y == MaxY)
				{
					Y = MaxY-1;
					rollUp();
				}
				_myScreenClearLine(Y);
			}
		}
	}
*/
	return i;
}

int myScreenPuts(const char *str)
{
	return myScreenPrintData(str, strlen(str));
}

void myScreenPrintf(const char *format, ...)
{
	va_list	opt;
	char	 buff[2048];
	int		bufsz;
	
	va_start(opt, format);
	bufsz = vsnprintf( buff, (size_t) sizeof(buff), format, opt);
	(void) myScreenPrintData(buff, bufsz);
	va_end(opt);
}


//  BITMAP
typedef struct
{
	char signature[2];
	unsigned int fileSize;
	unsigned int reserved;
	unsigned int offset;
}__attribute__ ((packed)) BmpHeader;

typedef struct
{
	unsigned int headerSize;
	unsigned int width;
	unsigned int height;
	unsigned short planeCount;
	unsigned short bitDepth;
	unsigned int compression;
	unsigned int compressedImageSize;
	unsigned int horizontalResolution;
	unsigned int verticalResolution;
	unsigned int numColors;
	unsigned int importantColors;
	
}__attribute__ ((packed)) BmpImageInfo;

typedef struct
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char reserved;
}__attribute__ ((packed)) Rgb;

typedef struct
{
	BmpHeader header;
	BmpImageInfo info;
// only for paletted bitmap files, in this case 8bpp
	Rgb colors[256]; 
}__attribute__ ((packed)) BmpFile;

int decodeBmp(u8* bmp_data, u16 width, u16 height, u32* dest)
{
	BmpFile* bmp = (BmpFile*)bmp_data;
	if ((width != bmp->info.width) || (height != bmp->info.height))
		return -1;

	// adjust for padded scanlines
	u32 row_width, x ,y; 
	if (bmp->info.bitDepth == 24)
		row_width = bmp->info.width *3;
	else
		row_width = bmp->info.width;
	while ((row_width & 3) != 0) row_width++;

	unsigned char* imgdata = (unsigned char*)bmp_data+bmp->header.offset; // not all bitmaps are created equal

	if (bmp->info.bitDepth == 8) // 8bpp bitmap (paletted)
	{
		u32 palette[256]; // max colors
		u32 colors, destPlace, destPal;
		if (bmp->info.numColors == 0) // assume 256 colors if 0 listed
			colors = 256;
		else
			colors = bmp->info.numColors;
		for(x = 0; x < colors; x++)
		{
//			palette[x] = (RGB15((bmp->colors[x].red >> 3), (bmp->colors[x].green >> 3), (bmp->colors[x].blue >> 3))| BIT(15));
			palette[x] = (bmp->colors[x].blue<<16) | (bmp->colors[x].green<<8) | bmp->colors[x].red;
		}
		//bmp stores the image upside down
		for(y = 0; y < bmp->info.height; y++)
		for(x = 0; x < bmp->info.width; x++)
		{
			destPlace = (bmp->info.height - 1 - y)*bmp->info.width + x;
			destPal = imgdata[(y*row_width) + x];
			dest[destPlace] = palette[destPal];
		}
	}
	if (bmp->info.bitDepth == 24) // BGR 24 bits per color - NOT ADJUSTED FOR PSP USE!!!
	{
		for(y = 0; y < bmp->info.height; y++)
		for(x = 0; x < bmp->info.width; x++)
		{
			u8 blue = imgdata[(y*row_width) + x*3];
			u8 green = imgdata[((y*row_width) + x*3)+1];
			u8 red = imgdata[((y*row_width) + x*3)+2];
			//dest[(bmp->info.height - 1 - y)*bmp->info.width + x] = RGB15(red, green, blue)| BIT(15);
			dest[x+y*PSP_SCREEN_WIDTH] =  0 | (blue<<16) | (green<<8) | red;
//			dest[(bmp->info.height - 1 - y)*bmp->info.width + x] = 0 | (blue<<16) | (green<<8) | red;
		}
	}

	return 0;
}

int loadBmpFromBuf(u8* image)
{
	u32 x, y;
	int ret = decodeBmp(image, 480, 272, (u32*)imgbuf);
	u32* dest = (u32*)bbuffer;
	u32* src = (u32*)imgbuf;
	for(x=0; x<480; x++) // blit the image into the backbuffer
	{
		for(y=0; y<272; y++)
		{
			dest[y*PSP_LINE_SIZE+x] = src[y*480+x];
		}
	}
	return ret;
}
// end BITMAP

