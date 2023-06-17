#include "tools/list.h"

void list_init(list_t *list)
{
    list->first = list->last = (list_node_t *)0;
    list->count = 0;
}

void list_insert_head(list_t *list, list_node_t *node)
{
    node->next = list->first;
    node->previce = (list_node_t *)0;

    if (list_is_empty(list))
    {
        list->last = list->first = node;
    }
    else
    {
        list->first->previce = node;
        list->first = node;
    }

    list->count++;
}

void list_insert_tail(list_t *list, list_node_t *node)
{
    node->previce = list->last;
    node->next = (list_node_t *)0;

    if (list_is_empty(list))
    {
        list->first = list->last = node;
    }
    else
    {
        list->last->next = node;
        list->last = node;
    }

    list->count++;
}

list_node_t *list_delete_head(list_t *list)
{
    if (list_is_empty(list))
        return (list_node_t *)0;

    list_node_t *pnode = list->first;
    list->first = pnode->next;

    if (list->first == (list_node_t *)0)
    {
        list->last = (list_node_t *)0;
    }
    else
    {
        pnode->next->previce = (list_node_t *)0;
    }

    pnode->previce = pnode->next = (list_node_t *)0;
    list->count--;

    return pnode;
}

list_node_t *list_delete(list_t *list, list_node_t *node)
{
    if (node == list->first)
    {
        list->first = node->next;
    }

    if (node == list->last)
    {
        list->last = node->previce;
    }

    if (node->previce)
    {
        node->previce->next = node->next;
    }

    if (node->next)
    {
        node->next->previce = node->previce;
    }

    node->previce = node->next = (list_node_t *)0;
    list->count--;

    return node;
}