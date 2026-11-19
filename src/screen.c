/*
 * screen.c - draws text on the LCD with Terasic's graphic library.
 *
 * Drawing happens in a frame buffer in RAM (LCD_CANVAS). DRAW_Refresh()
 * then copies the whole frame (1 KB) to the LCD over SPI. That copy takes a
 * few milliseconds, which is why main.c never refreshes the LCD while it is
 * timing a reaction.
 */
#include <stdlib.h>
#include <string.h>

#include "hwlib.h"
#include "LCD_Lib.h"
#include "lcd_graphic.h"
#include "font.h"
#include "screen.h"

#define LINE_HEIGHT 16

static LCD_CANVAS canvas;

int screen_init(void)
{
	canvas.Width = LCD_WIDTH;
	canvas.Height = LCD_HEIGHT;
	canvas.BitPerPixel = 1;
	canvas.FrameSize = canvas.Width * canvas.Height / 8;
	canvas.pFrame = malloc(canvas.FrameSize);
	if (canvas.pFrame == NULL)
		return 1;

	DRAW_Clear(&canvas, LCD_WHITE);
	DRAW_Refresh(&canvas);
	return 0;
}

void screen_close(void)
{
	if (canvas.pFrame == NULL)
		return;
	DRAW_Clear(&canvas, LCD_WHITE);
	DRAW_Refresh(&canvas);
	free(canvas.pFrame);
	canvas.pFrame = NULL;
}

static void draw_line(int row, const char *text)
{
	char buf[SCREEN_COLS + 1];
	int len, x, y = row * LINE_HEIGHT;

	if (text == NULL || text[0] == '\0')
		return;

	if (text[0] == '-') {
		/* horizontal rule in the middle of the row */
		DRAW_Line(&canvas, 4, y + LINE_HEIGHT / 2, LCD_WIDTH - 4, y + LINE_HEIGHT / 2, LCD_BLACK);
		return;
	}

	strncpy(buf, text, SCREEN_COLS);
	buf[SCREEN_COLS] = '\0';
	len = strlen(buf);
	x = (LCD_WIDTH - len * font_16x16.FontWidth) / 2;
	DRAW_PrintString(&canvas, x, y, buf, LCD_BLACK, &font_16x16);
}

void screen_show(const char *l1, const char *l2, const char *l3, const char *l4)
{
	if (canvas.pFrame == NULL)
		return;

	DRAW_Clear(&canvas, LCD_WHITE);
	draw_line(0, l1);
	draw_line(1, l2);
	draw_line(2, l3);
	draw_line(3, l4);
	DRAW_Refresh(&canvas);
}
