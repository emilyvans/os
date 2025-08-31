#include "8x8font.h"
#include "GDT.hpp"
#include "interupts.hpp"
#include "limine.h"
#include "limine_requests.hpp"
#include "physicalmem.hpp"
#include "utils.hpp"
#include "virtualmem.hpp"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define background_color 0x1d1f21

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((
	used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used,
               section(".limine_requests_"
                       "start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
	used,
	section(
		".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

// Halt and catch fire function.
static void hcf(void) {
	for (;;) {
		asm("hlt");
	}
}

struct point {
	uint64_t x;
	uint64_t y;
};

uint64_t cols = 0;
uint64_t rows = 0;
point cursor = {0, 0};
limine_framebuffer *framebuffer;
uint64_t scale = 2;

void put_char(char c) {
	volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
	uint64_t start_y = cursor.y * 8 * scale;
	uint64_t start_x = cursor.x * 8 * scale;
	uint8_t *character = font8x8_basic[(uint8_t)c];
	for (uint64_t y = start_y; y < start_y + (8 * scale); y++) {
		uint8_t row = character[int((y - start_y) / scale)];
		for (uint64_t x = start_x; x < start_x + (8 * scale); x++) {
			bool pixel = (row >> int((x - start_x) / scale)) & 1;
			if (pixel) {
				fb_ptr[y * framebuffer->width + x] = 0xFFFFFF;
			} else {
				// fb_ptr[y * framebuffer->width + x] = 0x000000;
			}
		}
	}
	cursor.x++;
	if (cursor.x == cols) {
		cursor.x = 0;
		cursor.y++;
	}
}

void print(const char *fmt) {
	char *to_print = (char *)fmt;

	while ((uint32_t)*to_print != 0) {
		put_char(*to_print);
		to_print++;
	}
	return;
}

void print(const char *fmt, uint64_t count) {
	char *to_print = (char *)fmt;
	for (uint64_t i = 0; i < count; i++) {
		put_char(*to_print);
		to_print++;
	}
}

char digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void print(uint64_t original_number, uint64_t base = 10) {
	char rev_num[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint64_t number = original_number % base;
	uint64_t current_running_number = original_number - number;
	current_running_number = current_running_number / base;
	uint64_t count = 1;
	rev_num[0] = digits[number];
	// put_char(char(number + '0'));
	while (current_running_number != 0) {
		count++;
		number = current_running_number % base;
		current_running_number -= number;
		current_running_number = current_running_number / base;
		rev_num[count - 1] = digits[number];
	}
	if (count > 64) {
		count = 64;
	}
	for (uint64_t i = count + 1; i >= 1; i--) {
		if (rev_num[i - 1] != 0) {
			put_char(rev_num[i - 1]);
		}
	}
}

void itoa(char *buffer, uint64_t num, uint64_t base = 10) {
	char rev_num[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint64_t number = num % base;
	uint64_t current_running_number = num - number;
	current_running_number = current_running_number / base;
	uint64_t count = 1;
	rev_num[0] = digits[number];
	// put_char(char(number + '0'));
	while (current_running_number != 0) {
		count++;
		number = current_running_number % base;
		current_running_number -= number;
		current_running_number = current_running_number / base;
		rev_num[count - 1] = digits[number];
	}
	if (count > 64) {
		count = 64;
	}
	uint64_t index = 0;
	for (uint64_t i = count + 1; i >= 1; i--) {
		if (rev_num[i - 1] != 0) {
			buffer[index] = rev_num[i - 1];
			index += 1;
		}
	}
	buffer[index] = 0;
}

uint64_t strlen(const char *str) {
	uint64_t i = 0;
	while (str[i] != 0 && i != INT64_MAX) {
		i += 1;
	}
	return i;
}

void printf(const char *fmt, ...) {
	va_list arg_list;
	va_start(arg_list, fmt);

	for (uint64_t i = 0; i < strlen(fmt); i++) {
		if (fmt[i] != '%') {
			if (fmt[i] == '\n') {
				cursor.x = 0;
				cursor.y += 1;
			} else {
				put_char(fmt[i]);
			}
			continue;
		}
		i += 1;
		switch (fmt[i]) {
		case 's': {
			char *str = va_arg(arg_list, char *);
			for (uint64_t j = 0; j < strlen(str); j++) {
				put_char(str[j]);
			}
		} break;
		case 'c': {
			put_char(va_arg(arg_list, int));
		} break;
		case '%': {
			put_char('%');
		} break;
		case 'b': {
			char buffer[64];
			itoa(buffer, va_arg(arg_list, uint64_t), 2);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'x': {
			char buffer[64];
			itoa(buffer, va_arg(arg_list, uint64_t), 16);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'p': {
			char buffer[64];
			itoa(buffer, va_arg(arg_list, uint64_t), 16);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		case 'u': {
			char buffer[64];
			itoa(buffer, va_arg(arg_list, uint64_t), 10);
			for (uint64_t j = 0; j < strlen(buffer); j++) {
				put_char(buffer[j]);
			}
		} break;
		}
	}

	va_end(arg_list);
}

void print_entry(Idt entry) {
	printf("%x %u %b %b %x %x %u 0x%x\n", entry.address_low, entry.selector,
	       entry.ist, entry.flags, entry.address_mid, entry.address_high,
	       entry.reserved,
	       ((uint64_t)entry.address_high << 32) |
	           ((uint64_t)entry.address_mid << 16) |
	           (uint64_t)entry.address_low);
}

void print_entry(GDT_entry entry) {
	printf("%x %x %x %b %b %x\n", entry.Limit0, entry.Base0, entry.Base1,
	       entry.Accessbyte, entry.Limit1_Flags, entry.Base2);
}

void clear_screen(uint32_t color) {
	for (size_t x = 0; x < framebuffer->width; x++) {
		for (size_t y = 0; y < framebuffer->height; y++) {
			volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
			fb_ptr[y * framebuffer->width + x] = color; // 0x0000FF;
		}
	}
	cursor = {0, 0};
}

__attribute__((interrupt)) void pf_handler(struct interrupt_frame *frame,
                                           uint64_t error_code) {
	clear_screen(background_color);
	printf("page fault\ntype: %x\nip: %x\ncs: %x\nflags: %x\nsp: %x\nss: "
	       "%x\nerror code: %x\n",
	       0xE, frame->ip, frame->cs, frame->flags, frame->sp, frame->ss,
	       error_code);
	hcf();
}
__attribute__((interrupt)) void
double_fault_handler(struct interrupt_frame *frame, uint64_t error_code) {
	printf("double fault");
	hcf();
}
__attribute__((interrupt)) void gp_handler(struct interrupt_frame *frame,
                                           uint64_t error_code) {
	printf("general protation");
	hcf();
}
__attribute__((interrupt)) void iop_handler(struct interrupt_frame *frame,
                                            uint64_t error_code) {
	printf("invalid opcode");
	hcf();
}
__attribute__((interrupt)) void unknown_handler(struct interrupt_frame *frame) {
	printf("unknow exception");
	hcf();
}

static Idtrr idt_reg = {};
static Idtr idtr = {};
static GDT_table gdt_table = {};
static GDT_descriptor gdt_descriptor = {};

struct generic_address_structure {
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

struct ACPI_SDT_header {
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

struct ACPI_FADT_table {
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
	generic_address_structure reset_register;
	uint8_t reset_value;
	uint16_t arm_boot_architecture;
	uint8_t minor_version;
	generic_address_structure x_firmware_control;
	generic_address_structure x_DSDT;
	generic_address_structure x_PM_1A_event_block;
	generic_address_structure x_PM_1B_event_block;
	generic_address_structure x_PM_1A_control_block;
	generic_address_structure x_PM_1B_control_block;
	generic_address_structure x_PM_2_control_block;
	generic_address_structure x_PM_timer_block;
	generic_address_structure x_GPE0_block;
	generic_address_structure x_GPE1_block;
	generic_address_structure sleep_control;
	generic_address_structure sleep_status;
} __attribute__((packed));

struct XSDT {
	ACPI_SDT_header header;
	uint64_t pointer_to_other_sdt[];
} __attribute__((packed));

struct ACPI_descriptor_table {
	enum { FADT } type;
	ACPI_SDT_header header;
	union {
		ACPI_FADT_table FADT_table;
	} data;
};

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
extern "C" void kmain(void) {
	// Ensure the bootloader actually understands our base revision (see
	// spec).
	if (LIMINE_BASE_REVISION_SUPPORTED == false) {
		hcf();
	}

	// Ensure we got a framebuffer.
	if (framebuffer_request.response == NULL ||
	    framebuffer_request.response->framebuffer_count < 1) {
		hcf();
	}

	if (hhdm_request.response == NULL) {
		hcf();
	}

	// Fetch the first framebuffer.
	framebuffer = framebuffer_request.response->framebuffers[0];

	for (size_t x = 0; x < framebuffer->width; x++) {
		for (size_t y = 0; y < framebuffer->height; y++) {
			volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
			fb_ptr[y * framebuffer->width + x] = 0x1d1f21; // 0x0000FF;
		}
	}

	cols = framebuffer->width / 8;
	rows = framebuffer->height / 8;

	// TODO: init global descriptor table

	set_gdt_entry(&gdt_table, 0, 0, 0, 0, 0);
	// kernel code segment
	set_gdt_entry(
		&gdt_table, 1, 0, 0,
		(uint8_t)(GDT_flags::page_granularity | GDT_flags::long_mode_segemnt),
		(uint8_t)(GDT_accessbyte_flags::read |
	              GDT_accessbyte_flags::executable |
	              GDT_accessbyte_flags::code_data_segemnt |
	              GDT_accessbyte_flags::present));
	// kernel data segment
	set_gdt_entry(
		&gdt_table, 2, 0, 0,
		(uint8_t)(GDT_flags::page_granularity | GDT_flags::long_mode_segemnt),
		(uint8_t)(GDT_accessbyte_flags::read |
	              GDT_accessbyte_flags::code_data_segemnt |
	              GDT_accessbyte_flags::present));
	// TODO: fix permission for user segements

	// user code segment
	set_gdt_entry(
		&gdt_table, 3, 0, 0,
		(uint8_t)(GDT_flags::page_granularity | GDT_flags::long_mode_segemnt),
		(uint8_t)(GDT_accessbyte_flags::read |
	              GDT_accessbyte_flags::executable |
	              GDT_accessbyte_flags::code_data_segemnt |
	              GDT_accessbyte_flags::present));
	// user data segment
	set_gdt_entry(
		&gdt_table, 4, 0, 0,
		(uint8_t)(GDT_flags::page_granularity | GDT_flags::long_mode_segemnt),
		(uint8_t)(GDT_accessbyte_flags::read |
	              GDT_accessbyte_flags::code_data_segemnt |
	              GDT_accessbyte_flags::present));

	gdt_descriptor.Size = sizeof(GDT_table) - 1;
	gdt_descriptor.Offset = (uint64_t)&gdt_table.entries;

	load_gdt(&gdt_descriptor);

	uint64_t offset = executable_address_request.response->virtual_base -
	                  executable_address_request.response->physical_base;

	// TODO: init interupt descriptor table
	for (uint64_t i = 0; i < 256; i++) {
		set_IDT_entry(&idtr, i, (uint64_t)unknown_handler, 0);
	}
	set_IDT_entry(&idtr, 0x6, (uint64_t)iop_handler, 0);
	set_IDT_entry(&idtr, 0x8, (uint64_t)double_fault_handler, 0);
	set_IDT_entry(&idtr, 0xD, (uint64_t)gp_handler, 0);
	set_IDT_entry(&idtr, 0xE, (uint64_t)pf_handler, 0);
	idt_reg.limit = sizeof(Idtr) - 1;
	idt_reg.base = (uint64_t)&idtr.idts;
	asm("cli");
	load_idt(&idt_reg);
	// enable interupts
	asm("sti");

	// TODO: load physical memory in virtual memory for kernel(identity) 0
	// -> 0x100000000 write page map lvl 4/5(default 4) entry to %%cr3
	Physical_memory::initialize();
	Virtual_memory::initialize();

	uint64_t memory_offset = hhdm_request.response->offset;

	RSDP *rsdp = (RSDP *)(rsdp_request.response->address + memory_offset);

	if (rsdp->Revision != 2) {
		printf("rsdp revision must be 2\n");
		hcf();
	}

	XSDP *xsdp = (XSDP *)(rsdp);

	XSDT *root_xsdt = (XSDT *)(xsdp->XsdtAddress + memory_offset);

	uint64_t entries =
		(root_xsdt->header.Length - sizeof(root_xsdt->header)) / 8;
	for (uint64_t i = 0; i < entries; i++) {
		ACPI_SDT_header *header =
			(ACPI_SDT_header *)(root_xsdt->pointer_to_other_sdt[i] +
		                        memory_offset);
		// FACP APIC HPET WAET(could ignore) BGRT
		if (memcmp(header->Signature, "FACP", 4) == 0) {
		}

		const char signature[5] = {0, 0, 0, 0, 0};
		memcpy((void *)signature, (void *)header->Signature, 4);
		printf("%s(0x%x) %u %u\n", signature, *(uint32_t *)header->Signature,
		       header->Revision, header->Length);
	}

	// clear_screen(0x1d1f21);

	// We're done, just hang...
	hcf();
}
