#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "i2c.h"
#include "oled.h"


unsigned int readings_internal(){
    ADMUX = 0b10001111;                                         // 1.1v ref, internal temp sensor
    ADCSRA = 0b11000000;                                        // ADC enable, ADC start conversion, prescaler div factor 2
    while(!(ADCSRA & (1 << ADIF)));                             //wait for adc to be ready
        ADCSRA |= (1 << ADIF);
        return ADC;
    //MCUCR = 0b10101000;                                       //sleep enable, adc noise reduction mode
    //__asm__ __volatile__ ( "sleep" :: );                      //put cpu to sleep
}


unsigned int readings_thermocouple(){
    ADMUX = 0b10000111;                                         // 1.1v ref, 20x adc gain pb3 pb4
    ADCSRA = 0b11000000;                                        // ADC enable, ADC start convertion, prescaler div factor 2
    while(!(ADCSRA & (1 << ADIF)));                             //wait for adc to be ready
        ADCSRA |= (1 << ADIF);
        return ADC;
}

int main(void){
    oled_init();                                                //init display
    oled_clear();                                               //clear display memory

    while(1){
    float internal_temp = 0;
    readings_internal();
    for(uint8_t i=0;i<32;i++) internal_temp = internal_temp + readings_internal();       //oversampling
    internal_temp = internal_temp / 32;
    internal_temp = 0.8929 * internal_temp - 258.52;                                     //interpolation with offset correction
    internal_temp = (int)internal_temp;                                                  //get rid of decimals
    oled_print_big_char(internal_temp,0,0);                                                  

    float thermocouple_temp = 0;
    readings_thermocouple();
    for(uint8_t i=0;i<32;i++) thermocouple_temp = thermocouple_temp + readings_thermocouple();
    thermocouple_temp = thermocouple_temp / 32;
    thermocouple_temp = (int)thermocouple_temp;
    oled_print_big_char(thermocouple_temp, 0, 8);
    _delay_ms(1000);

    } 
}



