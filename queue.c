#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *q = malloc(sizeof(struct list_head));
    if (!q)
        return NULL;

    INIT_LIST_HEAD(q);
    return q;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;

    element_t *cur, *next;
    list_for_each_entry_safe (cur, next, l, list)
        q_release_element(cur);

    free(l);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *new = malloc(sizeof(element_t));
    if (new) {
        new->value = strdup(s);
        if (!new->value) {
            free(new);
            return false;
        }
        list_add(&new->list, head);
        return true;
    }

    return false;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *new = malloc(sizeof(element_t));
    if (new) {
        new->value = strdup(s);
        if (!new->value) {
            free(new);
            return false;
        }
        list_add_tail(&new->list, head);
        return true;
    }

    return false;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *removed = list_first_entry(head, element_t, list);
    list_del(head->next);
    if (sp) {
        strncpy(sp, removed->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    return removed;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *removed = list_last_entry(head, element_t, list);
    list_del(head->prev);
    if (sp) {
        strncpy(sp, removed->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    return removed;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int size = 0;
    struct list_head *node;
    list_for_each (node, head)
        size++;

    return size;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;

    struct list_head *slow = head->next;
    for (struct list_head *fast = slow->next;
         fast != head && fast->next != head;
         slow = slow->next, fast = fast->next->next) {
    }

    list_del(slow);
    q_release_element(list_entry(slow, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head || list_empty(head))
        return false;

    element_t *cur, *next;
    bool del_tail = false;
    list_for_each_entry_safe (cur, next, head, list) {
        if (&next->list != head && strcmp(cur->value, next->value) == 0) {
            list_del(&cur->list);
            q_release_element(cur);
            del_tail = true;
        } else if (del_tail) {
            list_del(&cur->list);
            q_release_element(cur);
            del_tail = false;
        }
    }

    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head)
        return;

    for (struct list_head *node = head->next;
         node != head && node->next != head; node = node->next) {
        struct list_head *next = node->next->next;
        node->next->prev = node->prev;
        node->next->next = node;
        node->prev->next = node->next;
        node->prev = node->next;
        node->next = next;
        next->prev = node;
        // list_del(node);
        // list_add(node, node->next);
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    struct list_head *prev = head, *cur, *next;
    list_for_each_safe (cur, next, head) {
        cur->next = prev;
        cur->prev = next;
        prev = cur;
    }

    head->next = prev;
    head->prev = next;
}


static struct list_head *q_merge(struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL;
    struct list_head **cur = &head;
    while (a && b) {
        char *a_val = list_entry(a, element_t, list)->value;
        char *b_val = list_entry(b, element_t, list)->value;
        if (strcmp(a_val, b_val) <= 0) {
            *cur = a;
            a = a->next;
            cur = &(*cur)->next;
        } else {
            *cur = b;
            b = b->next;
            cur = &(*cur)->next;
        }
    }

    *cur = (struct list_head *) ((uintptr_t) a | (uintptr_t) b);
    // *cur = a ? a : b;
    return head;
}


static void q_merge_final(struct list_head *head, struct list_head *list)
{
    head->next = list;
    struct list_head *cur, *next;
    for (cur = head, next = cur->next; next; cur = next, next = next->next)
        next->prev = cur;

    cur->next = head;
    head->prev = cur;
}

/* Sort elements of queue in ascending order */
void q_sort(struct list_head *head)
{
    if (!head || list_empty(head) || head->prev == head->next)
        return;

    head->prev->next = NULL;
    struct list_head *list = head->next;
    struct list_head *pending = NULL;
    size_t count = 0;
    do {
        struct list_head **tail = &pending;
        size_t bits = count;
        for (; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;

        if (bits > 0) {
            struct list_head *a = *tail;
            struct list_head *b = a->prev;
            a = q_merge(b, a);
            a->prev = b->prev;
            *tail = a;
        }

        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);

    while (pending) {
        struct list_head *next = pending->prev;
        list = q_merge(pending, list);
        pending = next;
    }

    q_merge_final(head, list);
}
