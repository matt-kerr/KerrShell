// Matthew Kerr
// April 21, 2014

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedList.h"

void createList(Node ** head)
{
    Node * dummy_head = (Node *)calloc(1, sizeof(Node));
    dummy_head->next = NULL;
    dummy_head->prev = NULL;
    dummy_head->data = NULL;
    dummy_head->length = 0;
    *head = dummy_head;
}

char * findHistory(Node ** head, int index)
{
    if (*head == NULL)
    {
        return NULL;
    }
    // indexes should start at 1
    if (index < 1)
    {
        return NULL;
    }
    // index too big
    if ((*head)->length < index)
    {
        return NULL;
    }

    int i = 1;
    Node * curr = (Node *) (*head)->next;

    while (i < index)
    {
        curr = curr->next;
        i++;
    }
    return curr->data;
}

void addLast(Node ** head, char * data_to_add)
{
    // make dummy head
    // length for dummy head is the length of the list
    if (*head == NULL)
    {
        createList(head);
    }

    Node * newNode = (Node *)malloc(sizeof(Node));
    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->length = strlen(data_to_add);
    newNode->data = (char *)calloc((strlen(data_to_add) + 1), sizeof(char));
    strcpy(newNode->data, data_to_add);

    if ((*head)->next == NULL)
    {
        // list empty
        (*head)->next = newNode;
    }
    else
    {
        // add to end

        // first check if something needs to be removed
        if (MAX_HISTORY_LENGTH == (*head)->length)
        {
            // remove oldest entry
            Node * temp = (*head)->next;
            (*head)->next = (*head)->next->next;
            free(temp->data);
            free(temp);
            temp = NULL;
            (*head)->length--;
        }

        Node * curr = *head;
        while (curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = newNode;
        newNode->prev = curr;
        curr = newNode;
    }
    // increase list size by 1
    (*head)->length = (*head)->length + 1;
    return;
}

void printList(Node ** head)
{
    if ((*head)->next == NULL)
    {
        return;
    }

    int i = 1;
    Node * curr = (Node *) (*head)->next;
    while (curr != NULL)
    {
        printf(" %d %s\n", i, curr->data);
        curr = curr->next;
        i++;
    }
    return;
}

void clearList(Node ** head)
{
    if (*head == NULL)
    {
        return;
    }

    Node * curr = (Node *) (*head)->next;
    (*head)->next = NULL;
    (*head)->length = 0;
    Node * next = NULL;

    // remove everything except head
    while (curr != NULL)
    {
        free(curr->data);
        if (curr->next != NULL)
        {
            next = curr->next;
        }
        else
        {
            free(curr);
            break;
        }
        free(curr);
        curr = next;
    }

    // remove head
    free(*head);
    *head = NULL;
    return;
}


