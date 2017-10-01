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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef int bool;

typedef struct list_struct_t* list_t;
typedef struct node_struct_t* node_t;
typedef struct list_iter_struct_t* list_iter_t;

typedef bool(*list_compare_t)(const void*, const void*);
typedef void(*list_free_t)(void*);


struct list_struct_t {
	node_t head;
	node_t tail;
	list_compare_t cmp;
	list_free_t lfree;
};

struct node_struct_t {
	node_t next;
	node_t prev;
	void* val;
};

struct list_iter_struct_t {
	node_t node;
};


/*@null@*/ list_t list_init(list_compare_t cmp, list_free_t lree);
void list_exit(list_t inst);
void list_add(list_t inst, void* val);
void list_clear(list_t inst);
/*@null@*/ list_iter_t list_head(list_t l);
int list_is_empty(list_t inst);
size_t list_size(list_t inst);
int list_index_of(list_t inst, void* val);
/*@null@*/ void* list_get(list_t inst, int idx);
bool list_contains(list_t inst, void* val);
/*@null@*/ void* list_remove_by_index(list_t inst, int idx);
/*@null@*/ void* list_remove_by_value(list_t inst, void* val);
/*@null@*/ char* list_to_string(list_t inst);
static void list_add_node(list_t l, node_t n);
/*@null@*/ static node_t list_get_node_by_value(list_t inst, void* val);
/*@null@*/ static node_t list_get_node_by_index(list_t inst, int idx);
/*@null@*/ static void* list_remove_node(list_t inst, node_t n);

/*@null@*/ static node_t node_init(void* val);
/*@null@*/ static void* node_exit(node_t inst);
/*@null@*/ static void* node_get_value(node_t inst);
#if 0
/*@null@*/ static char* node_to_string(node_t inst);
#endif // 0

/*@null@*/ list_iter_t list_iter_init(node_t n);
void list_iter_exit(list_iter_t inst);
bool list_iter_next(list_iter_t inst);
bool list_iter_prev(list_iter_t inst);
/*@null@*/ node_t list_iter_get_node(list_iter_t inst);
/*@null@*/ void* list_iter_get(list_iter_t inst);

////////////////////////////////////////

/*@null@*/ list_t list_init(list_compare_t cmp, list_free_t lfree) {
	list_t inst;
	inst = (list_t) calloc(sizeof(struct list_struct_t), 1);
	if (inst == NULL) {
		printf("Err\n");
	} else {
		//printf("init list@%p\n", inst);
		inst->head = NULL;
		inst->tail = NULL;
		inst->cmp = cmp;
		inst->lfree = lfree;
	}
	return inst;
}

/*@null@*/ static node_t node_init(void* val) {
	node_t inst;
	inst = (node_t) calloc(sizeof(struct node_struct_t), 1);
	if (inst == NULL) {
		printf("Err\n");
	} else {
		//printf("init node@%p\n", inst);
		inst->next = NULL;
		inst->prev = NULL;
		inst->val = val;
	}
	return inst;
}

/*@null@*/ static void* node_get_value(node_t inst) {
	return inst->val;
}

/*@null@*/ static void* node_exit(node_t inst) {
	void* rval = node_get_value(inst);
	//printf("exit node@%p\n", inst);
	free(inst);
	return rval;
}

void list_clear(list_t inst) {
	void* val;
	node_t n = inst->head;
	node_t m;
	while (n != NULL) {
		m = n->next;
		val = node_exit(n);
		//printf("exit val@%p\n", inst->val);
if (inst->lfree != NULL) {
		inst->lfree(val);
}
		n = m;
	}
	inst->head = NULL;
	inst->tail = NULL;
}

void list_exit(list_t inst) {
	list_clear(inst);
	//printf("exit list@%p\n", inst);
	free(inst);
}

static void list_add_node(list_t l, node_t n) {
	if (l->head == NULL) {
		n->next = NULL;
		n->prev = NULL;
		l->head = n;
		l->tail = n;
	} else {
		n->next = NULL;
		n->prev = l->tail;
		l->tail->next = n;
		l->tail = n;
	}
}

/*@null@*/ list_iter_t list_iter_init(node_t n) {
	list_iter_t inst;
	inst = (list_iter_t) calloc(sizeof(struct list_iter_struct_t), 1);
	if (inst == NULL) {
		printf("Err\n");
	} else {
		//printf("init iter@%p\n", inst);
		inst->node = n;
	}
	return inst;
}

void list_iter_exit(list_iter_t inst) {
	//printf("exit iter@%p\n", inst);
	free(inst);
}

/*@null@*/ list_iter_t list_head(list_t l) {
	return list_iter_init(l->head);
}

bool list_iter_next(list_iter_t inst) {
	//bool rval;
	//rval = (inst->node != NULL) && (inst->node->next != NULL);
	if (inst->node != NULL) {
		inst->node = inst->node->next;
	}
	return (inst->node != NULL);
}

bool list_iter_prev(list_iter_t inst) {
	bool rval;
	rval = inst->node->prev != NULL;
	if (rval) {
		inst->node = inst->node->prev;
	}
	return rval;
}

/*@null@*/ node_t list_iter_get_node(list_iter_t inst) {
	return inst->node;
}

/*@null@*/ void* list_iter_get(list_iter_t inst) {
	void* rval = NULL;
	node_t node;

	node = list_iter_get_node(inst);
	if (node != NULL) {
		rval = node_get_value(list_iter_get_node(inst));
	}

	return rval;
}

bool list_iter_eof(list_iter_t inst) {
	return inst->node == NULL;
}

void list_add(list_t inst, void* val) {
	node_t n;
	//printf("Add %p\n", val);
	n = node_init(val);
	list_add_node(inst, n);
}

int list_is_empty(list_t inst) {
	return inst->head == NULL;
}

size_t list_size(list_t inst) {
	node_t n = inst->head;
	int rval = 0;
	while (n) {
		rval++;
		n = n->next;
	}
	return rval;
}

int list_index_of(list_t inst, void* val) {
	node_t n = inst->head;
	int rval = 0;
	while ((n != NULL) && (inst->cmp(node_get_value(n), val))) {
		rval++;
		n = n->next;
	}
	if (n == NULL) {
		rval = -1;
	}
	return rval;
}

/*@null@*/ static node_t list_get_node_by_value(list_t inst, void* val) {
	node_t n = inst->head;
	while ((n != NULL) && (inst->cmp(node_get_value(n), val))) {
		n = n->next;
	}
	return n;
}

/*@null@*/ static node_t list_get_node_by_index(list_t inst, int idx) {
	node_t n = inst->head;
	int ctr = 0;
	while ((n != NULL) && (ctr < idx)) {
		ctr++;
		n = n->next;
	}
	return n;
}

/*@null@*/ void* list_get(list_t inst, int idx) {
	node_t n = list_get_node_by_index(inst, idx);
	return node_get_value(n);
}

bool list_contains(list_t inst, void* val) {
	node_t n = list_get_node_by_value(inst, val);
	return n != NULL;
}

/*@null@*/ static void* list_remove_node(list_t inst, node_t n) {
	if (n->next != NULL) {
		n->next->prev = n->prev;
	} else {
		inst->tail = n->prev;
	}
	if (n->prev != NULL) {
		n->prev->next = n->next;
	} else {
		inst->head = n->next;
	}
	return node_exit(n);
}

/*@null@*/ void* list_remove_by_index(list_t inst, int idx) {
	node_t n = list_get_node_by_index(inst, idx);
	return list_remove_node(inst, n);
}

/*@null@*/ void* list_remove_by_value(list_t inst, void* val) {
	void* rval = NULL;

	node_t n = list_get_node_by_value(inst, val);
	if (n != NULL) {
		rval = list_remove_node(inst, n);
	}

	return rval;
}

/*@null@*/ char* list_to_string(list_t inst) {
	char rval[256];
	snprintf(rval, sizeof(rval), "(list_t) { size=%lu }", list_size(inst));
	return strdup(rval);
}

#if 0
/*@null@*/ static char* node_to_string(node_t inst) {
	char rval[256];
	snprintf(rval, sizeof(rval), "(node_t) { val=%p }", node_get_value(inst));
	return strdup(rval);
}

/////////////////////////////////////////////

static bool string_compare(const void* v1, const void* v2) {
	return strcmp(v1, v2);
}

static void string_free(void* p) {
	//printf("free string@%p\n", p);
	free(p);
}
#endif // 0

