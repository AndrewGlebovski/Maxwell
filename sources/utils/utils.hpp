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
    perror("Error");            \
    printf(__VA_ARGS__);        \
    exit(1);                    \
  }                             \
} while(0)

// ============================================================================

/// Prints ip and port in "ip:port" format.
void PrintSocketAddress(const sockaddr_in& sockaddr);

/// Creates sockaddr_in structure from ip and port. 
sockaddr_in CreateSockaddr(in_addr_t ip, in_port_t port);
