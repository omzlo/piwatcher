#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>

int i2c_open_dev(int file);

int i2c_read_byte_data(int file, uint8_t addr, uint8_t reg);

int i2c_write_byte_data(int file, uint8_t addr, uint8_t reg, uint8_t byte);

int i2c_read_block(int file, uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t count);

int i2c_write_block(int file, uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t count);

#endif
