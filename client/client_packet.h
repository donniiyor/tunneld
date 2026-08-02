#ifndef CLIENT_PACKET_H
#define CLIENT_PACKET_H

#include "client_event.h"
#include "protocol.h"

bool handle_packet(event_context_t *ctx, const packet_header_t *header);

#endif
