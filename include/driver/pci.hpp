#ifndef INCLUDE_DRIVER_PCI_HPP_
#define INCLUDE_DRIVER_PCI_HPP_
#include "driver/device.hpp"
#include <stdint.h>

#define PCI_ANY_ID (~0U)

typedef struct PCIDevice {
	uint8_t bus;
	uint8_t device_number;
	uint8_t function;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t subclass;
	uint8_t class_code;
	uint64_t config_address; // ECAM ADDRESS
	struct Device device;
} PCIDevice;

typedef struct PCIDeviceID {
	uint32_t vendor_id;
	uint32_t device_id;
	uint32_t sub_vendor_id;
	uint32_t sub_device_id;
} PCIDeviceID;

typedef struct PCIDriver {
	const char *name;
	PCIDeviceID *id_table;
	int (*probe)(PCIDevice *pci_dev);
	Driver driver;
} PCIDriver;

void register_pci_driver(PCIDriver *driver);
void register_pci_device(PCIDevice *device);

extern BusType pci_bus;

#endif // INCLUDE_DRIVER_PCI_HPP_
