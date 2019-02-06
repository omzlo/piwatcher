/*
    This code largely borrows from i2c-dev.h, form:
        Copyright (C) 1995-97 Simon G. Vogl
        Copyright (C) 1998-99 Frodo Looijaard <frodol@dds.nl>
   
    It is released under the GPL like the original work:    

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/ 

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "i2c.h"

/*
 *  Data for SMBus Messages
 */
#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */
#define I2C_SMBUS_I2C_BLOCK_MAX	32	/* Not specified but we use same structure */
union i2c_smbus_data {
	uint8_t byte;
	uint16_t word;
	uint8_t block[I2C_SMBUS_BLOCK_MAX + 2]; /* block[0] is used for length */
	/* and one more for PEC */
};

/* This is the structure as used in the I2C_SMBUS ioctl call */
struct i2c_smbus_ioctl_data {
	char read_write;
	uint8_t command;
	int size;
	union i2c_smbus_data *data;
};

/* smbus_access read or write markers */
#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0

/* SMBus transaction types (size parameter in the above functions)
 *    Note: these no longer correspond to the (arbitrary) PIIX4 internal codes! */
#define I2C_SMBUS_QUICK		    0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2
#define I2C_SMBUS_WORD_DATA	    3
#define I2C_SMBUS_PROC_CALL	    4
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7		/* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

/* ioctl stuff */
#define I2C_SLAVE	0x0703	/* Change slave address			*/
				/* Attn.: Slave address is 7 or 10 bits */
#define I2C_SMBUS	0x0720	/* SMBus-level access */



static int i2c_set_slave_address(int file, int address)
{
	if (ioctl(file, I2C_SLAVE, address) < 0) {
		fprintf(stderr,
				"Error: Could not set address to 0x%02x: %s\n",
				address, strerror(errno));
		return -errno;
	}
	return 0;
}

static int i2c_smbus_access(int file, char read_write, uint8_t command,
		int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}

/*******************************************************************/

int i2c_open_dev(int bus)
{
    char filename[100];
    int file;

    snprintf(filename, 100, "/dev/i2c-%d", bus);
    filename[99]=0;
    file = open(filename, O_RDWR);

    return file;
}

int i2c_read_byte_data(int file, uint8_t addr, uint8_t reg) 
{
    union i2c_smbus_data data;
    int res;

    if ((res = i2c_set_slave_address(file, addr))<0) {
        return res;
    }

    if (i2c_smbus_access(file, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE_DATA,&data))
        return -1;
    return 0x0FF & data.byte;
}

int i2c_write_byte_data(int file, uint8_t addr, uint8_t reg, uint8_t byte) 
{
    union i2c_smbus_data data;
    int res;

    if ((res = i2c_set_slave_address(file, addr))<0) {
        return res;
    }

    data.byte = byte;

    if (i2c_smbus_access(file, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA,&data))
        return -1;
    return 0;
}

int i2c_read_block(int file, uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t count)
{
    if (count>32 || count<1)
    {
        fprintf(stderr, "i2c block cannot be empty or more than 32 bytes.\n");
        return -1;
    }

    union i2c_smbus_data data;
    int res;

    if ((res = i2c_set_slave_address(file, addr))<0) {
        return res;
    }

    uint32_t size;
    if (count==32)
        size = I2C_SMBUS_I2C_BLOCK_BROKEN;
    else
        size = I2C_SMBUS_I2C_BLOCK_DATA;

    data.block[0] = count;

    if (i2c_smbus_access(file, I2C_SMBUS_READ, reg, size, &data)) {
        return -1;
    }
    for (int i=1;i<=data.block[0];i++)
        buf[i-1] = data.block[i];
    return data.block[0];
}

int i2c_write_block(int file, uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t count)
{
    if (count>32 || count<1)
    {
        fprintf(stderr, "i2c block cannot be empty or more than 32 bytes.\n");
        return -1;
    }

    union i2c_smbus_data data;
    int res;

    if ((res = i2c_set_slave_address(file, addr))<0) {
        return res;
    }

    uint32_t size;
    if (count==32)
        size = I2C_SMBUS_I2C_BLOCK_BROKEN;
    else
        size = I2C_SMBUS_I2C_BLOCK_DATA;

    data.block[0] = count;
    for (int i=1;i<=count;i++)
        data.block[i] = buf[i-1];
    
    if (i2c_smbus_access(file, I2C_SMBUS_WRITE, reg, size, &data))
        return -1;
    return 0;
}
