// Copyright 2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _I2C_H
#define _I2C_H

#include "types.h"


// Device.

struct i2c_device {
	struct i2c_device *next;	// devices on this bus

	int (*address)(struct i2c_device *dev, u8 address);	// returns ack
	int (*write)(struct i2c_device *dev, u8 data);		// returns ack
	u8 (*read)(struct i2c_device *dev);			// returns data
	void (*read_ack)(struct i2c_device *dev, int ack);
};


// Bus.

struct i2c_bus {
	struct i2c_device *devices;	// all devices on this bus
	struct i2c_device *device;	// currently addressed device
};


// EEPROM device.

void i2c_eeprom_create(struct i2c_bus *bus, u32 size, u8 address,
                       const char *file_name);


// Bitbang bus.

struct i2c_bus *i2c_bitbang_bus_create(void);
int i2c_bitbang(struct i2c_bus *bus, int sda, int scl);

#endif
