#include "keyboard.hpp"
#include "asm.hpp"
#include "console.hpp"
#include "keycode.hpp"
#include <stdint.h>

bool key_active[255] = {false};
bool command_active = false;

uint8_t key_set_1_translation_buffer[255][6] = {
	[key_code::escape_key] = {0x01, 0, 0, 0, 0, 0},
	[key_code::F1_key] = {0x3B, 0, 0, 0, 0, 0},
	[key_code::F2_key] = {0x3C, 0, 0, 0, 0, 0},
	[key_code::F3_key] = {0x3D, 0, 0, 0, 0, 0},
	[key_code::F4_key] = {0x3E, 0, 0, 0, 0, 0},
	[key_code::F5_key] = {0x3F, 0, 0, 0, 0, 0},
	[key_code::F6_key] = {0x40, 0, 0, 0, 0, 0},
	[key_code::F7_key] = {0x41, 0, 0, 0, 0, 0},
	[key_code::F8_key] = {0x42, 0, 0, 0, 0, 0},
	[key_code::F9_key] = {0x43, 0, 0, 0, 0, 0},
	[key_code::F10_key] = {0x44, 0, 0, 0, 0, 0},
	[key_code::F11_key] = {0x57, 0, 0, 0, 0, 0},
	[key_code::F12_key] = {0x58, 0, 0, 0, 0, 0},
	[key_code::print_screen_key] = {0xE0, 0x2A, 0xE0, 0x37, 0, 0},
	[key_code::scroll_lock_key] = {0x46, 0, 0, 0, 0, 0},
	[key_code::pause_key] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5},
	[key_code::back_tick_key] = {0x29, 0, 0, 0, 0, 0},
	[key_code::one_key] = {0x02, 0, 0, 0, 0, 0},
	[key_code::two_key] = {0x03, 0, 0, 0, 0, 0},
	[key_code::three_key] = {0x04, 0, 0, 0, 0, 0},
	[key_code::four_key] = {0x05, 0, 0, 0, 0, 0},
	[key_code::five_key] = {0x06, 0, 0, 0, 0, 0},
	[key_code::six_key] = {0x07, 0, 0, 0, 0, 0},
	[key_code::seven_key] = {0x08, 0, 0, 0, 0, 0},
	[key_code::eight_key] = {0x09, 0, 0, 0, 0, 0},
	[key_code::nine_key] = {0x0A, 0, 0, 0, 0, 0},
	[key_code::zero_key] = {0x0B, 0, 0, 0, 0, 0},
	[key_code::minus_key] = {0x0C, 0, 0, 0, 0, 0},
	[key_code::equels_key] = {0x0D, 0, 0, 0, 0, 0},
	[key_code::backspace_key] = {0x0E, 0, 0, 0, 0, 0},
	[key_code::insert_key] = {0xE0, 0x52, 0, 0, 0, 0},
	[key_code::home_key] = {0xE0, 0x47, 0, 0, 0, 0},
	[key_code::page_up_key] = {0x0E, 0x49, 0, 0, 0, 0},
	[key_code::number_lock_key] = {0x45, 0, 0, 0, 0, 0},
	[key_code::keypad_slash_key] = {0xE0, 0x35, 0, 0, 0, 0},
	[key_code::keypad_times_key] = {0x37, 0, 0, 0, 0, 0},
	[key_code::keypad_minus_key] = {0x4A, 0, 0, 0, 0, 0},
	[key_code::tab_key] = {0x0F, 0, 0, 0, 0, 0},
	[key_code::q_key] = {0x10, 0, 0, 0, 0, 0},
	[key_code::w_key] = {0x11, 0, 0, 0, 0, 0},
	[key_code::e_key] = {0x12, 0, 0, 0, 0, 0},
	[key_code::r_key] = {0x13, 0, 0, 0, 0, 0},
	[key_code::t_key] = {0x14, 0, 0, 0, 0, 0},
	[key_code::y_key] = {0x15, 0, 0, 0, 0, 0},
	[key_code::u_key] = {0x16, 0, 0, 0, 0, 0},
	[key_code::i_key] = {0x17, 0, 0, 0, 0, 0},
	[key_code::o_key] = {0x18, 0, 0, 0, 0, 0},
	[key_code::p_key] = {0x19, 0, 0, 0, 0, 0},
	[key_code::opening_square_bracket_key] = {0x1A, 0, 0, 0, 0, 0},
	[key_code::closing_square_bracket_key] = {0x1B, 0, 0, 0, 0, 0},
	[key_code::back_slash_key] = {0x2B, 0, 0, 0, 0, 0},
	[key_code::delete_key] = {0xE0, 0x53, 0, 0, 0, 0},
	[key_code::end_key] = {0xE0, 0x4F, 0, 0, 0, 0},
	[key_code::page_down_key] = {0xE0, 0x51, 0, 0, 0, 0},
	[key_code::numpad_seven_key] = {0x47, 0, 0, 0, 0, 0},
	[key_code::numpad_eight_key] = {0x48, 0, 0, 0, 0, 0},
	[key_code::numpad_nine_key] = {0x49, 0, 0, 0, 0, 0},
	[key_code::numpad_plus_key] = {0x4E, 0, 0, 0, 0, 0},
	[key_code::capslock_key] = {0x46, 0, 0, 0, 0, 0},
	[key_code::a_key] = {0x1E, 0, 0, 0, 0, 0},
	[key_code::s_key] = {0x1F, 0, 0, 0, 0, 0},
	[key_code::d_key] = {0x20, 0, 0, 0, 0, 0},
	[key_code::f_key] = {0x21, 0, 0, 0, 0, 0},
	[key_code::g_key] = {0x22, 0, 0, 0, 0, 0},
	[key_code::h_key] = {0x23, 0, 0, 0, 0, 0},
	[key_code::j_key] = {0x24, 0, 0, 0, 0, 0},
	[key_code::k_key] = {0x25, 0, 0, 0, 0, 0},
	[key_code::l_key] = {0x26, 0, 0, 0, 0, 0},
	[key_code::semi_colon_key] = {0x27, 0, 0, 0, 0, 0},
	[key_code::single_quote_key] = {0x28, 0, 0, 0, 0, 0},
	[key_code::enter_key] = {0x1C, 0, 0, 0, 0, 0},
	[key_code::numpad_four_key] = {0x4B, 0, 0, 0, 0, 0},
	[key_code::numpad_five_key] = {0x4C, 0, 0, 0, 0, 0},
	[key_code::numpad_six_key] = {0x4D, 0, 0, 0, 0, 0},
	[key_code::left_shift_key] = {0x2A, 0, 0, 0, 0, 0},
	[key_code::z_key] = {0x2C, 0, 0, 0, 0, 0},
	[key_code::x_key] = {0x2D, 0, 0, 0, 0, 0},
	[key_code::c_key] = {0x2E, 0, 0, 0, 0, 0},
	[key_code::v_key] = {0x2F, 0, 0, 0, 0, 0},
	[key_code::b_key] = {0x30, 0, 0, 0, 0, 0},
	[key_code::n_key] = {0x31, 0, 0, 0, 0, 0},
	[key_code::m_key] = {0x32, 0, 0, 0, 0, 0},
	[key_code::comma_key] = {0x33, 0, 0, 0, 0, 0},
	[key_code::period_key] = {0x34, 0, 0, 0, 0, 0},
	[key_code::forward_slash_key] = {0x35, 0, 0, 0, 0, 0},
	[key_code::right_shift_key] = {0x36, 0, 0, 0, 0, 0},
	[key_code::arrow_up_key] = {0xE0, 0x48, 0, 0, 0, 0},
	[key_code::numpad_one_key] = {0x4F, 0, 0, 0, 0, 0},
	[key_code::numpad_two_key] = {0x50, 0, 0, 0, 0, 0},
	[key_code::numpad_three_key] = {0x51, 0, 0, 0, 0, 0},
	[key_code::numpad_enter_key] = {0xE0, 0x1C, 0, 0, 0, 0},
	[key_code::left_control_key] = {0x1D, 0, 0, 0, 0, 0},
	[key_code::left_meta_key] = {0xE0, 0x5B, 0, 0, 0, 0},
	[key_code::left_alt_key] = {0x38, 0, 0, 0, 0, 0},
	[key_code::space_key] = {0x39, 0, 0, 0, 0, 0},
	[key_code::right_alt_key] = {0xE0, 0x38, 0, 0, 0, 0},
	[key_code::right_meta_key] = {0xE0, 0x5C, 0, 0, 0, 0},
	[key_code::right_control_key] = {0xE0, 0x1D, 0, 0, 0, 0},
	[key_code::arrow_left_key] = {0xE0, 0x4B, 0, 0, 0, 0},
	[key_code::arrow_down_key] = {0xE0, 0x50, 0, 0, 0, 0},
	[key_code::arrow_right_key] = {0xE0, 0x4D, 0, 0, 0, 0},
	[key_code::numpad_zero_key] = {0x52, 0, 0, 0, 0, 0},
	[key_code::numpad_period_key] = {0x53, 0, 0, 0, 0, 0},
};

uint8_t key_set_2_translation_buffer[255][8] = {
	[key_code::escape_key] = {},
	[key_code::F1_key] = {},
	[key_code::F2_key] = {},
	[key_code::F3_key] = {},
	[key_code::F4_key] = {},
	[key_code::F5_key] = {},
	[key_code::F6_key] = {},
	[key_code::F7_key] = {},
	[key_code::F8_key] = {},
	[key_code::F9_key] = {},
	[key_code::F10_key] = {},
	[key_code::F11_key] = {},
	[key_code::F12_key] = {},
	[key_code::print_screen_key] = {},
	[key_code::scroll_lock_key] = {},
	[key_code::pause_key] = {},
	[key_code::back_tick_key] = {},
	[key_code::one_key] = {},
	[key_code::two_key] = {},
	[key_code::three_key] = {},
	[key_code::four_key] = {},
	[key_code::five_key] = {},
	[key_code::six_key] = {},
	[key_code::seven_key] = {},
	[key_code::eight_key] = {},
	[key_code::nine_key] = {},
	[key_code::zero_key] = {},
	[key_code::minus_key] = {},
	[key_code::equels_key] = {},
	[key_code::backspace_key] = {},
	[key_code::insert_key] = {},
	[key_code::home_key] = {},
	[key_code::page_up_key] = {},
	[key_code::number_lock_key] = {},
	[key_code::keypad_slash_key] = {},
	[key_code::keypad_times_key] = {},
	[key_code::keypad_minus_key] = {},
	[key_code::tab_key] = {},
	[key_code::q_key] = {},
	[key_code::w_key] = {},
	[key_code::e_key] = {},
	[key_code::r_key] = {},
	[key_code::t_key] = {},
	[key_code::y_key] = {},
	[key_code::u_key] = {},
	[key_code::i_key] = {},
	[key_code::o_key] = {},
	[key_code::p_key] = {},
	[key_code::opening_square_bracket_key] = {},
	[key_code::closing_square_bracket_key] = {},
	[key_code::back_slash_key] = {},
	[key_code::delete_key] = {},
	[key_code::end_key] = {},
	[key_code::page_down_key] = {},
	[key_code::numpad_seven_key] = {},
	[key_code::numpad_eight_key] = {},
	[key_code::numpad_nine_key] = {},
	[key_code::numpad_plus_key] = {},
	[key_code::capslock_key] = {},
	[key_code::a_key] = {},
	[key_code::s_key] = {},
	[key_code::d_key] = {},
	[key_code::f_key] = {},
	[key_code::g_key] = {},
	[key_code::h_key] = {},
	[key_code::j_key] = {},
	[key_code::k_key] = {},
	[key_code::l_key] = {},
	[key_code::semi_colon_key] = {},
	[key_code::single_quote_key] = {},
	[key_code::enter_key] = {},
	[key_code::numpad_four_key] = {},
	[key_code::numpad_five_key] = {},
	[key_code::numpad_six_key] = {},
	[key_code::left_shift_key] = {},
	[key_code::z_key] = {},
	[key_code::x_key] = {},
	[key_code::c_key] = {},
	[key_code::v_key] = {},
	[key_code::b_key] = {},
	[key_code::n_key] = {},
	[key_code::m_key] = {},
	[key_code::comma_key] = {},
	[key_code::period_key] = {},
	[key_code::forward_slash_key] = {},
	[key_code::right_shift_key] = {},
	[key_code::arrow_up_key] = {},
	[key_code::numpad_one_key] = {},
	[key_code::numpad_two_key] = {},
	[key_code::numpad_three_key] = {},
	[key_code::numpad_enter_key] = {},
	[key_code::left_control_key] = {},
	[key_code::left_meta_key] = {},
	[key_code::left_alt_key] = {},
	[key_code::space_key] = {},
	[key_code::right_alt_key] = {},
	[key_code::right_meta_key] = {},
	[key_code::right_control_key] = {},
	[key_code::arrow_left_key] = {},
	[key_code::arrow_down_key] = {},
	[key_code::arrow_right_key] = {},
	[key_code::numpad_zero_key] = {},
	[key_code::numpad_period_key] = {},

};
uint8_t key_set_3_translation_buffer[255][2] = {
	[key_code::escape_key] = {},
	[key_code::F1_key] = {},
	[key_code::F2_key] = {},
	[key_code::F3_key] = {},
	[key_code::F4_key] = {},
	[key_code::F5_key] = {},
	[key_code::F6_key] = {},
	[key_code::F7_key] = {},
	[key_code::F8_key] = {},
	[key_code::F9_key] = {},
	[key_code::F10_key] = {},
	[key_code::F11_key] = {},
	[key_code::F12_key] = {},
	[key_code::print_screen_key] = {},
	[key_code::scroll_lock_key] = {},
	[key_code::pause_key] = {},
	[key_code::back_tick_key] = {},
	[key_code::one_key] = {},
	[key_code::two_key] = {},
	[key_code::three_key] = {},
	[key_code::four_key] = {},
	[key_code::five_key] = {},
	[key_code::six_key] = {},
	[key_code::seven_key] = {},
	[key_code::eight_key] = {},
	[key_code::nine_key] = {},
	[key_code::zero_key] = {},
	[key_code::minus_key] = {},
	[key_code::equels_key] = {},
	[key_code::backspace_key] = {},
	[key_code::insert_key] = {},
	[key_code::home_key] = {},
	[key_code::page_up_key] = {},
	[key_code::number_lock_key] = {},
	[key_code::keypad_slash_key] = {},
	[key_code::keypad_times_key] = {},
	[key_code::keypad_minus_key] = {},
	[key_code::tab_key] = {},
	[key_code::q_key] = {},
	[key_code::w_key] = {},
	[key_code::e_key] = {},
	[key_code::r_key] = {},
	[key_code::t_key] = {},
	[key_code::y_key] = {},
	[key_code::u_key] = {},
	[key_code::i_key] = {},
	[key_code::o_key] = {},
	[key_code::p_key] = {},
	[key_code::opening_square_bracket_key] = {},
	[key_code::closing_square_bracket_key] = {},
	[key_code::back_slash_key] = {},
	[key_code::delete_key] = {},
	[key_code::end_key] = {},
	[key_code::page_down_key] = {},
	[key_code::numpad_seven_key] = {},
	[key_code::numpad_eight_key] = {},
	[key_code::numpad_nine_key] = {},
	[key_code::numpad_plus_key] = {},
	[key_code::capslock_key] = {},
	[key_code::a_key] = {},
	[key_code::s_key] = {},
	[key_code::d_key] = {},
	[key_code::f_key] = {},
	[key_code::g_key] = {},
	[key_code::h_key] = {},
	[key_code::j_key] = {},
	[key_code::k_key] = {},
	[key_code::l_key] = {},
	[key_code::semi_colon_key] = {},
	[key_code::single_quote_key] = {},
	[key_code::enter_key] = {},
	[key_code::numpad_four_key] = {},
	[key_code::numpad_five_key] = {},
	[key_code::numpad_six_key] = {},
	[key_code::left_shift_key] = {},
	[key_code::z_key] = {},
	[key_code::x_key] = {},
	[key_code::c_key] = {},
	[key_code::v_key] = {},
	[key_code::b_key] = {},
	[key_code::n_key] = {},
	[key_code::m_key] = {},
	[key_code::comma_key] = {},
	[key_code::period_key] = {},
	[key_code::forward_slash_key] = {},
	[key_code::right_shift_key] = {},
	[key_code::arrow_up_key] = {},
	[key_code::numpad_one_key] = {},
	[key_code::numpad_two_key] = {},
	[key_code::numpad_three_key] = {},
	[key_code::numpad_enter_key] = {},
	[key_code::left_control_key] = {},
	[key_code::left_meta_key] = {},
	[key_code::left_alt_key] = {},
	[key_code::space_key] = {},
	[key_code::right_alt_key] = {},
	[key_code::right_meta_key] = {},
	[key_code::right_control_key] = {},
	[key_code::arrow_left_key] = {},
	[key_code::arrow_down_key] = {},
	[key_code::arrow_right_key] = {},
	[key_code::numpad_zero_key] = {},
	[key_code::numpad_period_key] = {},

};

uint8_t key_code_buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t key_code_bytes_recieved = 0;
uint8_t key_set = 1;

// TODO: implement PS/2 command
void send_ps2_command() {
	command_active = true;
}

void handle_ps2_command() {
	command_active = false;
}

bool is_break_code(uint8_t key_code) {
	bool status = false;
	for (uint8_t i = 0; i < 8; i++) {
		if (key_code_buffer[i] == 0) {
			break;
		}
	}
	return status;
}

void print_key_name(uint8_t key_code) {
	switch (key_code) {
	case key_code::escape_key:
		printf("escape");
		break;
	case key_code::F1_key:
		printf("F1");
		break;
	case key_code::F2_key:
		printf("F2");
		break;
	case key_code::F3_key:
		printf("F3");
		break;
	case key_code::F4_key:
		printf("F4");
		break;
	case key_code::F5_key:
		printf("F5");
		break;
	case key_code::F6_key:
		printf("F6");
		break;
	case key_code::F7_key:
		printf("F7");
		break;
	case key_code::F8_key:
		printf("F8");
		break;
	case key_code::F9_key:
		printf("F9");
		break;
	case key_code::F10_key:
		printf("F10");
		break;
	case key_code::F11_key:
		printf("F11");
		break;
	case key_code::F12_key:
		printf("F12");
		break;
	case key_code::print_screen_key:
		printf("print_screen");
		break;
	case key_code::scroll_lock_key:
		printf("scroll_lock");
		break;
	case key_code::pause_key:
		printf("pause");
		break;
	case key_code::back_tick_key:
		printf("back_tick");
		break;
	case key_code::one_key:
		printf("one");
		break;
	case key_code::two_key:
		printf("two");
		break;
	case key_code::three_key:
		printf("three");
		break;
	case key_code::four_key:
		printf("four");
		break;
	case key_code::five_key:
		printf("five");
		break;
	case key_code::six_key:
		printf("six");
		break;
	case key_code::seven_key:
		printf("seven");
		break;
	case key_code::eight_key:
		printf("eight");
		break;
	case key_code::nine_key:
		printf("nine");
		break;
	case key_code::zero_key:
		printf("zero");
		break;
	case key_code::minus_key:
		printf("minus");
		break;
	case key_code::equels_key:
		printf("equels");
		break;
	case key_code::backspace_key:
		printf("backspace");
		break;
	case key_code::insert_key:
		printf("insert");
		break;
	case key_code::home_key:
		printf("home");
		break;
	case key_code::page_up_key:
		printf("page_up");
		break;
	case key_code::number_lock_key:
		printf("number_lock");
		break;
	case key_code::keypad_slash_key:
		printf("keypad_slash");
		break;
	case key_code::keypad_times_key:
		printf("keypad_times");
		break;
	case key_code::keypad_minus_key:
		printf("keypad_minus");
		break;
	case key_code::tab_key:
		printf("tab");
		break;
	case key_code::q_key:
		printf("q");
		break;
	case key_code::w_key:
		printf("w");
		break;
	case key_code::e_key:
		printf("e");
		break;
	case key_code::r_key:
		printf("r");
		break;
	case key_code::t_key:
		printf("t");
		break;
	case key_code::y_key:
		printf("y");
		break;
	case key_code::u_key:
		printf("u");
		break;
	case key_code::i_key:
		printf("i");
		break;
	case key_code::o_key:
		printf("o");
		break;
	case key_code::p_key:
		printf("p");
		break;
	case key_code::opening_square_bracket_key:
		printf("opening_square_bracket");
		break;
	case key_code::closing_square_bracket_key:
		printf("closing_square_bracket");
		break;
	case key_code::back_slash_key:
		printf("back_slash");
		break;
	case key_code::delete_key:
		printf("delete");
		break;
	case key_code::end_key:
		printf("end");
		break;
	case key_code::page_down_key:
		printf("page_down");
		break;
	case key_code::numpad_seven_key:
		printf("numpad_seven");
		break;
	case key_code::numpad_eight_key:
		printf("numpad_eight");
		break;
	case key_code::numpad_nine_key:
		printf("numpad_nine");
		break;
	case key_code::numpad_plus_key:
		printf("numpad_plus");
		break;
	case key_code::capslock_key:
		printf("capslock");
		break;
	case key_code::a_key:
		printf("a");
		break;
	case key_code::s_key:
		printf("s");
		break;
	case key_code::d_key:
		printf("d");
		break;
	case key_code::f_key:
		printf("f");
		break;
	case key_code::g_key:
		printf("g");
		break;
	case key_code::h_key:
		printf("h");
		break;
	case key_code::j_key:
		printf("j");
		break;
	case key_code::k_key:
		printf("k");
		break;
	case key_code::l_key:
		printf("l");
		break;
	case key_code::semi_colon_key:
		printf("semi_colon");
		break;
	case key_code::single_quote_key:
		printf("single_quote");
		break;
	case key_code::enter_key:
		printf("enter");
		break;
	case key_code::numpad_four_key:
		printf("numpad_four");
		break;
	case key_code::numpad_five_key:
		printf("numpad_five");
		break;
	case key_code::numpad_six_key:
		printf("numpad_six");
		break;
	case key_code::left_shift_key:
		printf("left_shift");
		break;
	case key_code::z_key:
		printf("z");
		break;
	case key_code::x_key:
		printf("x");
		break;
	case key_code::c_key:
		printf("c");
		break;
	case key_code::v_key:
		printf("v");
		break;
	case key_code::b_key:
		printf("b");
		break;
	case key_code::n_key:
		printf("n");
		break;
	case key_code::m_key:
		printf("m");
		break;
	case key_code::comma_key:
		printf("comma");
		break;
	case key_code::period_key:
		printf("period");
		break;
	case key_code::forward_slash_key:
		printf("forward_slash");
		break;
	case key_code::right_shift_key:
		printf("right_shift");
		break;
	case key_code::arrow_up_key:
		printf("arrow_up");
		break;
	case key_code::numpad_one_key:
		printf("numpad_one");
		break;
	case key_code::numpad_two_key:
		printf("numpad_two");
		break;
	case key_code::numpad_three_key:
		printf("numpad_three");
		break;
	case key_code::numpad_enter_key:
		printf("numpad_enter");
		break;
	case key_code::left_control_key:
		printf("left_control");
		break;
	case key_code::left_meta_key:
		printf("left_meta");
		break;
	case key_code::left_alt_key:
		printf("left_alt");
		break;
	case key_code::space_key:
		printf("space");
		break;
	case key_code::right_alt_key:
		printf("right_alt");
		break;
	case key_code::right_meta_key:
		printf("right_meta");
		break;
	case key_code::right_control_key:
		printf("right_control");
		break;
	case key_code::arrow_left_key:
		printf("arrow_left");
		break;
	case key_code::arrow_down_key:
		printf("arrow_down");
		break;
	case key_code::arrow_right_key:
		printf("arrow_right");
		break;
	case key_code::numpad_zero_key:
		printf("numpad_zero");
		break;
	case key_code::numpad_period_key:
		printf("numpad_period");
		break;
	}
}

void clear_buffer() {
	key_code_buffer[0] = 0;
	key_code_buffer[1] = 0;
	key_code_buffer[2] = 0;
	key_code_buffer[3] = 0;
	key_code_buffer[4] = 0;
	key_code_buffer[5] = 0;
	key_code_buffer[6] = 0;
	key_code_buffer[7] = 0;
	key_code_bytes_recieved = 0;
}
/*
 * print_screen make
 * left shift make
 * print screen make
 * keypad time make
 * print_screen make
 * keypad times break
 * printscreen make
 * leftshift break
 *
 */
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
			bool is_break = false;
			for (uint64_t j = 0; j < 6; j++) { // max loop over 6 bytes
				if (key_code_byte == 0) {
					break;
				}
				if (key_bytes[j] != key_code_buffer[j]) {
					uint8_t byte_index = 0;
					if (key_code_bytes_recieved == 4) {
						if (j == 1) {
							byte_index = 3;
						} else if (j == 3) {
							byte_index = 1;
						} else {
							byte_index = j;
						}
					} else {
						byte_index = j;
					}
					if (((key_code_buffer[j] & ~0x80) ==
					     key_bytes[byte_index])) {
						is_break = true;
					} else {
						is_key = false;
						break;
					}
				}
			}
			if (is_key) {
				if (is_break) {
					key_active[i] = false;
					print_key_name(i);
					printf(" break\n");
				} else {
					key_active[i] = true;
					print_key_name(i);
					printf(" make\n");
				}
				clear_buffer();
				break;
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
#if 0
	clear_console();
	if (key_code_byte == 0x81) {
		key_code_buffer[0] = 0;
		key_code_buffer[1] = 0;
		key_code_buffer[2] = 0;
		key_code_buffer[3] = 0;
		key_code_buffer[4] = 0;
		key_code_buffer[5] = 0;
		key_code_buffer[6] = 0;
		key_code_buffer[7] = 0;
		key_code_bytes_recieved = 0;
	}
	uint8_t byte_1 = key_code_buffer[0];
	uint8_t byte_2 = key_code_buffer[1];
	uint8_t byte_3 = key_code_buffer[2];
	uint8_t byte_4 = key_code_buffer[3];
	uint8_t byte_5 = key_code_buffer[4];
	uint8_t byte_6 = key_code_buffer[5];
	uint8_t byte_7 = key_code_buffer[6];
	uint8_t byte_8 = key_code_buffer[7];

	printf("0: 0x%x\n1: 0x%x\n2: 0x%x\n3: 0x%x\n4: 0x%x\n5: 0x%x\n6: 0x%x\n7: "
	       "0x%x\n",
	       (uint64_t)byte_1, (uint64_t)byte_2, (uint64_t)byte_3,
	       (uint64_t)byte_4, (uint64_t)byte_5, (uint64_t)byte_6,
	       (uint64_t)byte_7, (uint64_t)byte_8);
#endif
}
