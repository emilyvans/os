#ifndef INCLUDE_DRIVER_PCI_HPP_
#define INCLUDE_DRIVER_PCI_HPP_
#include "driver/device.hpp"
#include <stdint.h>

#define PCI_ANY_ID (~0U)

struct pci_header {
	uint16_t Vendor_ID;
	uint16_t device_ID;
	uint16_t command;
	uint16_t status;
	uint8_t revision_ID;
	uint8_t prog_IF;
	uint8_t subclass;
	uint8_t class_code;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t BIST;
	union {
		struct {
			uint32_t BAR0;
			uint32_t BAR1;
			uint32_t BAR2;
			uint32_t BAR3;
			uint32_t BAR4;
			uint32_t BAR5;
			uint32_t Cardbus_CIS_pointer;
			uint16_t subsystem_vendor_id;
			uint16_t subsystem_id;
			uint32_t expansion_ROM_base_address;
			uint8_t capabilities_pointer;
			uint8_t reserved0[3];
			uint32_t reserved1;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint8_t min_grant;
			uint8_t max_latency;
		} type_0;
		struct {
			uint32_t BAR0;
			uint32_t BAR1;
			uint8_t primary_bus_number;
			uint8_t secondary_bus_number;
			uint8_t subordinate_bus_number;
			uint8_t secondary_latency_timer;
			uint8_t IO_base;
			uint8_t IO_limit;
			uint16_t secondary_status;
			uint16_t memory_base;
			uint16_t memory_limit;
			uint16_t prefetchable_memory_base;
			uint16_t prefetchable_memory_limit;
			uint32_t prefetchable_base_upper;
			uint32_t prefetchable_limit_upper;
			uint16_t IO_base_upper;
			uint16_t IO_limit_upper;
			uint8_t capability_pointer;
			uint8_t reserved0[3];
			uint32_t expansion_rom_base_address;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint16_t bridge;
		} type_1;
		struct {
			uint32_t cardbus_socket_base_address;
			uint8_t offset_of_capabilities_list;
			uint8_t reserved0;
			uint16_t secondary_status;
			uint8_t PCI_bus_number;
			uint8_t cardbus_bus_number;
			uint8_t subordinate_bus_number;
			uint8_t cardbus_latency_timer;
			uint32_t memory_base_address0;
			uint32_t memory_limit0;
			uint32_t memory_base_address1;
			uint32_t memory_limit1;
			uint32_t IO_base_address0;
			uint32_t IO_limit0;
			uint32_t IO_base_address1;
			uint32_t IO_limit1;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint16_t bridge;
			uint16_t subsystem_device_id;
			uint16_t subsystem_vendor_id;
			uint32_t pc_card_legacy_mode_16_bit_base_address;
		} type_2;
	};
} __attribute__((packed));

typedef struct pci_capability {
	uint8_t vendor;
	uint8_t next;
} __attribute__((packed)) pci_capability;

typedef struct BarAddress {
	bool is_memory_space;
	uint64_t address;
	uint64_t size;
} BarAddress;

BarAddress get_address_from_bar(volatile pci_header *pci_device,
                                uint8_t bar_index);

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
