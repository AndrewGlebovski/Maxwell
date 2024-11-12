/**
 * \file
 * \brief Contains UDP server implementation.
*/

// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/ip.h>     // Socket API.
#include <arpa/inet.h>      // IP converting functions.

#include <utils/utils.hpp>

// ============================================================================

class Server {
  static constexpr size_t kBufferSize = 508;
 
 public:
  // Creates socket and binds it to the provided address.
  Server(in_addr_t ip, in_port_t port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT(sock > -1, "Failed to create passive socket.\n");

    sockaddr_in addr = CreateSockaddr(ip, port);

    int error = bind(sock, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == 0, "Failed to bind server socket.\n");
  }

  // Non-Copyable and Non-Movable
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  // Collects client addresses.
  void Listen() {
    sockaddr_in addr = {};
    socklen_t addr_len = sizeof(sockaddr_in);

    const size_t BufferSize = sizeof(in_addr_t) + sizeof(in_port_t);
    char buffer[BufferSize];

    int error = recvfrom(sock, buffer, sizeof(int), 0, (sockaddr*)(&addr), &addr_len);
    ASSERT(error == sizeof(int), "Failed to receive message.\n");

    client_ip[clients] = *reinterpret_cast<in_addr_t*>(buffer);
    client_port[clients] = *reinterpret_cast<in_port_t*>(buffer + sizeof(in_addr_t));
    clients++;
  }

  /// Sends clients addresses to each other.
  bool Rendezvous() {
    if (clients < 2) {
      return false;
    }

    SendAddress(0, 1);
    SendAddress(1, 0);

    return true;
  }

  // Closes socket.
  ~Server() {
    ASSERT(close(sock) == 0, "Failed to close.\n");
  }

 private:
  /// Sends ip and port of source client to destination client. 
  void SendAddress(size_t src, size_t dst) {
    const size_t BufferSize = sizeof(in_addr_t) + sizeof(in_port_t);
    char buffer[BufferSize];

    sockaddr_in addr = CreateSockaddr(client_ip[dst], client_port[dst]);

    *reinterpret_cast<in_addr_t*>(buffer) = client_ip[src];
    *reinterpret_cast<in_port_t*>(buffer + sizeof(in_addr_t)) = client_port[src];

    int error = sendto(sock, buffer, BufferSize, 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == BufferSize, "Failed to send message.\n");
  }

  int sock = -1;
  in_addr_t client_ip[2] = {0, 0};
  in_port_t client_port[2] = {0, 0};
  size_t clients = 0;
};

// ============================================================================

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: server_ip server_port\n");
    exit(1);
  }

  in_addr_t server_addr = 0;
  ASSERT(inet_pton(AF_INET, argv[1], &server_addr), "Invalid IP.\n");
  
  in_port_t server_port = atoi(argv[2]);
  server_port = htons(server_port);

  Server server(server_addr, server_port);

  while (!server.Rendezvous()) {
    server.Listen();
  }

  return 0;
}
