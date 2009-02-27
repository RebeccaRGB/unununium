// Copyright 2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include <stdlib.h>

#include "i2c.h"


static void i2c_bus_find_device(struct i2c_bus *bus, u8 address)
{
	struct i2c_device *dev;

	for (dev = bus->devices; dev; dev = dev->next)
		if (dev->address(dev, address) == 0)
			break;

	bus->device = dev;
}

static int i2c_bus_write(struct i2c_bus *bus, u8 data)
{
	struct i2c_device *dev = bus->device;
	return dev->write(dev, data);
}

static u8 i2c_bus_read(struct i2c_bus *bus)
{
	struct i2c_device *dev = bus->device;
	return dev->read(dev);
}

static void i2c_bus_read_ack(struct i2c_bus *bus, int ack)
{
	struct i2c_device *dev = bus->device;
	if (dev->read_ack)
		dev->read_ack(dev, ack);
}


struct i2c_bitbang_bus {
	struct i2c_bus bus;

	void (*clock)(struct i2c_bitbang_bus *);

	int sda, scl, sda_dev;
	int dir;
	u8 bits;
	int nbits;
};

struct i2c_bus *i2c_bitbang_bus_create(void)
{
	return calloc(sizeof(struct i2c_bitbang_bus), 1);
}

static void clock_write(struct i2c_bitbang_bus *bb);

static void clock_write_ack(struct i2c_bitbang_bus *bb)
{
	if (bb->sda_dev == 0) {
		bb->clock = clock_write;
		bb->bits = 0;
		bb->nbits = 0;
		bb->sda_dev = 1;
	} else {
		bb->clock = 0;
	}
}

static void clock_write(struct i2c_bitbang_bus *bb)
{
	bb->bits = 2*bb->bits + bb->sda;
	bb->nbits++;
	if (bb->nbits == 8) {
		bb->sda_dev = i2c_bus_write(&bb->bus, bb->bits);
		bb->clock = clock_write_ack;
	}
}

static void clock_read(struct i2c_bitbang_bus *bb);

static void clock_read_ack(struct i2c_bitbang_bus *bb)
{
	i2c_bus_read_ack(&bb->bus, bb->sda);
	bb->clock = clock_read;
	if (bb->sda == 0) {
		bb->bits = i2c_bus_read(&bb->bus);
		bb->nbits = 0;
		bb->sda_dev = bb->bits >> 7;
		bb->clock = clock_read;
	} else
		bb->clock = 0;
}

static void clock_read(struct i2c_bitbang_bus *bb)
{
	bb->bits = 2*bb->bits;
	bb->sda_dev = bb->bits >> 7;
	bb->nbits++;
	if (bb->nbits == 8) {
		bb->sda_dev = 1;
		bb->clock = clock_read_ack;
	}
}

static void clock_address_ack(struct i2c_bitbang_bus *bb)
{
	if (bb->dir == 0) {
		bb->clock = clock_write;
		bb->bits = 0;
		bb->nbits = 0;
		bb->sda_dev = 1;
	} else {
		bb->clock = clock_read;
		bb->bits = i2c_bus_read(&bb->bus);
		bb->nbits = 0;
		bb->sda_dev = bb->bits >> 7;
	}
}

static void clock_address(struct i2c_bitbang_bus *bb)
{
	bb->bits = 2*bb->bits + bb->sda;
	bb->nbits++;
	if (bb->nbits == 8) {
		i2c_bus_find_device(&bb->bus, bb->bits);
		if (bb->bus.device == 0) {
			bb->clock = 0;
			return;
		}
		bb->sda_dev = 0;
		bb->dir = bb->bits & 1;
		bb->clock = clock_address_ack;
	}
}

int i2c_bitbang(struct i2c_bus *bus, int sda, int scl)
{
	struct i2c_bitbang_bus *bb = subtype(struct i2c_bitbang_bus, bus, bus);
	int old_sda = bb->sda;
	int old_scl = bb->scl;
	bb->sda = sda;
	bb->scl = scl;

	if (old_scl == 1 && scl == 1) {
		if (old_sda == 1 && sda == 0) {		// START
			bb->clock = clock_address;
			bb->bits = 0;
			bb->nbits = -1;
			bb->sda_dev = 1;
			bus->device = 0;
		}
		if (old_sda == 0 && sda == 1) {		// STOP
			bb->clock = 0;
			bb->sda_dev = 1;
		}
	}

	if (old_scl == 1 && scl == 0 && bb->clock)
		bb->clock(bb);

	return sda & bb->sda_dev;
}
