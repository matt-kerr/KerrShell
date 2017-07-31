// Matthew Kerr
// April 21, 2014

#ifndef linkedList_h
#define linkedList_h
#define MAX_HISTORY_LENGTH 200

typedef struct node
{
    char * data;
    int length;
    struct node * next;
    struct node * prev;
}Node;

void createList(Node ** head);
char * findHistory(Node ** head, int index);
void addLast(Node ** head, char * data_to_add);
void printList(Node ** head);
void clearList(Node ** head);

#endif
