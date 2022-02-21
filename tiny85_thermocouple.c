#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

//pins
#define sda PB0
#define scl PB2

//macros
#define sda_high() DDRB &= ~(1<<sda); //release sda, pulled high by oled chip
#define sda_low() DDRB |= (1<<sda); //pulled low by tiny
#define scl_high() DDRB &= ~(1<<scl);
#define scl_low() DDRB |= (1<<scl);

//init
void i2c_init(void) {
  DDRB  &= ~((1<<sda)|(1<<scl));  //release both lines
  PORTB &= ~((1<<sda)|(1<<scl));  //both low
}

//data transfer
void i2c_write(uint8_t dat){
    for (uint8_t i = 8; i; i--){
        sda_low();
        if (dat & 0b10000000) sda_high(); //check if msb == 1, if so sda high
        scl_high();
        dat = dat << 1;
        scl_low();
    }
    sda_high();
    scl_high();
    asm("nop"); 
    scl_low();
}

//start condition
void i2c_start(uint8_t addr){
    sda_low();
    scl_low();
    i2c_write(addr);
}

//stop condition
void i2c_stop(){
    sda_low();
    scl_high();
    sda_high();
}

/* communication with oled driver starts with i2c start condition -> 7bit slave address -> r/w bit -> ack bit is send by slave ->
    -> control byte (C0 | DC | 0 | 0 | 0 | 0 | 0 | 0) -> ack bit 

    if C0 = 0, only data is send
    if DC = 0, sent data is for command operation
    if DC = 1, data byte is for RAM operation

*/

#define oled_addr 0x78 // i2c address is 0x3C, we're adding rw bit == 0 (0x3C << 1 = 0x78)
#define oled_send_cmd 0x00 // command mode
#define oled_send_data 0x40 // data mode

const uint8_t oled_init_commands[] PROGMEM = {                  //init cmds stored in FLASH
    0xAE,             // oled off
    0x20, 0x00,       // set horizontal memory addressing mode, POR (whatever it is) = 0x00
    0xA8, 0x7F,       // set multiplex 0x7F for 128x128px display (0x3F displays half a screen (64px), 0x1F displays 32px)
    0xA4,             // A4 - normal, A5 - every pixel on display on regardles of the display data RAM
    0xAF,             // switch on OLED
};

const uint8_t oled_font[] PROGMEM = {
  0x3E, 0x51, 0x49, 0x45, 0x3E, // 0 16
  0x00, 0x42, 0x7F, 0x40, 0x00, // 1 17
  0x42, 0x61, 0x51, 0x49, 0x46, // 2 18
  0x21, 0x41, 0x45, 0x4B, 0x31, // 3 19
  0x18, 0x14, 0x12, 0x7F, 0x10, // 4 20
  0x27, 0x45, 0x45, 0x45, 0x39, // 5 21
  0x3C, 0x4A, 0x49, 0x49, 0x30, // 6 22
  0x01, 0x71, 0x09, 0x05, 0x03, // 7 23
  0x36, 0x49, 0x49, 0x49, 0x36, // 8 24
  0x06, 0x49, 0x49, 0x29, 0x1E, // 9 25
};

void oled_init(void){
    i2c_init();
    i2c_start(oled_addr);
    i2c_write(oled_send_cmd);
    for (uint8_t i = 0; i < sizeof(oled_init_commands); i++) i2c_write(pgm_read_byte(&oled_init_commands[i]));
    i2c_stop();
}

void oled_set_cursor(uint8_t x, uint8_t y){     // x in range 0 - 127, y in range 0 - 15
    i2c_start(oled_addr);
    i2c_write(oled_send_cmd);
    i2c_write(x & 0b00001111);           // set lower col adress (last four bits of 0b00000000, 00H - 0FH (0b00001111))
    i2c_write(0b00010000 | (x >> 4));    // set higher col adress (last 3 bits of 0b00010000, 10H - 17H)
    i2c_write(0xB0 | y);                     // set page address (last 4 bits of 0b10110000, B0H - BFH, 15 pages)
    i2c_stop();
}

void oled_print(char char){
    uint16_t char_index = char - 32
    i2c_write(0x00); 
    for (uint8_t i = 0; i < 5; i++) i2c_write(pgm_read_byte(&oled_font[i]));
        progmem_byte = pgm_read_byte(&oled_font[i]);
}

void oled_pixel_off(void){
    i2c_start(oled_addr);
    i2c_write(oled_send_data);
    i2c_write(0x00);
    i2c_stop();
}


void oled_clear(void){
    for (uint8_t i = 0; i <= 127; i++){
        for (uint8_t j = 0; j <= 15; j++){
            oled_set_cursor(i, j);
            oled_pixel_off();
        }
    }
}

void oled_light_up(void){
    for (uint8_t j = 0; j <= 15; j++){
        _delay_ms(500);
        for (uint8_t i = 0; i <= 127; i++){
            oled_set_cursor(i, j);
            oled_pixel_on();
        }
    }
}

void oled_print(void){

}

int main(void){
    oled_init();
    oled_clear();
    oled_set_cursor(0,0);
    oled_print();
}
