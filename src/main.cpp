#include "8x8font.h"
#include "GDT.hpp"
#include "interupts.hpp"
#include "limine.h"
#include "limine_requests.hpp"
#include "physicalmem.hpp"
#include "virtualmem.hpp"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct RSDP {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
} __attribute__((packed));

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
	char rev_num[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
	if (count > 19) {
		count = 19;
	}
	for (uint64_t i = count + 1; i >= 1; i--) {
		if (rev_num[i - 1] != 0) {
			put_char(rev_num[i - 1]);
		}
	}
}

__attribute__((interrupt)) void pf_handler(struct interrupt_frame *frame) {
	print("page fault");
	hcf();
}
__attribute__((interrupt)) void
double_fault_handler(struct interrupt_frame *frame, uint64_t error_code) {
	print("double fault");
	hcf();
}
__attribute__((interrupt)) void gp_handler(struct interrupt_frame *frame,
                                           uint64_t error_code) {
	print("general protation");
	hcf();
}
__attribute__((interrupt)) void iop_handler(struct interrupt_frame *frame,
                                            uint64_t error_code) {
	print("invalid opcode");
	hcf();
}
__attribute__((interrupt)) void unknown_handler(struct interrupt_frame *frame) {
	print("unknow exception");
	hcf();
}

void print_entry(Idt entry) {
	print(entry.address_low, 16);
	print(" ");
	print(entry.selector, 10);
	print(" ");
	print(entry.ist, 2);
	print(" ");
	print(entry.flags, 2);
	print(" ");
	print(entry.address_mid, 16);
	print(" ");
	print(entry.address_high, 16);
	print(" ");
	print(entry.reserved, 10);
	print(" 0x");
	print(((uint64_t)entry.address_high << 32) |
	          ((uint64_t)entry.address_mid << 16) | (uint64_t)entry.address_low,
	      16);
	cursor.x = 0;
	cursor.y++;
}

void print_entry(GDT_entry entry) {
	print(entry.Limit0, 10);
	print(" ");
	print(entry.Base0, 10);
	print(" ");
	print(entry.Base1, 10);
	print(" ");
	print(entry.Accessbyte, 2);
	print(" ");
	print(entry.Limit1_Flags, 2);
	print(" ");
	print(entry.Base2, 10);
	print(" ");
	cursor.x = 0;
	cursor.y++;
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

static Idtrr idt_reg = {};
static Idtr idtr = {};
static GDT_table gdt_table = {};
static GDT_descriptor gdt_descriptor = {};

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
extern "C" void kmain(void) {
	for (uint64_t i = 0; i < 9999999999; i++) {
		asm("nop");
	}
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

	print_entry(gdt_table.entries[0]);
	print_entry(gdt_table.entries[1]);
	print_entry(gdt_table.entries[2]);
	print_entry(gdt_table.entries[3]);
	print_entry(gdt_table.entries[4]);
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
	cursor.x = 0;
	cursor.y++;
	print_entry(idtr.idts[0x6]);
	print_entry(idtr.idts[0x8]);
	print_entry(idtr.idts[0xd]);
	print_entry(idtr.idts[0xe]);
	asm("cli");
	cursor.x = 0;
	cursor.y = 0;
	for (size_t x = 0; x < framebuffer->width; x++) {
		for (size_t y = 0; y < framebuffer->height; y++) {
			volatile uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
			fb_ptr[y * framebuffer->width + x] = 0x000000; // 0x0000FF;
		}
	}
	asm("cli");
	load_idt(&idt_reg);
	// enable interupts
	asm("sti");

	// TODO: load physical memory in virtual memory for kernel(identity) 0
	// -> 0x100000000 write page map lvl 4/5(default 4) entry to %%cr3
	Physical_memory::initialize();
	Virtual_memory::initialize();

	volatile RSDP *rsdp = (volatile RSDP *)rsdp_request.response->address;
	uint64_t mem_entry_count = memmap_request.response->entry_count;
	limine_memmap_entry **entries = memmap_request.response->entries;
	clear_screen(0x1d1f21);
	print("count: ");
	print(mem_entry_count);
	cursor.y++;
	cursor.x = 0;
	uint64_t addr = (uint64_t)rsdp->Signature;
	for (uint64_t index = 0; index < mem_entry_count; index++) {
		limine_memmap_entry *entry = entries[index];
		if (!(entry->base <= addr && (entry->base + entry->length) >= addr)) {
			continue;
		}

		print("base: 0x");
		print(entry->base, 16);
		print(" length: ");
		print(entry->length);
		print(" type: ");
		print(entry->type);
		print(", ");
		cursor.y++;
		cursor.x = 0;
	}
	print("      0x");
	print((uint64_t)rsdp->Signature, 16);
	cursor.y++;
	cursor.x = 0;
	print("      0x");
	print((uint64_t)hhdm_request.response->offset, 16);
	cursor.y++;
	cursor.x = 0;
	print("header: ");
	print((char *)rsdp->Signature + hhdm_request.response->offset, 8);
	cursor.y++;
	cursor.x = 0;
	/*
	struct Idt {
	    uint16_t address_low;
	    uint16_t selector;
	    uint8_t ist;
	    uint8_t flags;
	    uint16_t address_mid;
	    uint32_t address_high;
	    uint32_t reserved;
	} __attribute__((packed));
	*/

	Idt t = idtr.idts[14];
	print(sizeof(t));
	print(" ");
	print(offsetof(Idt, address_low) * 8);
	print(" ");
	print(offsetof(Idt, selector) * 8);
	print(" ");
	print(offsetof(Idt, ist) * 8);
	print(" ");
	print(offsetof(Idt, flags) * 8);
	print(" ");
	print(offsetof(Idt, address_mid) * 8);
	print(" ");
	print(offsetof(Idt, address_high) * 8);
	print(" ");
	print(offsetof(Idt, reserved) * 8);

	*(int *)0 = 50;

	//	print((char *)rsdp->Signature, 8);

	// We're done, just hang...
	hcf();
}
