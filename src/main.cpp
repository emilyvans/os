#include "cpu/GDT.hpp"
#include "cpu/asm.hpp"
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
	limine_base_revision[] = LIMINE_BASE_REVISION(4);

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.
__attribute__((
	used,
	section(".limine_requests_"
            "start"))) static volatile uint64_t limine_requests_start_marker[] =
	LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
	limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

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
	printf("general protation\nRIP=%x CS=%x RFLAGS=%x\nerror=%b\n", frame->rip,
	       frame->cs, frame->flags, frame->error_code);
	hcf();
}
void iop_handler(InterruptFrame *frame) {
	(void)frame; // to remove warning
	clear_console();
	printf("invalid opcode\n");
	printf("RIP: 0x%x\n", frame->rip);
	printf("opcode byte 1: 0x%x", *(uint8_t *)frame->rip);
	hcf();
}

volatile uint64_t miliseconds = 0;

void PIC_timer_handler() {
	// printf("timer\n");
	miliseconds += 1;
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

void sleep(uint64_t ms) {

	uint64_t start_time = miliseconds;

	while ((miliseconds - start_time) < ms) {
		asm volatile("hlt");
	}

	return;
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

	physicalmemory::initialize();
	virtualmemory::initialize();

	printf("total Memory: %uMiB\nfree memory:  %uMiB\n",
	       physicalmemory::get_total_ram() / 1024 / 1024,
	       physicalmemory::get_free_ram() / 1024 / 1024);

	// ps2_keyboard_get_current_keyset();
	// ps2_flush_keycode_buffer();

	// init_ACPI();
	// init_shell();

// Programmable interrupt timer setup
#define PIT_CHANNEL_0 0x40
#define PIT_CHANNEL_1 0x41
#define PIT_CHANNEL_2 0x42
#define PIT_MODE_COMMAND_REGISTER 0x43
	outb(PIT_MODE_COMMAND_REGISTER, 0b00110100);

	uint64_t PIT_FREQ = 1193182; // 1.193182MHZ
	uint16_t freq = 1000;
	uint16_t freq_divider = PIT_FREQ / freq;
	outb(PIT_CHANNEL_0, freq_divider & 0xFF);
	outb(PIT_CHANNEL_0, (freq_divider & 0xFF00) >> 8);

	PIC_unmask_interrupt(0);
	// while (miliseconds < 10000) {
	// };
	//  convert this to a non busy-loop
	for (;;) {
		// void *mem = page_alloc(255);
		// printf("total Memory: %uMiB\nfree memory:  %uMiB\n",
		//        physicalmemory::get_total_ram() / 1024 / 1024,
		//        physicalmemory::get_free_ram() / 1024 / 1024);
		//  ps2_handler();
		//  shell_loop();
		for (uint16_t i = 0; i < 10; i++) {
			asm volatile("hlt");
		}
	}
}
