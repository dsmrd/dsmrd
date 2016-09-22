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

#ifndef RBTREE_H
#define RBTREE_H

typedef int (*rbtree_less_then_t)(void* a, void* b);
typedef int (*rbtree_equals_t)(void* a, void* b);
typedef void (*rbtree_free_key_t)(void* a);
typedef void (*rbtree_free_value_t)(void* a);

typedef struct rbnode_struct_t* rbnode_t;
typedef struct rbtree_struct_t* rbtree_t;

rbtree_t rbtree_init(rbtree_less_then_t less_then, rbtree_equals_t equals,
		rbtree_free_key_t free_key, rbtree_free_value_t free_value);
void rbtree_exit(rbtree_t inst);
void* rbtree_put(rbtree_t t, void* key, void* value);
void* rbtree_get(rbtree_t inst, void* key);
void* rbtree_delete(rbtree_t inst, void* key);
void rbtree_foreach(rbtree_t inst, void (*callback)(void*, void*));

#endif // RBTREE_H

