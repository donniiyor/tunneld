#ifndef PACKET_H
#define PACKET_H

#include "event.h"
#include "protocol.h"

bool handle_packet(event_context_t *ctx, const packet_header_t *header);

#endif
