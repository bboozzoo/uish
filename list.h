#ifndef __LIST_H__
#define __LIST_H__ 

struct list_head_s {
    struct list_head_s * next;
    struct list_head_s * prev;
    void * data;
};

#define LIST_INIT(el) struct list_head_s el = {&el, &el, NULL }
#define LIST_INIT_DATA(el, dataa) struct list_head_s el = {&el, &el, dataa}
#define LIST_DATA(el, type) ((type *) el->data)

#define list_for(head, it) for (it = (head)->next; it != (head); it = it->next)

static inline void list_init(struct list_head_s * head, void * data) {
    head->next = head;
    head->prev = head;
    head->data = data;
}

static inline void list_add_tail(struct list_head_s * head, struct list_head_s * el) {
    head->prev->next = el;
    el->prev = head->prev;
    el->next = head;
    head->prev = el;
}

static inline void list_add(struct list_head_s * head, struct list_head_s * el) {
    head->next->prev = el;
    el->next = head->next;
    el->prev = head;
    head->next = el;
}

static inline int list_is_empty(struct list_head_s * s) {
    return (s->next == s);
}

static inline void list_del(struct list_head_s * el) {
    el->prev->next = el->next;
    el->next->prev = el->prev;
}

static inline void list_head_swap(struct list_head_s * head1, struct list_head_s * head2) {
    *head2 = *head1;
    head1->next->prev = head2;
    head1->prev->next = head2;
}

#endif /* __LIST_H__ */
