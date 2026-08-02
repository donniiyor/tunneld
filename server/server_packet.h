#ifndef SERVER_PACKET_H
#define SERVER_PACKET_H

#include "protocol.h"
#include "server_event.h"

bool handle_packet(event_context_t *ctx, const packet_header_t *header);

#endif
