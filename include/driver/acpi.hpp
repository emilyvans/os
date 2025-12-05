#ifndef INCLUDE_DRIVER_ACPI_HPP_
#define INCLUDE_DRIVER_ACPI_HPP_
#include <stdint.h>

struct GenericAddressStructure {
	uint8_t address_space;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	uint64_t address;
} __attribute__((packed));

struct RSDP {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
} __attribute__((packed));

struct XSDP {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress; // deprecated since version 2.0
	uint32_t Length;
	uint64_t XsdtAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));

struct ACPISDTHeader {
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
} __attribute__((packed));

struct ACPIFADTTable {
	uint32_t firmware_control;
	uint32_t DSDT_address;
	uint8_t interupt_model;
	uint8_t preferred_power_managemnt_profile;
	uint16_t SCI_interrupt;
	uint32_t SMM_interrupt_command_port;
	uint8_t ACPI_enable;
	uint8_t ACPI_disable;
	uint8_t S4BIOS_request;
	uint8_t PSTATE_control;
	uint32_t PM_1A_event_block;
	uint32_t PM_1B_event_block;
	uint32_t PM_1A_control_block;
	uint32_t PM_1B_control_block;
	uint32_t PM2_control_block;
	uint32_t PM_timer_block;
	uint32_t GPE0_block;
	uint32_t GPE1_block;
	uint8_t PM1_event_length;
	uint8_t PM1_control_length;
	uint8_t PM2_control_length;
	uint8_t PM_timer_length;
	uint8_t GPE0_length;
	uint8_t GPE1_length;
	uint8_t CSTATE_control;
	uint16_t worst_C2_latency;
	uint16_t worst_C3_latency;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alarm;
	uint8_t month_alarm;
	uint8_t century;
	//
	uint16_t boot_architecture_flags;
	uint8_t reserved;
	uint32_t flag;
	GenericAddressStructure reset_register;
	uint8_t reset_value;
	uint16_t arm_boot_architecture;
	uint8_t minor_version;
	GenericAddressStructure x_firmware_control;
	GenericAddressStructure x_DSDT;
	GenericAddressStructure x_PM_1A_event_block;
	GenericAddressStructure x_PM_1B_event_block;
	GenericAddressStructure x_PM_1A_control_block;
	GenericAddressStructure x_PM_1B_control_block;
	GenericAddressStructure x_PM_2_control_block;
	GenericAddressStructure x_PM_timer_block;
	GenericAddressStructure x_GPE0_block;
	GenericAddressStructure x_GPE1_block;
	GenericAddressStructure sleep_control;
	GenericAddressStructure sleep_status;
} __attribute__((packed));

struct XSDT {
	ACPISDTHeader header;
	uint64_t pointer_to_other_sdt[];
} __attribute__((packed));

struct ACPIDescriptorTable {
	enum { FADT } type;
	ACPISDTHeader header;
	union {
		ACPIFADTTable FADT_table;
	} data;
};

struct InteruptController {
	uint8_t type;
	uint8_t size;
} __attribute__((packed));

struct MADT {
	ACPISDTHeader header;
	uint32_t local_interupt_controller_address;
	uint32_t flags;
} __attribute__((packed));

struct BGRT {
	ACPISDTHeader header;
	uint16_t version_id;
	uint8_t status;
	uint8_t image_type;
	uint64_t image_address;
	uint32_t image_x_offset;
	uint32_t image_y_offset;
} __attribute__((packed));

struct BitmapDIB {
	uint32_t size;
	int32_t width;
	int32_t height;
	uint16_t color_planes_count;
	uint16_t bit_per_pixel;
	uint32_t compression_method;
	uint32_t image_size;
	uint32_t horizontal_resolution;
	uint32_t vertical_resolution;
	uint32_t color_count_in_color_palette;
	uint32_t important_colors_count;
} __attribute__((packed));

struct BitmapHeader {
	char magic[2];
	uint32_t file_size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t pixel_byte_offset;
	BitmapDIB info_header;
} __attribute__((packed));

void init_ACPI();
void display_BGRT(BGRT *bgrt);

#endif // INCLUDE_DRIVER_ACPI_HPP_
