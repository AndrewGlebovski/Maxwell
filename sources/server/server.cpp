/**
 * \file
 * \brief Contains server implementation.
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
 public:
  /// Creates socket and binds it to the provided address.
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

  /// Collects client addresses.
  void Listen() {
    sockaddr_in addr = {};
    socklen_t addr_len = sizeof(sockaddr_in);

    const size_t BufferSize = sizeof(in_addr_t) + sizeof(in_port_t);
    char buffer[BufferSize];

    int error = recvfrom(sock, buffer, BufferSize, 0, (sockaddr*)(&addr), &addr_len);
    ASSERT(error == BufferSize, "Failed to receive message.\n");

    info[clients].local_ip = *reinterpret_cast<in_addr_t*>(buffer);
    info[clients].local_port = *reinterpret_cast<in_port_t*>(buffer + sizeof(in_addr_t));
    info[clients].global_ip = addr.sin_addr.s_addr;
    info[clients].global_port = addr.sin_port;
    info[clients].id = clients;
    clients++;

    printf("Client %lu arrived.\n", clients);
  }

  /// Sends clients addresses to each other.
  bool Rendezvous() {
    if (clients < 2) {
      return false;
    }

    printf("Rendezvous.\n", clients);

    SendAddress(info[0], info[1]);
    SendAddress(info[1], info[0]);

    return true;
  }

  /// Closes socket.
  ~Server() {
    ASSERT(close(sock) == 0, "Failed to close.\n");
  }

 private:
  /// Sends ip and port of source client to destination client. 
  void SendAddress(const ClientInfo& src, const ClientInfo& dst) {
    char buffer[sizeof(ClientInfo)];
    WriteClientInfo(buffer, src);

    sockaddr_in addr = CreateSockaddr(dst.global_ip, dst.global_port);

    int error = sendto(sock, buffer, sizeof(ClientInfo), 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == sizeof(ClientInfo), "Failed to send message.\n");
  }

  /// Writes ClientInfo structure to buffer. 
  void WriteClientInfo(void* buffer, const ClientInfo& info) {
    memcpy(buffer, &info, sizeof(ClientInfo));
  }

  int sock = -1;
  ClientInfo info[2];
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
