/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * rbtree.c - A clean red-black tree implementation that can handle duplicate
 * nodes.
 */

/* Adapted from the following code written by Todd C. Miller:
 * http://cvsweb.openbsd.org/cgi-bin/cvsweb/src/usr.bin/sudo/Attic/redblack.c
 *
 * Copyright (c) 2004-2005, 2007, 2009 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Adapted from the following code written by Emin Martinian:
 * http://web.mit.edu/~emin/www/source_code/red_black_tree/index.html
 *
 * Copyright (c) 2001 Emin Martinian
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that neither the name of Emin
 * Martinian nor the names of any contributors are be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "includes.h"
#include "rbtree.h"


/* Allocate and initialize empty red-black tree. */
rbtree_t *rbtree_alloc(cmp_t cmp)
{
    rbtree_t *tree;

    if((tree = (rbtree_t *)PyMem_MALLOC(sizeof(rbtree_t))) == NULL)
        goto _err;

    tree->cmp = cmp;

    /* Use sentinel nodes to avoid checking for `NULL' pointers. */
    tree->nil.left = tree->nil.right = tree->nil.parent = &tree->nil;
    tree->nil.color = BLACK;
    tree->nil.data = NULL;

    /* Similarly, the fake root node keeps us from having to worry about
     * splitting the root.
     */
    tree->root.left = tree->root.right = tree->root.parent = &tree->nil;
    tree->root.color = BLACK;
    tree->root.data = NULL;

_err:
    return tree;
}


/* Perform a left rotation starting at node. */
static void rotate_left(rbtree_t *tree, rbnode_t *node)
{
    rbnode_t *child;

    child = node->right;
    node->right = child->left;

    if(child->left != RBTREE_NIL(tree))
        child->left->parent = node;
    child->parent = node->parent;

    if(node == node->parent->left)
        node->parent->left = child;
    else
        node->parent->right = child;

    child->left = node;
    node->parent = child;
}


/* Perform a right rotation starting at node. */
static void rotate_right(rbtree_t *tree, rbnode_t *node)
{
    rbnode_t *child;

    child = node->left;
    node->left = child->right;

    if(child->right != RBTREE_NIL(tree))
        child->right->parent = node;
    child->parent = node->parent;

    if(node == node->parent->left)
        node->parent->left = child;
    else
        node->parent->right = child;

    child->right = node;
    node->parent = child;
}


/* Insert node in red-black tree. Returns the newly inserted node on success or
 * `NULL' on failure.
 */
rbnode_t *rbtree_insert_node(rbtree_t *tree, void *data)
{
    rbnode_t *uncle;
    int res;

    rbnode_t *node = RBTREE_FIRST(tree);
    rbnode_t *parent = RBTREE_ROOT(tree);
    rbnode_t *ret = NULL;


    /* Find correct insertion point just like in normal binary trees. */
    while(node != RBTREE_NIL(tree))
    {
        parent = node;
        res = tree->cmp(data, node->data);
        node = res < 0 ? node->left : node->right;
    }

    /* Allocate a new node, assign it a red color and add two sentinel black
     * nodes as children.
     */
    if((node = (rbnode_t *)PyMem_MALLOC(sizeof(rbnode_t))) == NULL)
        goto _err;

    node->data = data;
    node->left = node->right = RBTREE_NIL(tree);
    node->parent = parent;
    node->color = RED;

    /* First node is on the root's left sub-tree. Otherwise use the comparison
     * function to insert the node at the proper slot.
     */
    if(parent == RBTREE_ROOT(tree) || tree->cmp(data, parent->data) < 0)
        parent->left = node;
    else
        parent->right = node;

    /* Save the newly allocated node pointer that will be returned. */
    ret = node;


    /* If the parent node is black we are all set, if it is red we have the
     * following possible cases to deal with. We iterate through the rest of
     * the tree to make sure none of the required properties is violated.
     *
     *  1) The uncle is red. We repaint both the parent and uncle black and
     *     repaint the grandparent node red.
     *
     *  2) The uncle is black and the new node is the right child of its parent,
     *     and the parent in turn is the left child of its parent. We do a left
     *     rotation to switch the roles of the parent and child, relying on
     *     further iterations to fixup the old parent.
     *
     *  3) The uncle is black and the new node is the left child of its parent,
     *     and the parent in turn is the left child of its parent. We switch the
     *     colors of the parent and grandparent and perform a right rotation
     *     around the grandparent. This makes the former parent the parent of
     *     the new node and the former grandparent.
     *
     * Note that because we use a sentinel for the root node we never need to
     * worry about replacing the root.
     */
    while(node->parent->color == RED)
    {
        if(node->parent == node->parent->parent->left)
        {
            uncle = node->parent->parent->right;
            if(uncle->color == RED)
            {
                node->parent->color = BLACK;
                uncle->color = BLACK;
                node->parent->parent->color = RED;
                node = node->parent->parent;
            }
            else
            {
                if(node == node->parent->right)
                {
                    node = node->parent;
                    rotate_left(tree, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rotate_right(tree, node->parent->parent);
            }
        }
        else
        {
            uncle = node->parent->parent->left;
            if(uncle->color == RED)
            {
                node->parent->color = BLACK;
                uncle->color = BLACK;
                node->parent->parent->color = RED;
                node = node->parent->parent;
            }
            else
            {
                if(node == node->parent->left)
                {
                    node = node->parent;
                    rotate_right(tree, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rotate_left(tree, node->parent->parent);
            }
        }
    }

    /* First node in tree is always black. */
    RBTREE_FIRST(tree)->color = BLACK;

_err:
    return ret;
}


/* Lookup node in red-black tree. Return node on success or `NULL' on failure. */
rbnode_t *rbtree_find_node(rbtree_t *tree, void *data)
{
    rbnode_t *node = RBTREE_FIRST(tree);
    int res;

    while(node != RBTREE_NIL(tree) && (res = tree->cmp(data, node->data)) != 0)
        node = res < 0 ? node->left : node->right;

    if(node == RBTREE_NIL(tree))
        node = NULL;

    return node;
}


/* Repair the tree after a node has been deleted by rotating and repainting
 * colors to restore the 4 properties inherent in red-black trees.
 */
static void rbtree_repair(rbtree_t *tree, rbnode_t *node)
{
    rbnode_t *sibling;

    while(node->color == BLACK && node != RBTREE_FIRST(tree))
    {
        if(node == node->parent->left)
        {
            sibling = node->parent->right;
            if(sibling->color == RED)
            {
                sibling->color = BLACK;
                node->parent->color = RED;
                rotate_left(tree, node->parent);
                sibling = node->parent->right;
            }

            if(sibling->right->color == BLACK && sibling->left->color == BLACK)
            {
                sibling->color = RED;
                node = node->parent;
            }
            else
            {
                if(sibling->right->color == BLACK)
                {
                    sibling->left->color = BLACK;
                    sibling->color = RED;
                    rotate_right(tree, sibling);
                    sibling = node->parent->right;
                }

                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                sibling->right->color = BLACK;
                rotate_left(tree, node->parent);
                node = RBTREE_FIRST(tree);
            }
        }
        else
        {
            sibling = node->parent->left;
            if(sibling->color == RED)
            {
                sibling->color = BLACK;
                node->parent->color = RED;
                rotate_right(tree, node->parent);
                sibling = node->parent->left;
            }

            if(sibling->right->color == BLACK && sibling->left->color == BLACK)
            {
                sibling->color = RED;
                node = node->parent;
            }
            else
            {
                if(sibling->left->color == BLACK)
                {
                    sibling->right->color = BLACK;
                    sibling->color = RED;
                    rotate_left(tree, sibling);
                    sibling = node->parent->left;
                }

                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                sibling->left->color = BLACK;
                rotate_right(tree, node->parent);
                node = RBTREE_FIRST(tree);
            }
        }
    }
    node->color = BLACK;
}


/* Returns the successor of node, or nil if there is none. */
static rbnode_t *rbtree_get_node_successor(rbtree_t *tree, rbnode_t *node)
{
    rbnode_t *succ;

    if((succ = node->right) != RBTREE_NIL(tree))
    {
        while(succ->left != RBTREE_NIL(tree))
            succ = succ->left;
    }
    else
    {
        /* No right child, move up until we find it or hit the root. */
        for(succ = node->parent; node == succ->right; succ = succ->parent)
            node = succ;
        if(succ == RBTREE_ROOT(tree))
            succ = RBTREE_NIL(tree);
    }
    return succ;
}


/* Delete node `z' from the tree and return its data pointer. */
void *rbtree_delete_node(rbtree_t *tree, rbnode_t *z)
{
    rbnode_t *x, *y;
    void *data = z->data;

    if(z->left == RBTREE_NIL(tree) || z->right == RBTREE_NIL(tree))
        y = z;
    else
        y = rbtree_get_node_successor(tree, z);
    x = (y->left == RBTREE_NIL(tree)) ? y->right : y->left;

    if((x->parent = y->parent) == RBTREE_ROOT(tree))
    {
        RBTREE_FIRST(tree) = x;
    }
    else
    {
        if(y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    }

    if(y->color == BLACK)
        rbtree_repair(tree, x);

    if(y != z)
    {
        y->left = z->left;
        y->right = z->right;
        y->parent = z->parent;
        y->color = z->color;
        z->left->parent = z->right->parent = y;
        if(z == z->parent->left)
            z->parent->left = y;
        else
            z->parent->right = y;
    }

    PyMem_FREE(z);
    return data;
}


/* Recursive portion of rbdestroy(). */
static void rbtree_free_node(rbtree_t *tree, rbnode_t *node)
{
    if(node != RBTREE_NIL(tree))
    {
        rbtree_free_node(tree, node->left);
        rbtree_free_node(tree, node->right);
        PyMem_FREE(node);
    }
}


/* Destroy the specified tree, calling the destructor destroy for each node and
 * then freeing the tree itself.
 */
void rbtree_free(rbtree_t *tree)
{
    rbtree_free_node(tree, RBTREE_FIRST(tree));
    PyMem_FREE(tree);
}
