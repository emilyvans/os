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

void print_pci_device(volatile pci_header *pci_device, uint8_t function) {
	if (pci_device->header_type & 0x80) {
		printf("-----------------------------------%hhu\n", function);
	} else {
		printf("------------------------------------\n");
	}
	printf("vendor: 0x%x\ndevice: 0x%x\ncommand: 0x%x\nstatus: "
	       "%b\nrevision_id: 0x%x\nprog_if: 0x%x\nclass: 0x%x\n"
	       "subclass: 0x%x\ncacheline_size: 0x%x\nlatency_timer: "
	       "0x%x\ntype: 0x%x\nBIST: 0x%x\n",
	       pci_device->Vendor_ID, pci_device->device_ID, pci_device->command,
	       pci_device->status, pci_device->revision_ID, pci_device->prog_IF,
	       pci_device->class_code, pci_device->subclass,
	       pci_device->cache_line_size, pci_device->latency_timer,
	       pci_device->header_type, pci_device->BIST);

	if ((pci_device->header_type & 0x7F) == 0x0) {
		printf("bar0: 0x%x\nbar1: 0x%x\nbar2: 0x%x\nbar3: 0x%x\nbar4: "
		       "0x%x\nbar5: 0x%x\ncardbus_CIS_pointer: 0x%x\n"
		       "subsystem_vendor: 0x%x\nsubsystem: 0x%x\n"
		       "expansion_rom: 0x%x\ncapabilities_pointer: 0x%x\n"
		       "interrupt_line: 0x%x\ninterrupt_pin: 0x%x\nmin_grant: "
		       "0x%x\nmax_latency: 0x%x\n",
		       pci_device->type_0.BAR0, pci_device->type_0.BAR1,
		       pci_device->type_0.BAR2, pci_device->type_0.BAR3,
		       pci_device->type_0.BAR4, pci_device->type_0.BAR5,
		       pci_device->type_0.Cardbus_CIS_pointer,
		       pci_device->type_0.subsystem_vendor_id,
		       pci_device->type_0.subsystem_id,
		       pci_device->type_0.expansion_ROM_base_address,
		       pci_device->type_0.capabilities_pointer,
		       pci_device->type_0.interrupt_line,
		       pci_device->type_0.interrupt_pin, pci_device->type_0.min_grant,
		       pci_device->type_0.max_latency);
	}
}

BusType pci_bus = {.name = "PCI", .match = &PCI_MATCH, .probe = &PCI_PROBE};
