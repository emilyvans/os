#include "keyboard.hpp"
#include "asm.hpp"
#include "console.hpp"
#include <stdint.h>

bool key_active[255] = {false};

uint8_t key_set_1_translation_buffer[255][6];
uint8_t key_set_2_translation_buffer[255][8];
uint8_t key_set_3_translation_buffer[255][2];

uint8_t key_code_buffer[8] = {0};
uint8_t key_code_bytes_recieved = 0;
uint8_t key_set = 1;

void keyboard_handler() {
	uint8_t key_code_byte = inb(0x60);
	// TODO: implement PS/2 command
	// TODO: implement key code conversion
	// TODO: implement keyrepeating
	printf("0x%x ", key_code_byte);
}
