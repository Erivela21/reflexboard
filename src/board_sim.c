/*
 * board_sim.c - PC simulator for ReflexBoard.
 *
 * Lets you build and play the game on your computer (make sim) before
 * touching the board:
 *   - any key (Enter, space, ...) = a press of the USER BUTTON
 *   - the LED is printed in the terminal
 *   - every LCD frame can be saved as an image (REFLEX_LCD_DUMP=<folder>)
 *
 * REFLEX_SIM_AUTO plays the game by itself, which is used for testing and
 * for the screenshots in the tutorial. It is a comma-separated list, one
 * item per button press, consumed in order:
 *   S    press as soon as the LED is off (start / play again / false start)
 *   230  press 230 ms after the LED turns on
 *   T    don't press this time (the try times out)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include "board.h"
#include "LCD_Lib.h"

#define PRESS_LENGTH_MS 120.0   /* how long a simulated press is held */
#define AUTO_GAP_MS     400.0   /* scripted presses are at least this far apart */

static struct termios saved_tty;
static bool tty_raw = false;

static bool led_on = false;
static double led_on_at = 0;     /* when the LED was turned on */
static int led_cycles = 0;       /* how many times the LED has been turned on */
static int skip_after_cycle = -1;/* T token: wait for the LED to go on and off once */
static double press_until = 0;   /* simulated press is held until this time */

static char *auto_script = NULL; /* REFLEX_SIM_AUTO, consumed token by token */
static char *auto_next = NULL;
static bool  auto_done = false;

static const char *dump_dir = NULL;
static int dump_count = 0;

static double now_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

int board_init(void)
{
	const char *s = getenv("REFLEX_SIM_AUTO");

	if (s && *s) {
		auto_script = strdup(s);
		auto_next = auto_script;
	} else if (isatty(STDIN_FILENO)) {
		/* raw, non-blocking keyboard: one key = one button press */
		struct termios raw;
		tcgetattr(STDIN_FILENO, &saved_tty);
		raw = saved_tty;
		raw.c_lflag &= ~(ICANON | ECHO);   /* keep ISIG so Ctrl+C still works */
		raw.c_cc[VMIN] = 0;
		raw.c_cc[VTIME] = 0;
		tcsetattr(STDIN_FILENO, TCSANOW, &raw);
		tty_raw = true;
	}

	dump_dir = getenv("REFLEX_LCD_DUMP");

	printf("[sim] ReflexBoard simulator: any key = USER BUTTON, Ctrl+C = quit\n");
	return 0;
}

void board_close(void)
{
	led_on = false;
	if (tty_raw)
		tcsetattr(STDIN_FILENO, TCSANOW, &saved_tty);
	free(auto_script);
}

void board_led(bool on)
{
	if (on && !led_on) {
		printf("[sim]   >>> LED ON <<<\n");
		fflush(stdout);
		led_on_at = now_ms();
		led_cycles++;
	}
	led_on = on;
}

/* Decide whether the next scripted press happens now. */
static bool auto_press_now(double now)
{
	char token[16];
	size_t n;

	if (auto_done || auto_next == NULL || *auto_next == '\0') {
		auto_done = true;
		return false;
	}

	n = strcspn(auto_next, ",");
	if (n >= sizeof(token))
		n = sizeof(token) - 1;
	memcpy(token, auto_next, n);
	token[n] = '\0';

	if (token[0] == 'T') {
		/* skip this try: consume the token once the LED has gone off again */
		if (skip_after_cycle < 0)
			skip_after_cycle = led_cycles;
		if (led_on || led_cycles == skip_after_cycle)
			return false;
		skip_after_cycle = -1;
	} else if (token[0] == 'S') {
		if (led_on)
			return false;
	} else {
		/* number: press that many ms after the LED came on */
		if (!led_on || now - led_on_at < atof(token))
			return false;
	}

	auto_next += n;
	if (*auto_next == ',')
		auto_next++;
	return token[0] != 'T';
}

bool board_button(void)
{
	double now = now_ms();

	if (now < press_until)
		return true;

	if (auto_script) {
		if (auto_done) {
			/* script finished: behave like Ctrl+C */
			raise(SIGINT);
			return false;
		}
		if (now >= press_until + AUTO_GAP_MS && auto_press_now(now)) {
			press_until = now + PRESS_LENGTH_MS;
			return true;
		}
		return false;
	}

	if (tty_raw) {
		char c;
		if (read(STDIN_FILENO, &c, 1) == 1) {
			press_until = now + PRESS_LENGTH_MS;
			return true;
		}
	}
	return false;
}

/*
 * Replaces LCD_FrameCopy() from the Terasic library: instead of sending the
 * frame over SPI, save it as a PBM image (black & white, 128x64).
 * Frame layout: 8 pages of 128 bytes, each byte is a column of 8 pixels.
 */
void LCD_FrameCopy(uint8_t *Data)
{
	char path[512];
	FILE *f;
	int x, y;

	if (dump_dir == NULL)
		return;

	snprintf(path, sizeof(path), "%s/lcd-%03d.pbm", dump_dir, dump_count++);
	f = fopen(path, "w");
	if (f == NULL)
		return;

	fprintf(f, "P1\n%d %d\n", LCD_WIDTH, LCD_HEIGHT);
	for (y = 0; y < LCD_HEIGHT; y++) {
		for (x = 0; x < LCD_WIDTH; x++)
			fputc((Data[(y / 8) * LCD_WIDTH + x] >> (y % 8)) & 1 ? '1' : '0', f);
		fputc('\n', f);
	}
	fclose(f);
}
