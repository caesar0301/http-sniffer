/*****
** Queue.h
** - defines a generic FIFO queue structure
** - maintains a void pointer in each node only
** - does not handle memory allocation for client data
** - supports optional memory deallocation for client data
** Notes
** - this package is provided as is with no warranty.
** - the author is not responsible for any damage caused
**   either directly or indirectly by using this package.
** - anybody is free to do whatever he/she wants with this
**   package as long as this header section is preserved.
** Created on 2004-01-20 by
** - Roger Zhang (rogerz@cs.dal.ca)
** Modifications
** -
** Last compiled under Linux with gcc-3
*/

#ifndef _RZ_C_QUEUE_
#define _RZ_C_QUEUE_

typedef struct _QueueNode {
    void *data;
    struct _QueueNode *link;
} QueueNode;

typedef struct _Queue {
    int size;
    QueueNode *head;
    QueueNode *tail;
} Queue;

/*****
** initialize an empty Queue
** must be called first after a new Queue is declared
*/ void queue_init(Queue *q);

/*****
** push a new element to the end of the Queue
** it's up to the client code to allocate and maintain memory of "element"
*/ void queue_push(Queue *q, void *element);

/*****
** return the first element in the Queue, or NULL when the Queue is empty
*/ void *queue_front(Queue *q);

/*****
** remove the first element (pointer) from the Queue
** set "release" to non-zero if memory deallocation is desired
*/ void queue_pop(Queue *q, int release);

/*****
** remove all elements (pointers) from the Queue
** set "release" to non-zero if memory deallocation is desired
*/ void queue_clear(Queue *q, int release);

/*****
** return current number of elements in the Queue, or 0 when Queue is empty
*/ int queue_size(Queue *q);

#endif /* _RZ_C_QUEUE_ */
