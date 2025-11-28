#ifndef INCLUDE_SPINLOCK_HPP_
#define INCLUDE_SPINLOCK_HPP_

#include <stdint.h>
class spinlock {
public:
	void lock();
	void unlock();

private:
	// atomic_flag state = ATOMIC_FLAG_INIT;
	volatile int st = false;
};

#endif // INCLUDE_SPINLOCK_HPP_
