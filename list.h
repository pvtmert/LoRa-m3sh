

#ifndef _LIST_H_
#define _LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define _LIST_FN_MOD_ static inline

///
/// \struct List
/// \brief
///
/// \var data pointer to data
/// \var size (optional) sizeof data or some identifier (to extract type later)
/// \var next pointer to next List item
///
typedef struct List {
	void *data;
	size_t size;
	struct List *next;
} list_t;


list_t* list_make(void*);
list_t* list_init(void*, size_t);
list_t* list_last(list_t*);
void list_add(list_t*, list_t*);
list_t* list_find(list_t*, list_t*);
list_t* list_del(list_t**);
list_t* list_has(list_t*, void*);
unsigned long list_size(list_t*);
unsigned long list_array(list_t*, void**);
list_t* list_pop(list_t**);
void list_push(list_t**, list_t*);
list_t* list_get(list_t*, unsigned long);
//void list_each(list_t*, void (*)(void*));
void list_each(list_t*, void (*)(void*, size_t));
void list_clean(list_t**, bool);

#endif
