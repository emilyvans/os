#include "driver/device.hpp"
#include "driver/console.hpp"
#include "list/container_of.hpp"

KListHead bus_list = {&bus_list, &bus_list};
KListHead device_list = {&device_list, &device_list};
KListHead driver_list = {&driver_list, &driver_list};

void register_bus(BusType *bus) {
	klist_add_tail(&bus_list, &bus->bus_list);
	klist_init(&bus->device_list);
	klist_init(&bus->driver_list);
}

void register_driver(Driver *driver, BusType *bus) {
	klist_add_tail(&driver_list, &driver->list_global);
	klist_add_tail(&bus->driver_list, &driver->list_bus);
	klist_init(&driver->device_list);
	for (Device *dev = container_of(bus->device_list.next, Device, list_bus);
	     &dev->list_bus != &bus->device_list;
	     dev = container_of(dev->list_bus.next, Device, list_bus)) {
	}
	KLIST_FOREACH(&bus->device_list) {
		Device *device = container_of(head, Device, list_bus);
		if (device->active_driver != nullptr) {
			continue;
		}
		if (bus->match(device, driver) != 0) {
			continue;
		}
		device->active_driver = driver;
		int result = 1;
		if (bus->probe) {
			result = bus->probe(device);
		} else if (driver->probe) {
			result = driver->probe(device);
		}
		if (result) {
			klist_add_tail(&driver->device_list, &device->list_driver);
		} else {
			device->active_driver = nullptr;
		}
	}
}

void register_device(Device *device, BusType *bus) {
	klist_add_tail(&device_list, &device->list_global);
	klist_add_tail(&bus->device_list, &device->list_bus);
	device->bus = bus;
	device->active_driver = nullptr;
	KLIST_FOREACH(&bus->driver_list) {
		Driver *driver = container_of(head, Driver, list_bus);
		if (bus->match(device, driver) != 0) {
			continue;
		}

		device->active_driver = driver;
		int result = 1;
		if (bus->probe) {
			result = bus->probe(device);
		} else if (driver->probe) {
			result = driver->probe(device);
		}
		if (result) {
			klist_add_tail(&driver->device_list, &device->list_driver);
			break;
		} else {
			device->active_driver = nullptr;
		}
	}
}

void unregister_driver(Driver *driver, BusType *bus) {}
void unregister_device(Device *device, BusType *bus) {}

#if 0
#define klist_for_each_entry(pos, head, member, iter)                          \
	for (klist_iter_init(head, iter);                                          \
	     (pos = ({                                                             \
			  struct klist_node *n = klist_next(iter);                         \
			  n ? container_of(n, typeof(*pos), member)({                      \
				  klist_iter_exit(iter);                                       \
				  NULL;                                                        \
			  });                                                              \
		  })) != NULL;)
#endif
