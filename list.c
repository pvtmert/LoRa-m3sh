
#include "list.h"

///
/// \file list.h
///
/// list functions
/// for generic linked list implementation
///


///
/// \brief creates new list item with given data
///
/// \param data pointer-to data to put in object
///
/// \return reference to created list
///
list_t* list_make(void *data) {
	list_t *list = (list_t*) malloc(sizeof(list_t));
	list->data = data;
	list->next = NULL;
	return list;
}

///
/// \brief initializes list with size
///
/// \param data pointer-to data to put in object
/// \param size the size of object or arbitary identifier to get object type
///
/// \return reference to created list
///
list_t* list_init(void *data, size_t size) {
	list_t *list = list_make(data);
	list->size = size;
	return list;
}

///
/// \brief gets last item of the list
///
/// \param list pointer-to list object
///
/// \return reference to last item
///
list_t* list_last(list_t *list) {
	while(list && list->next) {
		list = list->next;
		continue;
	}
	return list;
}

///
/// \brief adds item to the list, adds to next of current.
/// current next becomes last of child.
/// which means a -> b -> c and 1 -> 2 -> 3 then adding to 'b'
/// yields to a -> b -> 1 -> 2 -> 3 -> c
///
/// \param root pointer to list object
/// \param child pointer to child to be added
///
void list_add(list_t *root, list_t *child) {
	if(!root) return;
	list_last(child)->next = root->next;
	root->next = child;
	return;
}

///
/// \brief finds given node in given list
/// returns node before it to actively change next
///
/// \param list pointer to list
/// \param node reference to node (to be find)
///
/// \return reference to item
/// \note return value might be null if item not found
/// and can be same as given list if its root node.
///
list_t* list_find(list_t *list, list_t *node) {
	while(list && list != node && list->next != node) {
		list = list->next;
		continue;
	}
	return list;
}

///
/// \brief deletes node from the list
/// requires reference to node-ptr
/// so from a -> b -> c to remove 'b'
/// do del(&a->next) which returns to pointer to b
///
/// \param pointer to node-ptr
///
/// \return deleted list item
/// \note returned item's next still points to node appended
///
list_t* list_del(list_t **node) {
	if(!node || !*node) {
		return NULL;
	}
	list_t *temp = *node;
	*node = temp->next;
	return temp;
}

///
/// \brief gets list item if list contains specified data reference.
///
/// \param list reference to list
/// \param data reference to data
///
/// \return reference to list item containing data or null if not found
///
list_t* list_has(list_t *list, void *data) {
	while(list && list->data != data) {
		list = list->next;
		continue;
	}
	return list;
}

///
/// \brief gets size/length of the list from current point
///
/// \param list reference to list
///
/// \return unsigned long type of list length
///
unsigned long list_size(list_t *list) {
	unsigned long size = 0;
	while(list != NULL) {
		list = list->next;
		continue;
	}
	return size;
}

///
/// \brief converts list to array
///
/// \param list reference to list
/// \param target reference to object-ptr
///
/// \return size of the list that has been converted
/// \note target will be overwritten and reference will be lost
/// general usage is array(&my_array) where my_array is void*
///
unsigned long list_array(list_t *list, void **target) {
	unsigned long length = list_size(list);
	*target = malloc(length * sizeof(void*));
	for(int i=0; i<length && list != NULL; i++) {
		target[i] = list->data;
		list = list->next;
		continue;
	}
	return length;
}

///
/// \brief removes top/first item in the list
///
/// \param list reference to be removed
///
/// \return reference to removed item
///
list_t* list_pop(list_t **list) {
	if(!list) {
		return NULL;
	}
	list_t *temp = *list;
	*list = temp->next;
	return temp;
}

///
/// \brief inserts given item to top of list
///
/// \param list reference to list-pointer
/// \param node reference to node to be inserted
///
/// \note list parameter will be overwritten general usage
/// follows as push(&list) where list is list_t*
///
void list_push(list_t **list, list_t *node) {
	if(!list) {
		return;
	}
	if(!*list) {
		*list = node;
		return;
	}
	list_last(node)->next = *list;
	*list = node;
	return;
}

///
/// \brief gets Nth item from list
///
/// \param list reference to list
/// \param n index to be returned
///
/// \return reference to list item or null if n is less than list size
///
list_t* list_get(list_t *list, unsigned long n) {
	for(int i=0; i<n && list != NULL; i++) {
		list = list->next;
		continue;
	}
	return list;
}

///
/// \brief executes callback function for every item of the list
///
/// \param list reference to list
/// \param function reference to function
///
/// \note function requires two arguments
/// - void* actual reference to data
/// - size_t sizeof or identifier that is given to this node
///
void list_each(list_t *list, void (*function)(void*, size_t)) {
	while(list) {
		if(function && list->data) {
			(*function)(list->data, list->size);
		}
		list = list->next;
		continue;
	}
	return;
}

///
/// \brief cleans up the list
///
/// \param list reference to list-pointer
/// \param full perform full cleanup (free data too)
///
/// \note list parameter will be overwritten and cleaned
/// general usage follows as clean(&list) where list is list_t*
///
void list_clean(list_t **list, bool full) {
	if(!list || !*list) {
		return;
	}
	if( (*list)->next ) {
		list_clean( &(*list)->next, full );
	}
	if(full) {
		free((*list)->data);
	}
	free(*list);
	*list = NULL;
	return;
}
