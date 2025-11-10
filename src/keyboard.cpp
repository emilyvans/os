#include "keyboard.hpp"
#include "asm.hpp"
#include "console.hpp"
#include <stdint.h>

bool key_active[255] = {false};
bool command_active = false;

uint8_t key_set_1_translation_buffer[255][6];
uint8_t key_set_2_translation_buffer[255][8];
uint8_t key_set_3_translation_buffer[255][2];

uint8_t key_code_buffer[8] = {0};
uint8_t key_code_bytes_recieved = 0;
uint8_t key_set = 1;

// TODO: implement PS/2 command
void send_ps2_command() {
	command_active = true;
}

void handle_ps2_command() {
	command_active = false;
}

bool is_break_code() {
	return false;
}

void keyboard_handler() {
	uint8_t key_code_byte = inb(0x60);
	// TODO: implement key code conversion
	// TODO: implement keyrepeating
	if (command_active) {
		handle_ps2_command();
		return;
	}

	key_code_buffer[key_code_bytes_recieved] = key_code_byte;
	key_code_bytes_recieved += 1;

	if (key_set == 1) {
		for (uint64_t i = 0; i < 256; i++) {
			uint8_t *key_bytes = key_set_1_translation_buffer[i];
			bool is_key = true;
			for (uint64_t j = 0; j < 6; j++) {
				if (key_code_byte == 0) {
					break;
				}
				if (key_bytes[j] != key_code_buffer[j]) {
					is_key = false;
					break;
				}
			}
			if (is_key) {
				if (is_break_code()) {
					key_active[i] = false;
				} else {
					key_active[i] = true;
				}
			}
		}
	} else if (key_set == 2) {
		for (uint64_t i = 0; i < 256; i++) {
			for (uint64_t j = 0; j < 8; j++) {
			}
		}
	} else if (key_set == 3) {
		for (uint64_t i = 0; i < 256; i++) {
			for (uint64_t j = 0; j < 2; j++) {
			}
		}
	} else {
		printf("key set not supported: %d", key_set);
		return;
	}
	printf("0x%x ", key_code_byte);
}
