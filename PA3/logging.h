#ifndef _LOGGING_H_

#define _LOGGING_H_

#include "router.h"

void write_routing_table(FILE* out, router_t *current_state);

void print_entry( FILE* out, router_t *router, routing_entry_t* entry );

#endif
