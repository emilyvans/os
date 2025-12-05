#ifndef INCLUDE_KEYBOARD_KEYBOARD_HPP_
#define INCLUDE_KEYBOARD_KEYBOARD_HPP_
#include "stdint.h"

bool set_key_pressed_handler(void (*handler)(uint8_t key_code));
bool set_key_released_handler(void (*handler)(uint8_t key_code));
void reset_key_pressed_handler();
void reset_key_released_handler();

void ps2_handler();
void ps2_on_interrupt();
void ps2_disable_keyset_translation();
void ps2_keyboard_handler();
void ps2_send_command(uint8_t command, uint8_t data);
void ps2_keyboard_set_keyset(uint8_t key_set);
void ps2_keyboard_get_current_keyset();
void ps2_flush_keycode_buffer();

#endif // INCLUDE_KEYBOARD_KEYBOARD_HPP_
