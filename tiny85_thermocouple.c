#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

//pins
#define sda PB0
#define scl PB2

//macros
#define sda_high() DDRB &= ~(1<<sda); //release sda, pulled high by oled chip
#define sda_low() DDRB |= (1<<sda); //pulled low by tiny
#define scl_high() DDRB &= ~(1<<sda);
#define scl_low() DDRB |= (1<<scl);

//init
void i2c_init(void) {
  DDRB  &= ~((1<<sda)|(1<<scl));  //release both lines
  PORTB &= ~((1<<sda)|(1<<scl));  //both low
}s

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

//data transfer
void i2c_write(uint8_t dat){
    for (uint8_t i = 8; i; i--){
        sda_low();
        if ((dat << i) & 0b10000000) sda_high(); //check if msb == 1, if so sda high
        scl_high();
        dat = dat << 1;
        scl_low();
    }
    sda_high();
    scl_high();
    asm("nop"); 
    scl_low();
}

/* communication with oled driver starts with i2c start condition -> 7bit slave address -> r/w bit -> ack bit is send by slave ->
    -> control byte (C0 | DC | 0 | 0 | 0 | 0 | 0 | 0) -> ack bit 

    if C0 = 0, only data is send
    if DC = 0, sent data is for command operation
    if DC = 1, data byte is for RAM operation

*/

#define oled_addr 0b00111100 // 8bit, since r/w bit is included and always will be 0 -> read only
#define oled_send_cmd 0b00000000 // command mode
#define oled_send_data 0b01000000 // data mode
#define oled_init_commands_len 6 // no. of commands to send

const uint8_t oled_init_commands[] PROGMEM = { //init cmds stored in FLASH
    0xA8, 0x7F, // multiplexig ration
    0x20, 0x00, // page addressing mode
    0xA7,
    0xAF        // entire display on
};

void oled_init(void){
    i2c_init();
    i2c_start(oled_addr);
    i2c_write(oled_send_cmd);
    for (uint8_t i = 0; i < oled_init_commands_len; i++) i2c_write(pgm_read_byte(&oled_init_commands[i]));
    i2c_stop();
}

int main(void){
    oled_init();
}
