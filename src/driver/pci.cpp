#include "driver/pci.hpp"
#include "driver/console.hpp"
#include "driver/device.hpp"
#include "list/container_of.hpp"
#include <stdint.h>
void register_pci_driver(PCIDriver *driver) {
	driver->driver.name = driver->name;
	register_driver(&driver->driver, &pci_bus);
}
void register_pci_device(PCIDevice *device) {
	register_device(&device->device, &pci_bus);
}

int PCI_PROBE(Device *dev) {
	printf("PCI probe\n");
	PCIDevice *pci_dev = container_of(dev, PCIDevice, device);
	PCIDriver *pci_drv =
		container_of(pci_dev->device.active_driver, PCIDriver, driver);
	pci_drv->probe(pci_dev);
	return 1;
}

int is_list_terminator(PCIDeviceID id) {
	return id.vendor_id == 0;
}

int PCI_MATCH(Device *dev, Driver *drv) {
	PCIDevice *pci_dev = container_of(dev, PCIDevice, device);
	PCIDriver *pci_drv = container_of(drv, PCIDriver, driver);
	// printf("PCI match vid: 0x%x did: 0x%x\n", pci_dev->vendor_id,
	//        pci_dev->device_id);
	PCIDeviceID *id_list = pci_drv->id_table;
	if (id_list) {
		while (!is_list_terminator(*id_list)) {
			if (!(pci_dev->vendor_id == id_list->vendor_id ||
			      id_list->vendor_id == PCI_ANY_ID)) {
				id_list = id_list + 1;
				continue;
			}
			if (!(pci_dev->device_id == id_list->device_id ||
			      id_list->device_id == PCI_ANY_ID)) {
				id_list = id_list + 1;
				continue;
			}
			return 0;
		}
	}
	return 1;
}

BarAddress get_address_from_bar(volatile pci_header *pci_device,
                                uint8_t bar_index) {
	void *bar_address =
		(void *)&((uint32_t *)&pci_device->type_0.BAR0)[bar_index];
	BarAddress result;
	uint32_t bar = *(uint32_t *)bar_address;
	uint16_t command = pci_device->command;
	if ((bar & 1) == 1) { // I/O space
		result.is_memory_space = false;
		result.address = ((uint64_t)bar & ~(0x1));

		pci_device->command &= ~3;
		*(volatile uint32_t *)bar_address = 0xFFFFFFFF;

		uint32_t size = (~(*((uint32_t *)bar_address) & ~(0x1))) + 1;
		*(volatile uint32_t *)bar_address = bar;
		result.size = size;
	} else if (bar & 0x4) { // 64 bit memory address
		result.is_memory_space = true;
		uint64_t addr = *(uint64_t *)bar_address;
		result.address = ((*(uint64_t *)bar_address) & ~(0xF));

		pci_device->command &= ~3;
		*(volatile uint64_t *)bar_address = 0xFFFFFFFFFFFFFFFF;

		uint64_t size = (~(*((uint64_t *)bar_address) & ~(0xF))) + 1;
		*(volatile uint64_t *)bar_address = addr;
		result.size = size;
	} else { // 32 bit memory address
		result.is_memory_space = true;
		result.address = ((uint64_t)bar & ~(0xF));

		pci_device->command &= ~3;
		*(volatile uint32_t *)bar_address = 0xFFFFFFFF;

		uint32_t size = (~(*((uint32_t *)bar_address) & ~(0xF))) + 1;
		*(volatile uint32_t *)bar_address = bar;
		result.size = size;
	}
	pci_device->command = command;
	return result;
}

BusType pci_bus = {.name = "PCI", .match = &PCI_MATCH, .probe = &PCI_PROBE};
