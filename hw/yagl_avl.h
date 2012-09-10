/*
 * AVL tree implementation from libavl project.
 */

#ifndef _QEMU_YAGL_AVL_H
#define _QEMU_YAGL_AVL_H

#include "yagl_types.h"

/* Function types. */
typedef int yagl_avl_comparison_func(const void */*avl_a*/, const void */*avl_b*/,
    void */*avl_param*/);
typedef void yagl_avl_item_func(void */*avl_item*/, void */*avl_param*/);
typedef void *yagl_avl_copy_func(void */*avl_item*/, void */*avl_param*/);

/* Memory allocator. */
struct yagl_libavl_allocator
{
    void *(*libavl_malloc)(struct yagl_libavl_allocator */*allocator*/,
                           size_t /*libavl_size*/);
    void (*libavl_free)(struct yagl_libavl_allocator */*allocator*/,
                        void */*libavl_block*/);
};

/* Default memory allocator. */
extern struct yagl_libavl_allocator yagl_avl_allocator_default;
void *yagl_avl_malloc(struct yagl_libavl_allocator *allocator, size_t libavl_size);
void yagl_avl_free(struct yagl_libavl_allocator *allocator, void *libavl_block);

/* Maximum AVL tree height. */
#define YAGL_AVL_MAX_HEIGHT 92

/* Tree data structure. */
struct yagl_avl_table
{
    struct yagl_avl_node *avl_root; /* Tree's root. */
    yagl_avl_comparison_func *avl_compare; /* Comparison function. */
    void *avl_param; /* Extra argument to |avl_compare|. */
    struct yagl_libavl_allocator *avl_alloc; /* Memory allocator. */
    size_t avl_count; /* Number of items in tree. */
    unsigned long avl_generation; /* Generation number. */
};

/* An AVL tree node. */
struct yagl_avl_node
{
    struct yagl_avl_node *avl_link[2]; /* Subtrees. */
    void *avl_data; /* Pointer to data. */
    signed char avl_balance; /* Balance factor. */
};

/* AVL traverser structure. */
struct yagl_avl_traverser
{
    struct yagl_avl_table *avl_table; /* Tree being traversed. */
    struct yagl_avl_node *avl_node; /* Current node in tree. */
    struct yagl_avl_node *avl_stack[YAGL_AVL_MAX_HEIGHT];
    /* All the nodes above |avl_node|. */
    size_t avl_height; /* Number of nodes in |avl_parent|. */
    unsigned long avl_generation; /* Generation number. */
};

/* Table functions. */
struct yagl_avl_table *yagl_avl_create(yagl_avl_comparison_func *compare,
    void *param,
    struct yagl_libavl_allocator *allocator);
struct yagl_avl_table *yagl_avl_copy(const struct yagl_avl_table *org,
    yagl_avl_copy_func *copy,
    yagl_avl_item_func *destroy, struct yagl_libavl_allocator *allocator);
void yagl_avl_destroy(struct yagl_avl_table *tree, yagl_avl_item_func *destroy);
void **yagl_avl_probe(struct yagl_avl_table *tree, void *item);
void *yagl_avl_insert(struct yagl_avl_table *table, void *item);
void *yagl_avl_replace(struct yagl_avl_table *table, void *item);
void *yagl_avl_delete(struct yagl_avl_table *tree, const void *item);
void *yagl_avl_find(const struct yagl_avl_table *tree, const void *item);
void yagl_avl_assert_insert(struct yagl_avl_table *table, void *item);
void *yagl_avl_assert_delete(struct yagl_avl_table *table, void *item);

#define yagl_avl_count(table) ((size_t)(table)->avl_count)

/* Table traverser functions. */
void yagl_avl_t_init(struct yagl_avl_traverser *trav,
                     struct yagl_avl_table *tree);
void *yagl_avl_t_first(struct yagl_avl_traverser *trav,
                       struct yagl_avl_table *tree);
void *yagl_avl_t_last(struct yagl_avl_traverser *trav,
                      struct yagl_avl_table *tree);
void *yagl_avl_t_find(struct yagl_avl_traverser *trav,
                      struct yagl_avl_table *tree,
                      void *item);
void *yagl_avl_t_insert(struct yagl_avl_traverser *trav,
                        struct yagl_avl_table *tree,
                        void *item);
void *yagl_avl_t_copy(struct yagl_avl_traverser *trav,
                      const struct yagl_avl_traverser *src);
void *yagl_avl_t_next(struct yagl_avl_traverser *trav);
void *yagl_avl_t_prev(struct yagl_avl_traverser *trav);
void *yagl_avl_t_cur(struct yagl_avl_traverser *trav);
void *yagl_avl_t_replace(struct yagl_avl_traverser *trav, void *new_item);

#endif
