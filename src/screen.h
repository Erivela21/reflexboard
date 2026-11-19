/*
 * screen.h - text on the 128x64 LCD.
 *
 * With the 8x16 font the LCD fits 4 lines of 16 characters.
 */
#ifndef SCREEN_H
#define SCREEN_H

#define SCREEN_LINES 4
#define SCREEN_COLS  16

int  screen_init(void);
void screen_close(void);

/* Clear the LCD and show up to 4 centered lines (NULL = empty line).
 * A line starting with '-' is drawn as a horizontal rule. */
void screen_show(const char *l1, const char *l2, const char *l3, const char *l4);

#endif
