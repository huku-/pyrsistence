#ifndef _RBTREE_H_
#define _RBTREE_H_

#include "includes.h"

#define RBTREE_FIRST(t)    ((t)->root.left)
#define RBTREE_ROOT(t)     (&(t)->root)
#define RBTREE_NIL(t)      (&(t)->nil)

#define RED   0
#define BLACK 1


typedef int (*cmp_t)(const void *, const void *);

typedef struct rbnode
{
    struct rbnode *left;
    struct rbnode *right;
    struct rbnode *parent;
    int color;
    void *data;
} rbnode_t;

typedef struct rbtree
{
    cmp_t cmp;
    rbnode_t root;
    rbnode_t nil;
} rbtree_t;


rbtree_t *rbtree_alloc(cmp_t cmp);
rbnode_t *rbtree_insert_node(rbtree_t *, void *);
rbnode_t *rbtree_find_node(rbtree_t *, void *);
void *rbtree_delete_node(rbtree_t *, rbnode_t *);
void rbtree_free(rbtree_t *);

#endif /* _RBTREE_H */

