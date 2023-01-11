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

//////// OlED ///////////////////////////////////////////////

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

const uint8_t oled_font[][6] PROGMEM = {
{ 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 }, //0
{ 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 }, //1
{ 0x72, 0x49, 0x49, 0x49, 0x46, 0x00 }, //2
{ 0x21, 0x41, 0x49, 0x4D, 0x33, 0x00 }, //3
{ 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 }, //4
{ 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 }, //5
{ 0x3C, 0x4A, 0x49, 0x49, 0x31, 0x00 }, //6
{ 0x41, 0x21, 0x11, 0x09, 0x07, 0x00 }, //7
{ 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 }, //8
{ 0x46, 0x49, 0x49, 0x29, 0x1E, 0x00 }, //9
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //10 Space
{ 0x00, 0x06, 0x09, 0x06, 0x00, 0x00 }, //11 degree symbol = '`'
{ 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 }, //12 C
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

void oled_set_cursor(uint8_t x, uint8_t y){     // x in range 0 - 127, y in range 0 - 15
    i2c_start(oled_addr);
    i2c_write(oled_send_cmd);
    i2c_write(x & 0b00001111);                 // set lower col adress (last four bits of 0b00000000, 00H - 0FH (0b00001111))
    i2c_write(0b00010000 | (x >> 4));         // set higher col adress (last 3 bits of 0b00010000, 10H - 17H)
    i2c_write(0xB0 | y);                     // set page address (last 4 bits of 0b10110000, B0H - BFH, 15 pages)
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
    for (uint8_t column = 0; column < 6; column++){         //for each byte making the digit
        int bits = pgm_read_byte(&oled_font[c][column]);    //read corresponding bits
        i2c_write(bits);                                    //send bits to oled
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


void readings_internal_setup(){
    ADMUX = 0b10001111; // 1.1v ref,  adlar def., internal temp sensor
    //ADMUX = 0 << REFS2 | 2 << REFS0 | 15 << MUX0; //1.1V ref, left adjust, ADC4 for temp measurement 
    //ADCSRA = 1 << ADEN | 1 << ADIE | 4 << ADPS0; //ADC enable, ADC interrupt enable,
    ADCSRA = 0b10001110; // ADC enable, ADC interrupt enable, prescaler div factor 64
    ADCSRB = 0;                 
    //set_sleep_mode(SLEEP_MODE_ADC);
}
unsigned int readings_internal(){
    MCUCR = (1 << SE) | (1 << SM0); //sleep enable, adc noise reduction mode
    //sleep_enable();
     __asm__ __volatile__ ( "sleep" :: ); //put cpu to sleep
    return ADC;
}

void readings_thermocouple_setup(){
    ADMUX = 0b10000111; // 1.1v ref, adlar def., 20x adc gain pb3 pb4
    ADCSRA = 0b10001110;  // ADC enable, ADC interrupt enable, prescaler div factor 64
    ADCSRB = 0;   
}

unsigned int readings_thermocouple(){
    MCUCR = (1 << SE) | (1 << SM0); //sleep enable, adc noise reduction mode
    //sleep_enable();
     __asm__ __volatile__ ( "sleep" :: ); //put cpu to sleep
    return ADC;
}

//////// VOLTAGE - TEMP CONVERTION ///////////////////////////////////////////////
int temp[9] = {0, 1684, 3370, 4995, 6613, 8269, 9993, 11805, 13751};
int adc_to_temp(int adc){
    int n = adc >> 9;
    unsigned int difference = temp[n+1] - temp[n];
    unsigned int proportion = adc - (n << 9);
    unsigned int extra = ((unsigned long)proportion * difference) >> 9;
    return (temp[n] + extra + 5)/10;
}


ISR(ADC_vect){
    
}

const int internal_offset = -17;
const int adc_offset =-7;
int j = 0;
int main(void){
    oled_init(); 
    oled_clear();
    SREG = 0b10000000;

    while(1){
    //internal reading
    readings_internal_setup();
    readings_internal();
    int internal_temp = 0;
    for(int i=0;i<16;i++) internal_temp = internal_temp + readings_internal();
    internal_temp = (internal_temp >> 4) - 276 + internal_offset;
    oled_print_temp(internal_temp,0,0);


    //thermocouple
    readings_thermocouple_setup();
    readings_thermocouple();
    int thermocouple_temp = 0;
    for(int j=0;j<16;j++) thermocouple_temp = thermocouple_temp + readings_internal();
    thermocouple_temp = adc_to_temp(thermocouple_temp >> 2) + adc_offset*4;
    //thermocouple_temp = (thermocouple_temp >> 4) - 256;
    oled_print_temp(thermocouple_temp + internal_temp, 0, 5);
    
    _delay_ms(1000);
    } 
}



