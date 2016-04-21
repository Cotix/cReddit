#include "reddit.h"
#include "global.h"

/*
typedef struct
{
    int data;
    StackNode *next;
}
StackNode;
*/

EXPORT_SYMBOL StackNode *pushStackNode(StackNode *head, int input)
{
    StackNode *newNode = (StackNode *)(rmalloc(sizeof(StackNode)));
    newNode->data = input;
    newNode->next = head;
    return newNode;
}

EXPORT_SYMBOL void popStackNode(StackNode **head)
{
    if (*head == NULL)
        return;

    StackNode *currentHead = *head;
    *head = (*head)->next;
    free(currentHead);
    return;
}

EXPORT_SYMBOL void freeStackNode(StackNode *head)
{
    if (head == NULL)
        return;
    while (head->next != NULL)
        popStackNode(&head);
    free(head);
}

