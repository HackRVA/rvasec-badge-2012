/*
 * main.c
 *
 *  Created on: Jun 13, 2012
 *      Author: Luke Libraro - LukeL99@gmail.com
 */

#include <msp430.h>
#include "pcd8544_for_msp430.h"
#include "images.h"
#include "string.h"

// Interrupt status flags
char mode = 0;
char count = 0;
unsigned int cursor_index = 0;
char output[];
char blinky = 0;
char input_seq[] = { 5, 5, 5, 5, 5, 5, 5, 5 };
char elite = 0;

#define BLINK_CONST 1024
#define DEBOUNCE 1000
#define TEXT_INPUT_Y 1
#define PERM_IMAGE_MODE 0
#define TEXT_INPUT_MODE 1
#define TEMP_IMG_MODE 2
#define KONAMI_MODE 3
#define TEXT_MODE 4

int main(int argc, char **argv) {
	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer

	DCOCTL = CALDCO_1MHZ; // Set DCO to 16MHz
	BCSCTL1 = CALBC1_1MHZ; // MCLC = SMCLK = DCOCLK = 16MHz
	BCSCTL1 |= DIVA_0; // ACLK = ACLK/1

	BCSCTL3 = LFXT1S1; //Select VLOCLK for ACLCK (i.e. 12khz intosc)
	TACCTL0 = OUTMOD_2 + CCIE; // TACCR0 interrupt enabled
	TACTL = TASSEL_1 + ID1 + ID0 + MC_1; // ACLCK, 1/8 DIVIDER, upmode to TCCR0 value

	//Set port directions
	P1DIR = 0x3F; // bits 0, 1, 2, 3, 4, 5 to outputs
	P1OUT = 0x00; // All bits low to initialize
	//initialize buttons
	P2DIR &= ~0x0F; // Set P2.0-3 to input direction
	P2REN |= 0x0F; // Set P2.0-3 to use pull up/down resistor
	P2OUT &= ~0x0F; // Set P2.0-3 to use pull down resistor
	//configure interrupts for buttons 2.0-3
	P2IE = 0x0F; //Enable interrupts for each button
	P2IES = 0x00; //Set all ports to be sensitive only to rising edges

	nokia_init();

	TACCR0 = 0; //Compare to Timer_A register (approx. 25 sec for VLOCLK & 1/8 divider)
	draw_image(rvasec_logo);
	TACCR0 = BLINK_CONST; //Compare to Timer_A register (approx. 25 sec for VLOCLK & 1/8 divider)
	__bis_SR_register(LPM3_bits + GIE);
	//Enter LPM3 w/interrupt
	__no_operation(); // For debugger, executes next instruction just like a PIC
}

void perm_img_mode(const unsigned char* img) {
	wait(BLINK_CONST);
	mode = PERM_IMAGE_MODE;
	draw_image(img);
}

// prompt for text
void prompt() {
	int i;
	mode = TEXT_INPUT_MODE;
	nokia_ddram_clear();
	nokia_goto_cursor(0, 0);
	nokia_print_string("user@rvasec:~$");
	for (i = 0; i < 12; i++) {
		output[i] = ' ';
	}
	cursor_index = 0;
}

// 99 LIVES!
void konami_mode() {
	mode = KONAMI_MODE;
	nokia_ddram_clear();
	nokia_goto_cursor(13, 2);
	nokia_print_string("99 LIVES!");
}

// For displaying an image temporarily
void temp_img_mode(const unsigned char* img) {
	wait(BLINK_CONST);
	mode = TEMP_IMG_MODE;
	draw_image(img);
}

// For displaying text for a short time
void text_mode(const char* text) {
	wait(BLINK_CONST);
	mode = TEXT_MODE;
	nokia_print_string(text);
}

// Capture input for input sequences
void store_input(char button) {
	int i;
	for (i = 0; i < 7; i++) {
		input_seq[i] = input_seq[i + 1];
	}
	input_seq[7] = button;
	if (mode != TEXT_INPUT_MODE && input_seq[5] == 0 && input_seq[6] == 0
			&& input_seq[7] == 0) {
		prompt();
	} else if (input_seq[0] == 0 && input_seq[1] == 0 && input_seq[2] == 3
			&& input_seq[3] == 3 && input_seq[4] == 1 && input_seq[5] == 2
			&& input_seq[6] == 1 && input_seq[7] == 2) {
		konami_mode();
	}
}

#pragma vector=TIMER0_A0_VECTOR	// Timer A0 interrupt service routine
__interrupt void timer_a_interrupt(void) {
	TACCR0 = BLINK_CONST; // reset TACCR0 register
	if (elite) {
		P1OUT ^= nok_led; // Toggle LED using exclusive-OR
	}
	// Perm image display mode
	if (mode == PERM_IMAGE_MODE) {
		__no_operation();
	} else if (mode == TEXT_INPUT_MODE) { // text input mode
		if (blinky) {
			nokia_goto_cursor(cursor_index * 6, TEXT_INPUT_Y);
			nokia_print_char(output[cursor_index]);
			blinky = 0;
		} else {
			nokia_goto_cursor(cursor_index * 6, TEXT_INPUT_Y);
			nokia_print_char('_');
			blinky = 1;
		}
	} else if (mode == TEMP_IMG_MODE) { //temp image display mode
		if (count == 10) {
			mode = 1;
			count = 0;
			prompt();
		} else {
			count++;
		}
	} else if (mode == KONAMI_MODE) {
		if (count == 10) {
			mode = 1;
			count = 0;
			prompt();
		} else {
			P1OUT ^= nok_led; // Toggle LED using exclusive-OR
			count++;
		}
	} else if (mode == TEXT_MODE) {
		if (count == 10) {
			mode = 1;
			count = 0;
			prompt();
		} else {
			count++;
		}
	}
}

#pragma vector=PORT2_VECTOR	// Timer A0 interrupt service routine
__interrupt void port_2_interrupt(void) {
	wait(DEBOUNCE); //debounce
	// Clear port 2 interrupt requests
	P2IFG = 0x00;
	if (P2IN & 0x01) { //up
		store_input(0);
		if (mode == TEXT_INPUT_MODE) {
			if (output[cursor_index] == ' ') {
				output[cursor_index] = 'a';
			} else if (output[cursor_index] == 'z') {
				output[cursor_index] = '0';
			} else if ((output[cursor_index] >= '0'
					&& output[cursor_index] < '9')
					|| (output[cursor_index] >= 'a'
					&& output[cursor_index] < 'z')) {
				output[cursor_index]++;
			}
			nokia_goto_cursor(cursor_index * 6, TEXT_INPUT_Y);
			nokia_print_char(output[cursor_index]);
		}
	} else if (P2IN & 0x08) { //down
		store_input(3);
		if (mode == TEXT_INPUT_MODE) {
			if (output[cursor_index] == 'a') {
				output[cursor_index] = ' ';
			} else if (output[cursor_index] == '0') {
				output[cursor_index] = 'z';
			} else if (output[cursor_index] != ' ') {
				output[cursor_index]--;
			}
			nokia_goto_cursor(cursor_index * 6, TEXT_INPUT_Y);
			nokia_print_char(output[cursor_index]);
		}
	} else if (P2IN & 0x02) { //left
		store_input(1);
		if (mode == TEXT_INPUT_MODE && cursor_index > 0) {
			nokia_goto_cursor(cursor_index * 6, TEXT_INPUT_Y);
			nokia_print_char(output[cursor_index]);
			cursor_index--;
		}
	} else if (P2IN & 0x04) { //right
		store_input(2);
		if (mode == TEXT_INPUT_MODE && cursor_index < 10) {
			nokia_goto_cursor(cursor_index * 6, TEXT_INPUT_Y);
			nokia_print_char(output[cursor_index]);
			cursor_index++;
		}
	}
// check to see if any codes were entered
	if (mode == TEXT_INPUT_MODE) {
		if (!(strcmp(output, "hackrva     "))) {
			temp_img_mode(hackrva_logo);
		} else if (!(strcmp(output, "anonymous   "))) {
			perm_img_mode(anonymous);
		} else if (!(strcmp(output, "rvasec      "))) {
			temp_img_mode(rvasec_logo);
		} else if (!(strcmp(output, "babs        "))) {
			temp_img_mode(babs);
		} else if (!(strcmp(output, "31337       "))) {
			elite = 1;
			prompt();
		}
	}

}
