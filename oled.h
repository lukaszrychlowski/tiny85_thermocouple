#ifndef OLED_H  
#define OLED_H

#define oled_addr 0x78                                          // i2c address is 0x3C, we're adding rw bit = 0 (0x3C << 1 = 0x78)
#define oled_send_cmd 0x00                                      // oled command mode
#define oled_send_data 0x40                                     // oled data mode



void oled_init(void);
void oled_set_cursor(uint8_t, uint8_t);
void oled_pixel_off(void);
void oled_pixel_on(void);
void oled_clear(void);
void oled_light_up(void);
void oled_print_char(int);
uint8_t get_digit(unsigned int, unsigned int);
void oled_print_temp(int, int, int);
#endif