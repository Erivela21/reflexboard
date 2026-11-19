/*
 * board.h - everything ReflexBoard needs from the hardware.
 *
 * Two implementations exist:
 *   board.c      - the real DE10-Standard (HPS GPIO1 + LCD over SPIM0, via /dev/mem)
 *   board_sim.c  - a PC simulator (keyboard = button, terminal = LED, LCD frames saved as images)
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

/* Map the hardware, set up the LED/button pins and switch the LCD on.
 * Returns 0 on success. */
int  board_init(void);

/* Turn the LED and the LCD backlight off and release the hardware. */
void board_close(void);

/* HPS USER LED (GPIO1 bit 24). */
void board_led(bool on);

/* HPS USER BUTTON (GPIO1 bit 25): true while the button is held down. */
bool board_button(void);

#endif
