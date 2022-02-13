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

void oled_pixel_on(void){
    i2c_start(oled_addr);
    i2c_write(oled_send_data);
    i2c_write(0xFF);
    i2c_stop();
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
    for (uint8_t i = 0; i <= 127; i++){
        for (uint8_t j = 0; j <= 15; j++){
            oled_set_cursor(i, j);
            oled_pixel_on();
            _delay_ms(50);
        }
    }
}

int main(void){
    oled_init();
    while(1){
        oled_clear();
        _delay_ms(500);
        oled_light_up();
    }
}
