/*
 * board.c - real hardware: DE10-Standard HPS.
 *
 * The LED, the button and the LCD control pins all live in the same
 * peripheral, GPIO1, so a single mmap() of the HPS register area gives us
 * access to everything. This is the same technique used by Terasic's
 * hps_gpio example (see the course tutorial Tutorial-HPS-BlinkLED).
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"

#include "board.h"
#include "LCD_Hw.h"
#include "LCD_Lib.h"

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

/* GPIO1 bits (HPS_GPIO53 = LED, HPS_GPIO54 = button; GPIO1 starts at GPIO29) */
#define BIT_LED     (0x01000000)   /* bit 24 */
#define BIT_BUTTON  (0x02000000)   /* bit 25, active low */

/* Address of a register inside our mapping */
#define REG(addr) ( virtual_base + ( ( uint32_t )( addr ) & ( uint32_t )( HW_REGS_MASK ) ) )

static void *virtual_base = NULL;
static int fd = -1;

int board_init(void)
{
	if ( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\" (are you root?)\n" );
		return 1;
	}

	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
	if ( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return 1;
	}

	/* LED pin is an output; the button pin stays an input (reset default) */
	alt_setbits_word( REG( ALT_GPIO1_SWPORTA_DDR_ADDR ), BIT_LED );
	board_led( false );

	/* LCD: reset pulse, D/C pin, SPI master 0 (Terasic driver), then backlight */
	LCDHW_Init( virtual_base );
	LCDHW_BackLight( true );
	LCD_Init();

	return 0;
}

void board_close(void)
{
	if ( virtual_base == NULL )
		return;

	board_led( false );
	LCDHW_BackLight( false );

	munmap( virtual_base, HW_REGS_SPAN );
	close( fd );
	virtual_base = NULL;
}

void board_led(bool on)
{
	if ( on )
		alt_setbits_word( REG( ALT_GPIO1_SWPORTA_DR_ADDR ), BIT_LED );
	else
		alt_clrbits_word( REG( ALT_GPIO1_SWPORTA_DR_ADDR ), BIT_LED );
}

bool board_button(void)
{
	uint32_t scan_input = alt_read_word( REG( ALT_GPIO1_EXT_PORTA_ADDR ) );

	/* The button pulls the pin to 0 when pressed */
	return ( ~scan_input & BIT_BUTTON ) != 0;
}
