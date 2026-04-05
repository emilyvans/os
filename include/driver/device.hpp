#ifndef INCLUDE_DRIVER_DEVICE_HPP_
#define INCLUDE_DRIVER_DEVICE_HPP_

#include <stdint.h>

typedef struct klist_head {
	struct klist_head *next, *prev;
	void *owner;
} klist_head;

#define KLIST_FOREACH(list_head)                                               \
	for (klist_head *head = (list_head)->next; head != (list_head);            \
	     head = head->next)

typedef struct device_t {
	struct klist_head list_global;
	struct klist_head list_siblings;
	struct driver_t *active_driver;
	struct bus_type *bus;
} device_t;

typedef struct driver_t {
	struct klist_head list_global;
	struct klist_head list_siblings;
	int (*probe)(device_t *device, driver_t *driver);
	int (*match)(device_t *device, driver_t *driver);

} driver_t;

typedef struct bus_type {
	const char *name;
	struct klist_head driver_list;
	struct klist_head device_list;
	struct klist_head bus_list;
	int (*probe)(device_t *device, driver_t *driver);
	int (*match)(device_t *device, driver_t *driver);
} bus_type;

extern klist_head bus_list;
extern klist_head device_list;
extern klist_head driver_list;

#endif // INCLUDE_DRIVER_DEVICE_HPP_
