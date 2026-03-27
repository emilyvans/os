#ifndef INCLUDE_DRIVER_DEVICE_HPP_
#define INCLUDE_DRIVER_DEVICE_HPP_

#include <stdint.h>

typedef struct device_t {
	struct driver_t *active_driver;
	uint16_t vendor_id;
	uint16_t device_id;
	struct device_t *next;
} device_t;

typedef struct driver_t {

	struct driver_t *next;
} driver_t;

typedef struct {
	const char *name;
	driver_t *driver_list;
	device_t *device_list;
	int (*probe)(device_t *);
	int (*match)(device_t *);
} bus_type;

#endif // INCLUDE_DRIVER_DEVICE_HPP_
