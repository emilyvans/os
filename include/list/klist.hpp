#ifndef INCLUDE_LIST_KLIST_HPP_
#define INCLUDE_LIST_KLIST_HPP_

typedef struct KListHead {
	struct KListHead *next, *prev;
} KListHead;

#define KLIST_DEFINE(name) KListHead name = {&name, &name};

#define KLIST_FOREACH(list_head)                                               \
	for (KListHead *head = (list_head)->next; head != (list_head);             \
	     head = head->next)

// void klist_add_tail(klist_head *list_head, klist_head *list_tail);

// void klist_init(klist_head *list);

inline void klist_add_tail(KListHead *list_head, KListHead *list_tail) {
	list_tail->prev = list_head->prev;
	list_tail->next = list_head;
	list_head->prev->next = list_tail;
	list_head->prev = list_tail;
}

inline void klist_init(KListHead *list) {
	list->next = list;
	list->prev = list;
}
#endif // INCLUDE_LIST_KLIST_HPP_
