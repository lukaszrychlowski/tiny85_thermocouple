#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

//////// I2C ///////////////////////////////////////////////

//pins
#define sda PB0
#define scl PB2

//macros
#define sda_high() DDRB &= ~(1<<sda);                                //release sda, pulled high by oled chip
#define sda_low() DDRB |= (1<<sda);                                  //pulled low by tiny
#define scl_high() DDRB &= ~(1<<scl);
#define scl_low() DDRB |= (1<<scl);

//init
void i2c_init(void) {
  DDRB  &= ~((1<<sda)|(1<<scl));                                     //release both lines
  PORTB &= ~((1<<sda)|(1<<scl));                                     //both low
}

//data transfer
void i2c_write(uint8_t dat){
    for (uint8_t i = 8; i; i--){
        sda_low();
        if (dat & 0b10000000) sda_high();                            //check if msb == 1, if so sda high
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

//////// OlED ///////////////////////////////////////////////

/* communication with oled driver starts with i2c start condition -> 7bit slave address -> r/w bit -> ack bit is send by slave ->
    -> control byte (C0 | DC | 0 | 0 | 0 | 0 | 0 | 0) -> ack bit 

    if C0 = 0, only data is send
    if DC = 0, sent data is for command operation
    if DC = 1, data byte is for RAM operation

*/

#define oled_addr 0x78                                          // i2c address is 0x3C, we're adding rw bit = 0 (0x3C << 1 = 0x78)
#define oled_send_cmd 0x00                                      // oled command mode
#define oled_send_data 0x40                                     // oled data mode

const uint8_t oled_init_commands[] PROGMEM = {                  //init cmds stored in FLASH
    0xAE,                                                       // oled off
    0x20, 0x00,                                                 // set horizontal memory addressing mode, POR (whatever it is) = 0x00
    0xA8, 0x7F,                                                 // set multiplex 0x7F for 128x128px display (0x3F displays half a screen (64px), 0x1F displays 32px)
    0xA4,                                                       // A4 - normal, A5 - every pixel on display on regardles of the display data RAM
    0xAF,                                                       // switch on OLED
};

const uint8_t oled_font[][6] PROGMEM = {
{ 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 },                         //0
{ 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 },                         //1
{ 0x72, 0x49, 0x49, 0x49, 0x46, 0x00 },                         //2
{ 0x21, 0x41, 0x49, 0x4D, 0x33, 0x00 },                         //3
{ 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 },                         //4
{ 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 },                         //5
{ 0x3C, 0x4A, 0x49, 0x49, 0x31, 0x00 },                         //6
{ 0x41, 0x21, 0x11, 0x09, 0x07, 0x00 },                         //7
{ 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 },                         //8
{ 0x46, 0x49, 0x49, 0x29, 0x1E, 0x00 },                         //9
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },                         //10 Space
{ 0x00, 0x06, 0x09, 0x06, 0x00, 0x00 },                         //11 degree symbol = '`'
{ 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 },                         //12 C
};
const int space = 10;
const int degree = 11;
const int centigrade = 12;

void oled_init(void){
    i2c_init();
    i2c_start(oled_addr);
    i2c_write(oled_send_cmd);
    for (uint8_t i = 0; i < sizeof(oled_init_commands); i++) i2c_write(pgm_read_byte(&oled_init_commands[i]));
    i2c_stop();
}

void oled_set_cursor(uint8_t x, uint8_t y){                     // x in range 0 - 127, y in range 0 - 15
    i2c_start(oled_addr);
    i2c_write(oled_send_cmd);
    i2c_write(x & 0b00001111);                                  // set lower col adress (last four bits of 0b00000000, 00H - 0FH (0b00001111))
    i2c_write(0b00010000 | (x >> 4));                           // set higher col adress (last 3 bits of 0b00010000, 10H - 17H)
    i2c_write(0xB0 | y);                                        // set page address (last 4 bits of 0b10110000, B0H - BFH, 15 pages)
    i2c_stop();
}

void oled_pixel_off(void){
    i2c_start(oled_addr);
    i2c_write(oled_send_data);
    i2c_write(0x00);
    i2c_stop();
}

void oled_pixel_on(void){
    i2c_start(oled_addr);
    i2c_write(oled_send_data);
    i2c_write(0xFF);
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

void oled_print_char(int c){
    i2c_start(oled_addr);
    i2c_write(oled_send_data);
    for (uint8_t column = 0; column < 6; column++){             //for each byte making the digit
        int bits = pgm_read_byte(&oled_font[c][column]);        //read corresponding bits
        i2c_write(bits);                                        //send bits to oled
    }
    i2c_stop();
}

uint8_t get_digit(unsigned int val, unsigned int div){      
    /* assume val = 1234
    div = 1000 -> val/div % 10 = 1.234 -> int 1                          
    div = 100  -> val/div % 10 = 2.34 -> int 2
    div = 10   -> val/div % 10 = 3.4 -> int 3                             
    div = 1    -> val/div % 10 = 4 -> int 4
    */                
    return (val/div) % 10;       
}

void oled_print_temp(int temp, int col, int line){
    for (int i = 1000; i > 0; i = i / 10){
        oled_set_cursor(col, line);
        char digit = get_digit(temp, i);
        oled_print_char(digit);
        col = col + 8;
    }
    oled_set_cursor(col, line);
    oled_print_char(degree);
    col = col + 8;
    oled_set_cursor(col, line);
    oled_print_char(centigrade);
}

//////// READINGS ///////////////////////////////////////////////


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
    for(uint8_t i=0;i<99;i++) internal_temp = internal_temp + readings_internal();       //oversampling overkill
    internal_temp = internal_temp / 99;
    internal_temp = 0.8929 * internal_temp - 258.52;                                     //interpolation
    internal_temp = (int)internal_temp;                                                  //get rid of decimals
    oled_print_temp(internal_temp,0,0);                                                  

    float thermocouple_temp = 0;
    readings_thermocouple();
    for(uint8_t i=0;i<99;i++) thermocouple_temp = thermocouple_temp + readings_thermocouple();
    thermocouple_temp = thermocouple_temp / 99;
    thermocouple_temp = (int)thermocouple_temp;
    oled_print_temp(thermocouple_temp, 0, 5);


    } 
}



