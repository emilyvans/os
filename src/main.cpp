#include "GDT.hpp"
#include "acpi.hpp"
#include "asm.hpp"
#include "console.hpp"
#include "init.hpp"
#include "interrupts.hpp"
#include "limine.h"
#include "limine_requests.hpp"
#include "panic.hpp"
#include "physical_memory.hpp"
#include "pic.hpp"
#include "screen.hpp"
#include "utils.hpp"
#include "virtual_memory.hpp"
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

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((used,
               section(".limine_requests_"
                       "start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
	used,
	section(
		".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

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
/*
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
}*/

void print_entry(IDTEntry entry) {
	clear_console();
	printf("%x %u %b %b %x %x %u 0x%x\n", entry.address_low, entry.selector,
	       entry.ist, entry.flags, entry.address_mid, entry.address_high,
	       entry.reserved,
	       ((uint64_t)entry.address_high << 32) |
	           ((uint64_t)entry.address_mid << 16) |
	           (uint64_t)entry.address_low);
}

void print_entry(GDTEntry entry) {
	clear_console();
	printf("%x %x %x %b %b %x\n", entry.Limit0, entry.Base0, entry.Base1,
	       entry.Accessbyte, entry.Limit1_Flags, entry.Base2);
}

void pf_handler(InterruptFrame *frame) {
	// clear_screen(background_color);
	// clear_console();
	uint64_t fault_address;

	// CR2 contains the linear address that caused the fault
	asm volatile("mov %%cr2, %0" : "=r"(fault_address));
	printf("\npage fault\ntype: %x\nip: %x\ncs: %x\nflags: %x\nsp: %x\nss: "
	       "%x\nerror code: %x\noffending address: 0x%x",
	       0xE, frame->rip, frame->cs, frame->flags, frame->sp, frame->ss,
	       frame->error_code, fault_address);
	hcf();
}
void double_fault_handler(InterruptFrame *frame) {
	clear_console();
	(void)frame; // to remove warning
	printf("double fault\n");
	hcf();
}
void gp_handler(InterruptFrame *frame) {
	// clear_console();
	(void)frame; // to remove warning
	printf("general protation\n");
	hcf();
}
void iop_handler(InterruptFrame *frame) {
	clear_console();
	(void)frame; // to remove warning
	printf("invalid opcode\n");
	hcf();
}

void PIC_timer_handler(InterruptFrame *frame) {
	printf("timer\n");
	for (uint64_t i = 0; i < 9999999; i++) {
	}
	PIC_send_EOI(0);
}

void keyboard_interrupt_handler(InterruptFrame *frame) {
	uint8_t key_code_byte = inb(0x60);
	printf("0x%x ", key_code_byte);
	PIC_send_EOI(1);
}

void unknown_handler(InterruptFrame *frame) {
	clear_console();
	printf("unknow exception\nnumber: %x\n", frame->interupt_nr);
	hcf();
}

void interrupt_handler(InterruptFrame *frame) {
	switch (frame->interupt_nr) {
	case 0x6:
		iop_handler(frame);
		break;
	case 0x8:
		double_fault_handler(frame);
		break;
	case 0xD:
		gp_handler(frame);
		break;
	case 0xE:
		pf_handler(frame);
		break;
	case 0x20:
		PIC_timer_handler(frame);
		break;
	case 0x21:
		keyboard_interrupt_handler(frame);
		break;
	default:
		unknown_handler(frame);
	}
}

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
	limine_framebuffer *framebuffer =
		framebuffer_request.response->framebuffers[0];

	uint32_t scale = 2;

	init_display(framebuffer, background_color, scale);

	init_GDT();
	init_IDT();

	// init_PIC();
	/*
	    disable_PIC();
	*/

	physicalmemory::initialize();
	virtualmemory::initialize();

	printf("ttt\n");

	init_ACPI();
	/*
	    asm volatile("mov $60, %%rax\n\t"   // sys_exit
	                 "xor %%rdi, %%rdi\n\t" // exit(0)
	                 "syscall"
	                 :
	                 :
	                 : "%rax", "%rdi");*/

	// We're done, just hang...
	hcf();
}
