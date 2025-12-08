#include "programs/shell.hpp"
#include "driver/console.hpp"
#include "driver/keyboard/keyboard.hpp"
#include "driver/keyboard/keycode.hpp"
#include <stdint.h>

bool is_printable(uint8_t keycode) {
	switch (keycode) {
	case key_code::one_key:
	case key_code::two_key:
	case key_code::three_key:
	case key_code::four_key:
	case key_code::five_key:
	case key_code::six_key:
	case key_code::seven_key:
	case key_code::eight_key:
	case key_code::nine_key:
	case key_code::zero_key:
	case key_code::minus_key:
	case key_code::equals_key:
	case key_code::keypad_slash_key:
	case key_code::keypad_times_key:
	case key_code::keypad_minus_key:
	case key_code::q_key:
	case key_code::w_key:
	case key_code::e_key:
	case key_code::r_key:
	case key_code::t_key:
	case key_code::y_key:
	case key_code::u_key:
	case key_code::i_key:
	case key_code::o_key:
	case key_code::p_key:
	case key_code::opening_square_bracket_key:
	case key_code::closing_square_bracket_key:
	case key_code::back_slash_key:
	case key_code::numpad_seven_key:
	case key_code::numpad_eight_key:
	case key_code::numpad_nine_key:
	case key_code::numpad_plus_key:
	case key_code::a_key:
	case key_code::s_key:
	case key_code::d_key:
	case key_code::f_key:
	case key_code::g_key:
	case key_code::h_key:
	case key_code::j_key:
	case key_code::k_key:
	case key_code::l_key:
	case key_code::semi_colon_key:
	case key_code::single_quote_key:
	case key_code::enter_key:
	case key_code::numpad_four_key:
	case key_code::numpad_five_key:
	case key_code::numpad_six_key:
	case key_code::z_key:
	case key_code::x_key:
	case key_code::c_key:
	case key_code::v_key:
	case key_code::b_key:
	case key_code::n_key:
	case key_code::m_key:
	case key_code::comma_key:
	case key_code::period_key:
	case key_code::forward_slash_key:
	case key_code::numpad_one_key:
	case key_code::numpad_two_key:
	case key_code::numpad_three_key:
	case key_code::numpad_enter_key:
	case key_code::space_key:
	case key_code::numpad_zero_key:
	case key_code::numpad_period_key:
		return true;
	}
	return false;
}

char keycode_to_char(uint8_t keycode) {
	switch (keycode) {
	case key_code::one_key:
		return '1';
	case key_code::two_key:
		return '2';
	case key_code::three_key:
		return '3';
	case key_code::four_key:
		return '4';
	case key_code::five_key:
		return '5';
	case key_code::six_key:
		return '6';
	case key_code::seven_key:
		return '7';
	case key_code::eight_key:
		return '8';
	case key_code::nine_key:
		return '9';
	case key_code::zero_key:
		return '0';
	case key_code::minus_key:
		return '-';
	case key_code::equals_key:
		return '=';
	case key_code::keypad_slash_key:
		return '/';
	case key_code::keypad_times_key:
		return '*';
	case key_code::keypad_minus_key:
		return '-';
	case key_code::q_key:
		return 'q';
	case key_code::w_key:
		return 'w';
	case key_code::e_key:
		return 'e';
	case key_code::r_key:
		return 'r';
	case key_code::t_key:
		return 't';
	case key_code::y_key:
		return 'y';
	case key_code::u_key:
		return 'u';
	case key_code::i_key:
		return 'i';
	case key_code::o_key:
		return 'o';
	case key_code::p_key:
		return 'p';
	case key_code::opening_square_bracket_key:
		return '[';
	case key_code::closing_square_bracket_key:
		return ']';
	case key_code::back_slash_key:
		return '\\';
	case key_code::numpad_seven_key:
		return '7';
	case key_code::numpad_eight_key:
		return '8';
	case key_code::numpad_nine_key:
		return '9';
	case key_code::numpad_plus_key:
		return '+';
	case key_code::a_key:
		return 'a';
	case key_code::s_key:
		return 's';
	case key_code::d_key:
		return 'd';
	case key_code::f_key:
		return 'f';
	case key_code::g_key:
		return 'g';
	case key_code::h_key:
		return 'h';
	case key_code::j_key:
		return 'j';
	case key_code::k_key:
		return 'k';
	case key_code::l_key:
		return 'l';
	case key_code::semi_colon_key:
		return ';';
	case key_code::single_quote_key:
		return '\'';
	case key_code::enter_key:
		return '\n';
	case key_code::numpad_four_key:
		return '4';
	case key_code::numpad_five_key:
		return '5';
	case key_code::numpad_six_key:
		return '6';
	case key_code::z_key:
		return 'z';
	case key_code::x_key:
		return 'x';
	case key_code::c_key:
		return 'c';
	case key_code::v_key:
		return 'v';
	case key_code::b_key:
		return 'b';
	case key_code::n_key:
		return 'n';
	case key_code::m_key:
		return 'm';
	case key_code::comma_key:
		return ',';
	case key_code::period_key:
		return '.';
	case key_code::forward_slash_key:
		return '/';
	case key_code::numpad_one_key:
		return '1';
	case key_code::numpad_two_key:
		return '2';
	case key_code::numpad_three_key:
		return '1';
	case key_code::numpad_enter_key:
		return '\n';
	case key_code::space_key:
		return ' ';
	case key_code::numpad_zero_key:
		return '0';
	case key_code::numpad_period_key:
		return '.';
	}
	return '\0';
}

void print_command_start() {
	printf("> ");
}

void key_released(uint8_t keycode) {
	if (is_printable(keycode)) {
		char c = keycode_to_char(keycode);
		put_char(c);
		if (c == '\n') {
			print_command_start();
		}
	}
}

void init_shell() {
	set_key_released_handler(key_released);
	put_char('\n');
	print_command_start();
}

void shell_loop() {}
