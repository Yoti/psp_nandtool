/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 *  pspdebug.h - Prototypes for the pspDebug library
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 *
 * $Id: pspdebug.h 2319 2007-09-30 15:58:31Z tyranid $
 */
#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <psptypes.h>
#include <pspmoduleinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

enum // special characters
{
	RO_LEFT = 0,
	RO_RIGHT,
	U_ARROW,
	R_ARROW,
	D_ARROW,
	L_ARROW,
	SQUARE,
	CIRCLE,
	TRIANGLE
};

/**
 * draws a box, x/y coords are pixels not char
 * topx /topy = top left pixel of the box
 * botx/boty = bottom right pixel of the box 
 */
void drawBox(int topx, int topy, int botx, int boty, u32 color);

// draws a line from x to y with a pixel length of len, coords are pixels
void drawLine(int x, int y, int len, u32 color);

/**
 * text coords, set the are that will roll up when a line wrap or \n occurs
 * xmin/ymin = the setxy() of the top left character of the area (this is not pixels)
 * xmax/ymax = the setxy() of the bottome right character of the area (again not pixels)
 * ~ also sets x/y to xmin/ymin to ensure it is within the area
*/
void enableRollUpArea(int xmin, int ymin, int xmax, int ymax);

// returns settings to full screen settings, probably should never be called externally
void resetRollUpArea(void);

// screen is set up to be 68 wide and 34 tall
// sets roll area and draws a box (color) around it with 4 px spacing
void drawTextBox(int topx, int topy, int width, int height, u32 color);
// sets roll area and draws a box (color) around it with 4 px spacing, fading background into a color by a degree of 0-31 , saving previous screen contents to fade back out to
// lineColor = outline; intColor = bg color for text in box; fadeColor = color to fade rest of screen to; fade = degree of fade to color 0-31
void showPopupBox(int topx, int topy, int width, int height, u32 lineColor, u32 intColor, u32 fadeColor, u8 fade);
// restores previous screen contents when box is faded in
void hidePopupBox(void);
//  clears the text area of the box to bg color, works for popup boxes as well.
void clearBoxChars(void);
// clears the full area inside the box bounds, can set color by setting background color
void clearBox(void);

void showPercentBar(int orgx, int orgy, u32 boxcolor, u32 bgcolor, u32 fgcolor, u32 txcolor);
void updatePercent(int top, int bottom);
void removeBound();
void restoreBound();
void putSChar(int ch, int x, int y);
void myScreenSwapBuffer(void);
void fadeTo(u32 color, int comp);
void fadeIn(void);
void restoreFade(void);

// loads an 8 bit bitmap onto the current front buffer, must be full screen 480x272
int loadBmpFromBuf(u8* image);

/** 
  * put special char (hand made glyphs)
  */void myScreenPutSChar(int ch);

/** 
  * Initialise the debug screen
  */
void myScreenInit(void);

/**
  * Do a printf to the debug screen.
  *
  * @param fmt - Format string to print
  * @param ... - Arguments
  */
void myScreenPrintf(const char *fmt, ...) __attribute__((format(printf,1,2)));

/**
  * Do a printf to the debug screen.
  * @note This is for kernel mode only as it uses a kernel function
  * to perform the printf instead of using vsnprintf, use normal printf for
  * user mode.
  *
  * @param fmt - Format string to print
  * @param ... - Arguments
  */
void myScreenKprintf(const char *format, ...) __attribute__((format(printf,1,2)));

/**
 * Enable or disable background colour writing (defaults to enabled)
 * 
 * @param enable - Set 1 to to enable background color, 0 for disable
 */
void myScreenEnableBackColor(int enable);

/** 
  * Set the background color for the text
  * @note To reset the entire screens bg colour you need to call myScreenClear
  *
  * @param color - A 32bit RGB colour
  */
void myScreenSetBackColor(u32 color);

/**
  * Set the text color 
  *
  * @param color - A 32 bit RGB color
  */
void myScreenSetTextColor(u32 color);

/**
 * Set the color mode (you must have switched the frame buffer appropriately)
 */
//void myScreenSetColorMode(void);

/** 
  * Draw a single character to the screen.
  *
  * @param x - The x co-ordinate to draw to (pixel units)
  * @param y - The y co-ordinate to draw to (pixel units)
  * @param color - The text color to draw
  * @param ch - The character to draw
  */
void myScreenPutChar(int x, int y, u32 color, u8 ch);

/**
  * Set the current X and Y co-ordinate for the screen (in character units)
  */
void myScreenSetXY(int x, int y);

/**
  * Set the video ram offset used for the screen
  *
  * @param offset - Offset in bytes
  */
void myScreenSetOffset(int offset);

/** 
  * Get the current X co-ordinate (in character units)
  *
  * @return The X co-ordinate
  */
int myScreenGetX(void);

/** 
  * Get the current Y co-ordinate (in character units)
  *
  * @return The Y co-ordinate
  */
int myScreenGetY(void);

/**
  * Clear the debug screen.
  */
void myScreenClear(void);

/**
  * Print non-nul terminated strings.
  * 
  * @param buff - Buffer containing the text.
  * @param size - Size of the data
  *
  * @return The number of characters written
  */
int myScreenPrintData(const char *buff, int size);

/**
 * Print a string
 * @param str - String
 * @return The number of characters written
 */
int myScreenPuts(const char *str);

/** Type for the debug print handlers */
//typedef int (*PspDebugPrintHandler)(const char *data, int len);

/** Type for the debug input handler */
//typedef int (*PspDebugInputHandler)(char *data, int len);


#ifdef __cplusplus
}
#endif

#endif // __GRAPHICS_H__
