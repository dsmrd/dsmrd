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


typedef int (*rbnode_less_then)(void* a, void* b);
typedef int (*rbnode_equals)(void* a, void* b);
typedef void (*rbnode_free_key)(void* a);
typedef void (*rbnode_free_value)(void* a);

typedef struct rbnode_struct_t* rbnode_t;
typedef struct rbtree_struct_t* rbtree_t;

// Red-Black tree description
typedef enum { BLACK, RED } rbnode_color_t;

struct rbnode_struct_t {
	rbnode_t left;          // left child
	rbnode_t right;         // right child
	rbnode_t parent;        // parent
	rbnode_color_t color;   // node color (BLACK, RED)
	void* key;                 // key stored in node
	void* value;                 // value stored in node
};

#define NIL &sentinel           // all leafs are sentinels
struct rbnode_struct_t sentinel = { NIL, NIL, 0, BLACK, 0};

struct rbtree_struct_t {
	rbnode_t root;               // root of Red-Black tree
	rbnode_less_then less_than;
	rbnode_equals equals;
	rbnode_free_key free_key;
	rbnode_free_value free_value;
};


static void rbnode_rotate_left(rbnode_t x, rbtree_t t) {

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

static void rbnode_rotate_right(rbnode_t x, rbtree_t t) {

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

static void rbnode_insert_fixup(rbnode_t x, rbtree_t t) {

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
					rbnode_rotate_left(x, t);
				}

				// recolor and rotate
				x->parent->color = BLACK;
				x->parent->parent->color = RED;
				rbnode_rotate_right(x->parent->parent, t);
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
					rbnode_rotate_right(x, t);
				}
				x->parent->color = BLACK;
				x->parent->parent->color = RED;
				rbnode_rotate_left(x->parent->parent, t);
			}
		}
	}
	t->root->color = BLACK;
}

rbnode_t rbnode_insert(void* key, void* value, rbtree_t t) {
	rbnode_t current, parent, x;

	//
	// allocate node for key and insert in tree
	//

	// find where node belongs
	current = t->root;
	parent = 0;
	while (current != NIL) {
		if (t->equals(key, current->key)) {
			return (current);
		}
		parent = current;
		current = t->less_than(key, current->key) ?
			current->left : current->right;
	}

	// setup new node
	if ((x = malloc (sizeof(*x))) == 0) {
		printf ("insufficient memory (rbnode_insert)\n");
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
		if(t->less_than(key, parent->key)) {
			parent->left = x;
		} else {
			parent->right = x;
		}
	} else {
		t->root = x;
	}

	rbnode_insert_fixup(x, t);
	return(x);
}

static void rbnode_delete_fixup(rbnode_t x, rbtree_t t) {

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
				rbnode_rotate_left(x->parent, t);
				w = x->parent->right;
			}
			if (w->left->color == BLACK && w->right->color == BLACK) {
				w->color = RED;
				x = x->parent;
			} else {
				if (w->right->color == BLACK) {
					w->left->color = BLACK;
					w->color = RED;
					rbnode_rotate_right(w, t);
					w = x->parent->right;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				rbnode_rotate_left(x->parent, t);
				x = t->root;
			}
		} else {
			rbnode_t w = x->parent->left;
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				rbnode_rotate_right(x->parent, t);
				w = x->parent->left;
			}
			if (w->right->color == BLACK && w->left->color == BLACK) {
				w->color = RED;
				x = x->parent;
			} else {
				if (w->left->color == BLACK) {
					w->right->color = BLACK;
					w->color = RED;
					rbnode_rotate_left(w, t);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				rbnode_rotate_right(x->parent, t);
				x = t->root;
			}
		}
	}
	x->color = BLACK;
}

void rbnode_delete(rbnode_t z, rbtree_t t) {
	rbnode_t x, y;

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
		t->free_value(z->value);
		z->key = y->key;
		z->value = y->value;
	} else {
		t->free_key(y->key);
		t->free_value(y->value);
	}

	if (y->color == BLACK) {
		rbnode_delete_fixup(x, t);
	}

	free (y);
}

rbnode_t rbnode_find(void* key, rbtree_t t) {

	//
	// find node containing key
	//

	rbnode_t current = t->root;
	while(current != NIL) {
		if(t->equals(key, current->key)) {
			return (current);
		} else {
			current = t->less_than(key, current->key) ?
				current->left : current->right;
		}
	}

	return(0);
}

int compLT(void* a, void* b) {
	return strcmp(a, b) < 0;
}

int compEQ(void* a, void* b) {
	return strcmp(a, b) == 0;
}

void FK(void* k) {
	free(k);
}

void FV(void* k) {
	//free(k);
}

void main(int argc, char **argv) {
	int a, maxnum, ct;
	rbnode_t t;
	struct rbtree_struct_t r;

	r.root = NIL;
	r.less_than = compLT;
	r.equals = compEQ;
	r.free_key = FK;
	r.free_value = FV;

	// command-line:
	//
	//   rbt maxnum
	//
	//   rbt 2000
	//       process 2000 records
	//
	//

	struct {
		char* key;
		int inserted;
	} s[] = {
		{ "A", 0 },
		{ "B", 0 },
		{ "C", 0 },
		{ "D", 0 },
		{ "E", 0 },
		{ "F", 0 },
		{ "G", 0 },
		{ "H", 0 },
		{ "I", 0 },
		{ "J", 0 },
	};

	//if (argc > 0) {
		//maxnum = atoi(argv[1]);
	//} else {
		maxnum = 500;
	//}

	srand(time(NULL));

	for (ct = maxnum; ct; ct--) {
		a = rand() % 9 + 1;
		if ((t = rbnode_find(s[a].key, &r)) != NULL) {
			printf("Deleting %s value %s\n", (char*)t->key, (char*)t->value);
			rbnode_delete(t, &r);
		} else {
			t = rbnode_insert(strdup(s[a].key), s[a].key, &r);
			printf("Inserting %s value %s\n", (char*)t->key, (char*)t->value);
		}
	}

	for (a = 0; a < 9; a++) {
		if ((t = rbnode_find(s[a].key, &r)) != NULL) {
			printf("Deleting %s value %s\n", (char*)t->key, (char*)t->value);
			rbnode_delete(t, &r);
		}
	}
}

