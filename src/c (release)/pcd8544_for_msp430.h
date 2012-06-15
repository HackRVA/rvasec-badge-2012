#include "msp430g2402.h"
/*				
 * Nokia 3310 PCD8544 LCD Commands
 * ================================
 * nokia_init(); Initializes the LCD
 * nokia_ddram_clear(); clears the DDRAM
 * nokia_send_cmd(0x20); Instructs LCD with commands
 * nokia_write_data(0x01); Instructs LCD with data
 * nokia_send_byte(0xFE); Sends 0xFE to the LCD
 * nokia_goto_cursor(x,y); Moves LCD cursor from (0,0) to (83.5,47.5)
 * nokia_contrast(0x0F); Sets Vop contrast mode
 * nokia_print_string("sample text"); Prints 'sample text' on LCD
 * nokia_print_char('E'); Prints individual character on LCD
 * lcdpixel (x,y); Sets LCD pixel at that coordinate
 *					
*/
#define nok_reset BIT0 //LCD Reset pin P1.0
#define nok_dc	 BIT1  //LCD D/C pin   P1.1
#define nok_sdin BIT2  //LCD MOSI pin  P1.2
#define nok_sclk BIT3  //LCD SCLK pin  P1.3
#define nok_led BIT4 //LCD Backlight pin P1.4
#define GLCD_SETYADDR	0x40
extern void nokia_init(void);
extern void nokia_ddram_clear(void);
extern void nokia_send_cmd(char);
extern void nokia_write_data(char);
extern void nokia_send_byte(char );
extern void nokia_goto_cursor(char, char);
extern void nokia_contrast(char);
extern void nokia_print_string(const char*);
extern void nokia_print_char(char);
extern void lcdpixel (char, char) ;
extern void wait(unsigned int);
extern void draw_image(const unsigned char*);
