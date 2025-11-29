#include "driver/pic.hpp"
#include "cpu/asm.hpp"
#include <stdint.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xA0
#define PIC2_DATA (PIC2_COMMAND + 1)
#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define ICW4_8086 0x01

#define CASCADE_IRQ 2

void init_PIC() {
	asm("cli");
	outb(PIC1_COMMAND,
	     ICW1_INIT |
	         ICW1_ICW4); // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, 0x20); // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, 0x28); // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 1 << CASCADE_IRQ); // ICW3: tell Master PIC that there is a
	io_wait();
	// slave PIC at IRQ2
	outb(PIC2_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

	outb(PIC1_DATA,
	     ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Unmask both PICs.
	outb(PIC1_DATA, 1);
	outb(PIC2_DATA, 0);
	asm("sti");
}

void PIC_send_EOI(uint8_t interrupt_number) {
	if (interrupt_number >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
	io_wait();
}
