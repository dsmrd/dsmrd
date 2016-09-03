
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "logging.h"
#include "list.h"
#include "dispatch.h"

typedef struct ava_struct_t* avahi_t;

avahi_t avahi_init(char* name);
void avahi_open(avahi_t inst, dispatch_t dis);
void ava_close(avahi_t inst);

