#ifndef CONNECTION_MAP_H
#define CONNECTION_MAP_H

#include "connection.h"

#define MAX_CONNECTIONS 65536

struct connection_map {
    struct connection *connections[MAX_CONNECTIONS];
};

void connection_map_init(struct connection_map *map);

struct connection *connection_map_get(struct connection_map *map, uint32_t id);

void connection_map_set(struct connection_map *map, uint32_t id, struct connection *conn);

void connection_map_remove(struct connection_map *map, uint32_t id);

#endif
