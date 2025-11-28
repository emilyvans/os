#include "spinlock.hpp"
#include "console.hpp"

void fence() {
	asm("SFENCE");
}

int locked = true;

void spinlock::lock() {
	__sync_synchronize();
	while (__atomic_compare_exchange_n(&this->st, &locked, true, 0, 3, 3)) {
	}
}

void spinlock::unlock() {
	__sync_synchronize();
	st = false;
}
