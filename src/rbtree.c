/*
 * DSMRd - Dutch Smart Meter Requirements Daemon
 * Copyright (C)2016  M.J. de Wit
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * Heavily modified source code based on following implementation:
 * <https://www.cs.auckland.ac.nz/~jmor159/PLDS210/niemann/s_rbt.txt>
 * No original copyrights found.
 */

/* red-black tree */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "rbtree.h"


typedef int (*rbtree_less_then_t)(void* a, void* b);
typedef int (*rbtree_equals_t)(void* a, void* b);
typedef void (*rbtree_free_key_t)(void* a);
typedef void (*rbtree_free_value_t)(void* a);

typedef struct rbnode_struct_t* rbnode_t;
typedef struct rbtree_struct_t* rbtree_t;

// Red-Black tree description
typedef enum { BLACK, RED } rbnode_color_t;

struct rbnode_struct_t {
	rbnode_t left;          // left child
	rbnode_t right;         // right child
	rbnode_t parent;        // parent
	rbnode_color_t color;   // node color (BLACK, RED)
	void* key;              // key stored in node
	void* value;            // value stored in node
};

#define NIL &sentinel           // all leafs are sentinels
struct rbnode_struct_t sentinel = { NIL, NIL, NULL, BLACK, NULL, NULL};

struct rbtree_struct_t {
	rbnode_t root;               // root of Red-Black tree
	rbtree_less_then_t less_then;
	rbtree_equals_t equals;
	rbtree_free_key_t free_key;
	rbtree_free_value_t free_value;
};


static void _rbtree_rotate_left(rbtree_t t, rbnode_t x) {

	//
	// rotate node x to left
	//

	rbnode_t y = x->right;

	// establish x->right link
	x->right = y->left;
	if (y->left != NIL) {
		y->left->parent = x;
	}

	// establish y->parent link
	if (y != NIL) y->parent = x->parent;
	if (x->parent) {
		if (x == x->parent->left) {
			x->parent->left = y;
		} else {
			x->parent->right = y;
		}
	} else {
		t->root = y;
	}

	// link x and y
	y->left = x;
	if (x != NIL) {
		x->parent = y;
	}
}

static void _rbtree_rotate_right(rbtree_t t, rbnode_t x) {

	//
	// rotate node x to right
	//

	rbnode_t y = x->left;

	// establish x->left link
	x->left = y->right;
	if (y->right != NIL) {
		y->right->parent = x;
	}

	// establish y->parent link
	if (y != NIL) y->parent = x->parent;
	if (x->parent) {
		if (x == x->parent->right) {
			x->parent->right = y;
		} else {
			x->parent->left = y;
		}
	} else {
		t->root = y;
	}

	// link x and y
	y->right = x;
	if (x != NIL) {
		x->parent = y;
	}
}

static void _rbtree_put_fixup(rbtree_t t, rbnode_t x) {

	//
	// maintain Red-Black tree balance
	// after inserting node x
	//

	// check Red-Black properties
	while (x != t->root && x->parent->color == RED) {
		// we have a violation
		if (x->parent == x->parent->parent->left) {
			rbnode_t y = x->parent->parent->right;
			if (y->color == RED) {

				// uncle is RED
				x->parent->color = BLACK;
				y->color = BLACK;
				x->parent->parent->color = RED;
				x = x->parent->parent;
			} else {

				// uncle is BLACK
				if (x == x->parent->right) {
					// make x a left child
					x = x->parent;
					_rbtree_rotate_left(t, x);
				}

				// recolor and rotate
				x->parent->color = BLACK;
				x->parent->parent->color = RED;
				_rbtree_rotate_right(t, x->parent->parent);
			}
		} else {

			// mirror image of above code
			rbnode_t y = x->parent->parent->left;
			if (y->color == RED) {

				// uncle is RED
				x->parent->color = BLACK;
				y->color = BLACK;
				x->parent->parent->color = RED;
				x = x->parent->parent;
			} else {

				// uncle is BLACK
				if (x == x->parent->left) {
					x = x->parent;
					_rbtree_rotate_right(t, x);
				}
				x->parent->color = BLACK;
				x->parent->parent->color = RED;
				_rbtree_rotate_left(t, x->parent->parent);
			}
		}
	}
	t->root->color = BLACK;
}

void* rbtree_put(rbtree_t t, void* key, void* value) {
	rbnode_t current, parent, x;
	void* rval = 0;

	//
	// allocate node for key and insert in tree
	//

	// find where node belongs
	current = t->root;
	parent = 0;
	while (current != NIL) {
		if (t->equals(key, current->key)) {
			rval = current->value;
			current->value = value;
			return rval;
		}
		parent = current;
		current = t->less_then(key, current->key) ?
			current->left : current->right;
	}

	// setup new node
	if ((x = malloc (sizeof(*x))) == 0) {
		printf ("insufficient memory (rbtree_put)\n");
		exit(1);
	}
	x->key = key;
	x->value = value;
	x->parent = parent;
	x->left = NIL;
	x->right = NIL;
	x->color = RED;

	// insert node in tree
	if(parent) {
		if(t->less_then(key, parent->key)) {
			parent->left = x;
		} else {
			parent->right = x;
		}
	} else {
		t->root = x;
	}

	_rbtree_put_fixup(t, x);

	return NULL;
}

static void _rbtree_delete_node_fixup(rbtree_t t, rbnode_t x) {

	//
	// maintain Red-Black tree balance
	// after deleting node x
	//

	while (x != t->root && x->color == BLACK) {
		if (x == x->parent->left) {
			rbnode_t w = x->parent->right;
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				_rbtree_rotate_left(t, x->parent);
				w = x->parent->right;
			}
			if (w->left->color == BLACK && w->right->color == BLACK) {
				w->color = RED;
				x = x->parent;
			} else {
				if (w->right->color == BLACK) {
					w->left->color = BLACK;
					w->color = RED;
					_rbtree_rotate_right(t, w);
					w = x->parent->right;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				_rbtree_rotate_left(t, x->parent);
				x = t->root;
			}
		} else {
			rbnode_t w = x->parent->left;
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				_rbtree_rotate_right(t, x->parent);
				w = x->parent->left;
			}
			if (w->right->color == BLACK && w->left->color == BLACK) {
				w->color = RED;
				x = x->parent;
			} else {
				if (w->left->color == BLACK) {
					w->right->color = BLACK;
					w->color = RED;
					_rbtree_rotate_left(t, w);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				_rbtree_rotate_right(t, x->parent);
				x = t->root;
			}
		}
	}
	x->color = BLACK;
}

static void* _rbtree_delete_node(rbtree_t t, rbnode_t z) {
	rbnode_t x, y;
	void* rval;

	//
	// delete node z from tree
	//

	if ((!z) || (z == NIL)) {
		return;
	}

	if ((z->left == NIL) || (z->right == NIL)) {
		// y has a NIL node as a child
		y = z;
	} else {
		// find tree successor with a NIL node as a child
		y = z->right;
		while (y->left != NIL) {
			y = y->left;
		}
	}

	// x is y's only child
	if (y->left != NIL) {
		x = y->left;
	} else {
		x = y->right;
	}

	// remove y from the parent chain
	x->parent = y->parent;
	if (y->parent) {
		if (y == y->parent->left) {
			y->parent->left = x;
		} else {
			y->parent->right = x;
		}
	} else {
		t->root = x;
	}

	if (y != z) {
		t->free_key(z->key);
		rval = z->value;
		z->key = y->key;
		z->value = y->value;
	} else {
		t->free_key(y->key);
		rval = y->value;
	}

	if (y->color == BLACK) {
		_rbtree_delete_node_fixup(t, x);
	}

	free (y);

	return rval;
}

static rbnode_t _rbtree_find_node(void* key, rbtree_t t) {

	//
	// find node containing key
	//

	rbnode_t current = t->root;
	while(current != NIL) {
		if(t->equals(key, current->key)) {
			return current;
		} else {
			current = t->less_then(key, current->key) ?
				current->left : current->right;
		}
	}

	return NULL;
}

void* rbtree_get(rbtree_t inst, void* key) {
	void* rval = NULL;
	rbnode_t node;

	node = _rbtree_find_node(key, inst);
	if (node != NULL) {
		rval = node->value;
	}

	return rval;
}

void* rbtree_delete(rbtree_t inst, void* key) {
	void* rval = NULL;
	rbnode_t node;

	node = _rbtree_find_node(key, inst);
	if (node != NULL) {
		rval = _rbtree_delete_node(inst, node);
	}

	return rval;
}

rbtree_t rbtree_init(rbtree_less_then_t less_then, rbtree_equals_t equals, rbtree_free_key_t free_key, rbtree_free_value_t free_value) {
	rbtree_t inst;

	inst = (rbtree_t) malloc(sizeof(struct rbtree_struct_t));
	inst->root = NIL;
	inst->less_then = less_then;
	inst->equals = equals;
	inst->free_key = free_key;
	inst->free_value = free_value;

	return inst;
}

void rbtree_exit(rbtree_t inst) {
	free(inst);
}

