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

#ifndef LIST_H
#define LIST_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct list_struct_t* list_t;
typedef struct node_struct_t* node_t;
typedef struct list_iter_struct_t* list_iter_t;

typedef int(*list_compare_t)(const void*, const void*);
typedef void(*list_free_t)(void*);


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

/*@null@*/ list_iter_t list_iter_init(node_t n);
void list_iter_exit(list_iter_t inst);
bool list_iter_next(list_iter_t inst);
bool list_iter_prev(list_iter_t inst);
/*@null@*/ node_t list_iter_get_node(list_iter_t inst);
/*@null@*/ void* list_iter_get(list_iter_t inst);
int list_iter_eof(list_iter_t inst);

#endif // LIST_H

