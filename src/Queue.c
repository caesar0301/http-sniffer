/*****
** Queue.c
** - implements the methods declared in Queue.h
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

#include <stdlib.h>
#include "queue.h"

void QueueInit(Queue *q)
{
    q->size = 0;
    q->head = q->tail = NULL;
}

int QueueSize(Queue *q)
{
    return q->size;
}

void QueuePush(Queue *q, void *element)
{
    if (!q->head) {
        q->head = (QueueNode*)malloc(sizeof(QueueNode));
        q->head->data = element;
        q->tail = q->head;
    } else {
        q->tail->link = (QueueNode*)malloc(sizeof(QueueNode));
        q->tail = q->tail->link;
        q->tail->data = element;
    }

    q->tail->link = NULL;
    q->size++;
}

void *QueueFront(Queue *q)
{
    return q->size ? q->head->data : NULL;
}

void *QueueTail(Queue *q)
{
    return q->size ? q->tail->data : NULL;
}

void QueuePop(Queue *q, int release)
{
    if (q->size) {
        QueueNode *temp = q->head;
        if (--(q->size)) {
            q->head = q->head->link;
        } else {
            q->head = q->tail = NULL;
        }
        // release memory accordingly
        if (release) {
            free(temp->data);
        }
        free(temp);
    }
}

void QueueClear(Queue *q, int release)
{
    while (q->size) {
        QueueNode *temp = q->head;
        q->head = q->head->link;
        if (release) {
            free(temp->data);
        }
        free(temp);
        q->size--;
    }

    q->head = q->tail = NULL;
}
