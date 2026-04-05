#ifndef INCLUDE_DRIVER_PCI_HPP_
#define INCLUDE_DRIVER_PCI_HPP_
#include "driver/device.hpp"
#include <stdint.h>

typedef struct pci_device {
	uint8_t bus;
	uint8_t device_number;
	uint8_t function;
	uint16_t Vendor_ID;
	uint16_t device_ID;
	uint8_t subclass;
	uint8_t class_code;
	uint64_t config_address;
	struct device_t device;
} pci_device;

#endif // INCLUDE_DRIVER_PCI_HPP_
