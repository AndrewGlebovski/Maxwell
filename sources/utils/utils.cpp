/**
 * \file
 * \brief Contains utility functions implementation.
*/

// ============================================================================

#include <arpa/inet.h>      // IP converting functions.

#include <utils/utils.hpp>

// ============================================================================

void PrintSocketAddress(const sockaddr_in& sockaddr) {
  char str[INET_ADDRSTRLEN] = "";
  inet_ntop(AF_INET, &sockaddr.sin_addr, str, INET_ADDRSTRLEN);

  in_port_t port = ntohs(sockaddr.sin_port);

  printf("%s:%hu", str, port);
}

sockaddr_in CreateSockaddr(in_addr_t ip, in_port_t port) {
  sockaddr_in addr = {};

  addr.sin_family = AF_INET;
  addr.sin_addr = {ip};
  addr.sin_port = port;

  return addr;
}
