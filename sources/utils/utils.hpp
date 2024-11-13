/**
 * \file
 * \brief Contains utility functions declaration.
*/

#pragma once

// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>

// ============================================================================

#define ASSERT(condition, ...)  \
do {                            \
  if (!(condition)) {           \
    printf(__VA_ARGS__);        \
    perror("Error");            \
    exit(1);                    \
  }                             \
} while(0)

// ============================================================================

struct ClientInfo {
  in_addr_t local_ip = 0;
  in_port_t local_port = 0;
  in_addr_t global_ip = 0;
  in_port_t global_port = 0;
  uint8_t id = 0;
};

// ============================================================================

/// Prints ip and port in "ip:port" format.
void PrintSocketAddress(const sockaddr_in& sockaddr);

/// Creates sockaddr_in structure from ip and port. 
sockaddr_in CreateSockaddr(in_addr_t ip, in_port_t port);
