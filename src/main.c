/*
 * ReflexBoard - a reaction-time trainer for the DE10-Standard (HPS / Linux).
 *
 * One round = 5 tries. In each try the USER LED lights up after a random
 * wait of 1 to 4 seconds, and you press the USER BUTTON as fast as you can.
 *   - pressing before the LED lights up is a FALSE START
 *   - not pressing within 2 seconds is TOO SLOW
 * At the end you get your best time, your average and a rating.
 *
 * Everything is shown on the board's LCD and printed in the terminal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "board.h"
#include "screen.h"

#define TRIES          5
#define WAIT_MIN_MS    1000    /* random wait before the LED: 1..4 s */
#define WAIT_MAX_MS    4000
#define TIMEOUT_MS     2000    /* no press within 2 s = too slow */
#define DEBOUNCE_MS    30
#define SHOW_RESULT_MS 1500    /* time each result stays on screen */

enum outcome { VALID, FALSE_START, TOO_SLOW };

struct attempt {
	enum outcome outcome;
	double ms;              /* reaction time, only for VALID */
};

static volatile sig_atomic_t quit = 0;

static void on_signal(int sig)
{
	(void)sig;
	quit = 1;
}

/* Milliseconds from a clock that never jumps (unlike the time of day). */
static double now_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void sleep_ms(int ms)
{
	double end = now_ms() + ms;
	while (!quit && now_ms() < end)
		usleep(1000);
}

/* Wait until the button is pressed. Returns 0 if we were asked to quit. */
static int wait_press(void)
{
	while (!quit && !board_button())
		usleep(5000);
	return !quit;
}

/* Wait until the button is released (and stops bouncing). */
static void wait_release(void)
{
	while (!quit && board_button())
		usleep(5000);
	sleep_ms(DEBOUNCE_MS);
}

static const char *rating(double ms)
{
	if (ms < 200) return "WORLD CLASS";
	if (ms < 250) return "PRO";
	if (ms < 300) return "GOOD";
	if (ms < 400) return "AVERAGE";
	return "KEEP TRAINING";
}

static struct attempt play_try(int n)
{
	struct attempt a = { TOO_SLOW, 0 };
	char title[17], line[17];
	double t0, t1, go_at;

	snprintf(title, sizeof(title), "TRY %d/%d", n, TRIES);
	printf("Try %d/%d: get ready...\n", n, TRIES);
	fflush(stdout);
	screen_show(title, "-", "GET READY...", "wait for LED");

	/* Random wait: pressing during it is a false start */
	go_at = now_ms() + WAIT_MIN_MS + rand() % (WAIT_MAX_MS - WAIT_MIN_MS + 1);
	while (!quit && now_ms() < go_at) {
		if (board_button()) {
			a.outcome = FALSE_START;
			printf("Try %d/%d: FALSE START!\n", n, TRIES);
			screen_show(title, "-", "FALSE START!", "too early");
			wait_release();
			sleep_ms(SHOW_RESULT_MS);
			return a;
		}
		usleep(1000);
	}
	if (quit)
		return a;

	/*
	 * GO! Turn the LED on and start the clock. From here until the press we
	 * only read the button register in a tight loop: no LCD refresh, no
	 * printf, no sleep, so the measurement is as precise as possible.
	 */
	board_led(true);
	t0 = now_ms();
	do {
		t1 = now_ms();
	} while (!board_button() && t1 - t0 < TIMEOUT_MS && !quit);
	board_led(false);

	if (quit)
		return a;

	if (t1 - t0 >= TIMEOUT_MS) {
		printf("Try %d/%d: TOO SLOW!\n", n, TRIES);
		screen_show(title, "-", "TOO SLOW!", "no press in 2 s");
	} else {
		a.outcome = VALID;
		a.ms = t1 - t0;
		printf("Try %d/%d: %6.1f ms  %s\n", n, TRIES, a.ms, rating(a.ms));
		snprintf(line, sizeof(line), "%.0f ms", a.ms);
		screen_show(title, "-", line, rating(a.ms));
		wait_release();
	}
	fflush(stdout);
	sleep_ms(SHOW_RESULT_MS);
	return a;
}

static void show_summary(const struct attempt *tries)
{
	double best = 0, sum = 0;
	int valid = 0, false_starts = 0, too_slow = 0, i;
	char l1[17], l2[17], l3[17];

	for (i = 0; i < TRIES; i++) {
		if (tries[i].outcome == VALID) {
			if (valid == 0 || tries[i].ms < best)
				best = tries[i].ms;
			sum += tries[i].ms;
			valid++;
		} else if (tries[i].outcome == FALSE_START) {
			false_starts++;
		} else {
			too_slow++;
		}
	}

	printf("---------- RESULTS ----------\n");
	if (valid > 0) {
		printf("Best        : %6.1f ms\n", best);
		printf("Average     : %6.1f ms (%d valid tries)\n", sum / valid, valid);
		printf("Rating      : %s\n", rating(sum / valid));
		snprintf(l1, sizeof(l1), "BEST %4.0f ms", best);
		snprintf(l2, sizeof(l2), "AVG  %4.0f ms", sum / valid);
	} else {
		printf("No valid tries.\n");
		snprintf(l1, sizeof(l1), "NO VALID TRIES");
		l2[0] = '\0';
	}
	printf("False starts: %d\n", false_starts);
	printf("Too slow    : %d\n", too_slow);
	printf("-----------------------------\n");
	printf("Press USER BUTTON to play again (Ctrl+C to quit)\n");
	fflush(stdout);

	snprintf(l3, sizeof(l3), "FOULS %d", false_starts + too_slow);
	screen_show(l1, l2, l3, valid > 0 ? rating(sum / valid) : "TRY AGAIN");
}

int main(void)
{
	struct sigaction sa;
	struct attempt tries[TRIES];
	const char *seed = getenv("REFLEX_SEED");
	int i;

	/* Ctrl+C (or ssh closing) ends the game cleanly: LED off, LCD cleared */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	srand(seed ? (unsigned)atoi(seed) : (unsigned)time(NULL) ^ (unsigned)getpid());

	if (board_init() != 0)
		return 1;
	if (screen_init() != 0) {
		board_close();
		return 1;
	}

	printf("=========== ReflexBoard ===========\n");
	printf("Press USER BUTTON to start (Ctrl+C to quit)\n");
	fflush(stdout);
	screen_show("REFLEXBOARD", "-", "press USER", "BUTTON to start");

	while (wait_press()) {
		wait_release();

		for (i = 0; i < TRIES && !quit; i++)
			tries[i] = play_try(i + 1);
		if (quit)
			break;

		show_summary(tries);
	}

	printf("\nBye!\n");
	screen_close();
	board_close();
	return 0;
}
