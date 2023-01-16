#ifndef I2C_H  
#define I2C_H

//pins
#define sda PB0
#define scl PB2

//macros
#define sda_high() DDRB &= ~(1<<sda);                                //release sda, pulled high by oled chip
#define sda_low() DDRB |= (1<<sda);                                  //pulled low by tiny
#define scl_high() DDRB &= ~(1<<scl);
#define scl_low() DDRB |= (1<<scl);

void i2c_init(void);            //init protocol
void i2c_write(uint8_t);        //send byte
void i2c_start(uint8_t);        //start condition
void i2c_stop(void);            //stop condition
#endif