#ifndef INCLUDE_DRIVER_DEVICE_HPP_
#define INCLUDE_DRIVER_DEVICE_HPP_

#include "list/klist.hpp"
#include <stdint.h>

typedef struct Device {
	KListHead list_global;
	KListHead list_bus;
	KListHead list_driver;
	struct Driver *active_driver = nullptr;
	struct BusType *bus = nullptr;
	void *driver_data;
} Device;

typedef struct Driver {
	const char *name;
	KListHead list_global;
	KListHead list_bus;
	KListHead device_list;
	int (*probe)(Device *device);
} Driver;

typedef struct BusType {
	const char *name;
	KListHead driver_list;
	KListHead device_list;
	KListHead bus_list;
	int (*probe)(Device *device);
	int (*match)(Device *device, Driver *driver);
} BusType;

extern KListHead bus_list;
extern KListHead device_list;
extern KListHead driver_list;

void register_bus(BusType *bus);
void register_driver(Driver *driver, BusType *bus);
void register_device(Device *device, BusType *bus);
void unregister_driver(Driver *driver, BusType *bus);
void unregister_device(Device *device, BusType *bus);

#endif // INCLUDE_DRIVER_DEVICE_HPP_
