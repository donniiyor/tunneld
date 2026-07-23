#include <string.h>

#include "connection_map.h"

void connection_map_init(struct connection_map *map) {
    memset(map, 0, sizeof(*map));
}

struct connection *connection_map_get(struct connection_map *map, uint32_t id) {
    return map->connections[id];
}

void connection_map_set(struct connection_map *map, uint32_t id, struct connection *conn) {
    map->connections[id] = conn;
}

void connection_map_remove(struct connection_map *map, uint32_t id) {
    map->connections[id] = NULL;
}
