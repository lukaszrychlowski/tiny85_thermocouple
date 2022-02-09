#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

//pins
#define sda PB0
#define scl PB2

//macros
#define sda_high DDRB &= ~(1<<sda); //release sda, pulled high by oled chip
#define sda_low DDRB |= (1<<sda); //pulled low by tiny
#define scl_high DDRB &= ~(1<<sda);
#define scl_low DDRB |= (1<<scl);

//init
void i2c_init(void) {
  DDRB  &= ~((1<<sda)|(1<<scl));  //release both lines
  PORTB &= ~((1<<sda)|(1<<scl));  //both low
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

//data transfer
void i2c_write(uint8_t dat){
    sda_low();
    for (uint8_t i=8;i;i--){
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
