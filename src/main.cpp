#include "cpu/GDT.hpp"
#include "cpu/interrupts.hpp"
#include "driver/acpi.hpp"
#include "driver/console.hpp"
#include "driver/init.hpp"
#include "driver/keyboard/keyboard.hpp"
#include "driver/pic.hpp"
#include "driver/uart.hpp"
#include "limine/limine_requests.hpp"
#include "memory/physical_memory.hpp"
#include "memory/virtual_memory.hpp"
#include "panic.hpp"
#include "programs/shell.hpp"
#include "stdbool.h"
#include "stddef.h"
#include <limine.h>
#include <stdint.h>

#define background_color 0x1d1f21

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
	limine_base_revision[] = LIMINE_BASE_REVISION(3);

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((
	used,
	section(".limine_requests_"
            "start"))) static volatile uint64_t limine_requests_start_marker[] =
	LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
	limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

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
	(void)frame; // to remove warning
	clear_console();
	printf("double fault\n");
	hcf();
}
void gp_handler(InterruptFrame *frame) {
	(void)frame; // to remove warning
	// clear_console();
	printf("general protation\n");
	hcf();
}
void iop_handler(InterruptFrame *frame) {
	(void)frame; // to remove warning
	clear_console();
	printf("invalid opcode\n");
	hcf();
}

void PIC_timer_handler() {
	printf("timer\n");
	for (uint64_t i = 0; i < 9999999; i++) {
	}
	PIC_send_EOI(0);
}

void keyboard_interrupt_handler() {
	ps2_on_interrupt();
	PIC_send_EOI(1);
}

void unknown_handler(InterruptFrame *frame) {
	clear_console();
	printf("unknow exception\nnumber: %x\n", frame->interupt_nr);
	hcf();
}

void interrupt_handler(InterruptFrame *frame) {

	if (frame->interupt_nr == 0x27) {
		uint8_t isr = PIC_get_master_isr();
		if (!(isr & (1 << 7))) {
			return;
		}
	} else if (frame->interupt_nr == 0x2F) {
		uint8_t isr = PIC_get_slave_isr();
		if (!(isr & (1 << 7))) {
			PIC_send_master_EOI();
			return;
		}
	}

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
		PIC_timer_handler();
		break;
	case 0x21:
		keyboard_interrupt_handler();
		break;
	default:
		unknown_handler(frame);
	}
}

extern "C" void kmain(void) {
	if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
		hcf();
	}

	if (framebuffer_request.response == NULL ||
	    framebuffer_request.response->framebuffer_count < 1) {
		hcf();
	}

	if (hhdm_request.response == NULL) {
		hcf();
	}

	limine_framebuffer *framebuffer =
		framebuffer_request.response->framebuffers[0];

	uint32_t scale = 2;

	init_display(framebuffer, background_color, scale);
	uart_init();

	init_GDT();
	init_IDT();
	init_PIC();
	// ps2_keyboard_get_current_keyset();
	ps2_flush_keycode_buffer();

	/*
	    disable_PIC();
	*/

	physicalmemory::initialize();
	virtualmemory::initialize();

	init_ACPI();
	init_shell();

	// convert this to a non busy-loop
	for (;;) {
		ps2_handler();
		shell_loop();
	}
}
