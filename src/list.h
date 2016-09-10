#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef int bool;

typedef struct list_struct_t* list_t;
typedef struct node_struct_t* node_t;
typedef struct iter_struct_t* iter_t;

typedef bool(*list_compare_t)(const void*, const void*);
typedef void(*list_free_t)(void*);


/*@null@*/ list_t list_init(list_compare_t cmp, list_free_t lree);
void list_exit(list_t inst);
void list_add(list_t inst, void* val);
void list_clear(list_t inst);
/*@null@*/ iter_t list_head(list_t l);
int list_is_empty(list_t inst);
size_t list_size(list_t inst);
int list_index_of(list_t inst, void* val);
/*@null@*/ void* list_get(list_t inst, int idx);
bool list_contains(list_t inst, void* val);
/*@null@*/ void* list_remove_by_index(list_t inst, int idx);
/*@null@*/ void* list_remove_by_value(list_t inst, void* val);
/*@null@*/ char* list_to_string(list_t inst);

/*@null@*/ iter_t iter_init(node_t n);
void iter_exit(iter_t inst);
bool iter_next(iter_t inst);
bool iter_prev(iter_t inst);
/*@null@*/ node_t iter_get_node(iter_t inst);
/*@null@*/ void* iter_get(iter_t inst);
