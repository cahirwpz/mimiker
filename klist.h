#ifndef __KLIST_H__
#define __KLIST_H__

/* First proposal - we distinguish the head of the list.
 * It makes operations like empty possible, but makes inserting slightly more
 * complicated. It makes all operations like INSERT_AFTER dependent on the head
 * structure too. */

// Make a list element structure from a given type.
#define KL_CLIST_ELEMENT(type) 0

// Make a list head type for a given type.
#define KL_CLIST_HEAD(head_name, type) 0 

// Initialize the list with an element passed.
#define KL_CLIST_INIT(head, elem) 0

// Makes a new head.
#define KL_CLIST_UNSHIFT(head, new_head) 0

// Adds an element to the end (esentially, being the predecessor of a head) 
// of a list.
#define KL_CLIST_APPEND(head, elem) 0

// Inserts a new element after the given element.
#define KL_CLIST_INSERT_AFTER(head, elem, new_elem) 0

// As above, but adding a new element before the given element.
#define KL_CLIST_INSERT_BEFORE(head, elem, new_elem) 0

// Extract an element out of the head structure.
// Having HEAD_ELEM and INSERT_BEFORE, APPEND can be omitted for a minimal API.
#define KL_CLIST_HEAD_ELEM(head) 0

// Remove the given element. When it is the head, the successor becomes the new
// head.
#define KL_CLIST_REMOVE(head, elem) 0

// Get the next or previous element of a list.
#define KL_CLIST_NEXT(elem) 0
#define KL_CLIST_PREV(elem) 0

// Check if a given list is empty.
#define KL_CLIST_EMPTY(head) 0

// Checks if a given element is the head. Especially useful if we want to
// finish the iteration.
#define KL_CLIST_IS_HEAD(head, elem) 0

#endif /* __KLIST_H__ */
