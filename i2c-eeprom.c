// Copyright 2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdlib.h>

#include "types.h"
#include "platform.h"

#include "i2c.h"
	#include <stdio.h>


struct i2c_eeprom {
	struct i2c_device dev;

	u8 *data;
	u32 size;	// in bytes

	void *file_cookie;

	u8 address;	// i2c bus address
	u32 current;	// data pointer
	int address_bytes_left;
};


static int i2c_eeprom_address(struct i2c_device *dev, u8 address)
{
	struct i2c_eeprom *eeprom = subtype(struct i2c_eeprom, dev, dev);

	// XXX: for 24c04 only
	if ((address & 0xfc) != eeprom->address)
		return 1;

	if ((address & 1) == 0) {
		// XXX: for 24c04 only
		eeprom->address_bytes_left = 1;
		eeprom->current = (address >> 1) & 1;
	}

	return 0;
}

static int i2c_eeprom_write(struct i2c_device *dev, u8 data)
{
	struct i2c_eeprom *eeprom = subtype(struct i2c_eeprom, dev, dev);

	if (eeprom->address_bytes_left) {
		eeprom->address_bytes_left--;
		eeprom->current = (eeprom->current << 8) | data;
	} else {
		eeprom->data[eeprom->current] = data;
//debug("EEPROM: wrote %02x @ %02x, writing to file\n", data, eeprom->current);
		save_eeprom(eeprom->file_cookie, eeprom->data, eeprom->size);
		// XXX: for 24c04 only
		eeprom->current = (eeprom->current & ~0x0f) |
		                  ((eeprom->current + 1) & 0x0f);
	}
	return 0;
}

static u8 i2c_eeprom_read(struct i2c_device *dev)
{
	struct i2c_eeprom *eeprom = subtype(struct i2c_eeprom, dev, dev);

	u8 data = eeprom->data[eeprom->current];
	// XXX: for 24c04 only
	eeprom->current = (eeprom->current + 1) & 0x1ff;

	return data;
}

void i2c_eeprom_create(struct i2c_bus *bus, u32 size, u8 address,
                       const char *file_name)
{
	struct i2c_eeprom *eeprom = malloc(sizeof *eeprom);
	eeprom->dev.next = bus->devices;
	bus->devices = &eeprom->dev;

	eeprom->dev.address  = i2c_eeprom_address;
	eeprom->dev.write    = i2c_eeprom_write;
	eeprom->dev.read     = i2c_eeprom_read;
	eeprom->dev.read_ack = 0;

	eeprom->data = calloc(size, 1);
	eeprom->size = size;

	eeprom->file_cookie = open_eeprom(file_name, eeprom->data, size);

	eeprom->address = address;
	eeprom->current = 0;
}
