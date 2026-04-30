#ifndef INCLUDE_LIST_CONTAINER_OF_HPP_
#define INCLUDE_LIST_CONTAINER_OF_HPP_

#undef offsetof
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

#define container_of(ptr, type, member)                                        \
	({                                                                         \
		void *member_ptr = (void *)(ptr);                                      \
		((type *)((char *)member_ptr - offsetof(type, member)));               \
	})

#endif // INCLUDE_LIST_CONTAINER_OF_HPP_
