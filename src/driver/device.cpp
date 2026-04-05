#include "driver/device.hpp"
#include "driver/console.hpp"
#undef offsetof
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

klist_head bus_list = {&bus_list, &bus_list, nullptr};

// TODO: add error handling
#define container_of(ptr, type, member)                                        \
	({                                                                         \
		void *member_ptr = (void *)(ptr);                                      \
		((type *)((char *)member_ptr - offsetof(type, member)));               \
	})

void register_bus(bus_type *bus) {
	bus->bus_list.prev = bus_list.prev;
	bus->bus_list.next = &bus_list;
	bus_list.prev->next = &bus->bus_list;
	bus_list.prev = &bus->bus_list;
	bus->bus_list.owner = bus;
	KLIST_FOREACH(&bus_list) {
		bus_type *buss = container_of(head, bus_type, bus_list);
		printf("owner: %s", buss->name);
	}
}
