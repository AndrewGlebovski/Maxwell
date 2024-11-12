/**
 * \file
 * \brief Contains UDP client implementation.
*/

// ============================================================================

#include <stdio.h>
#include <unistd.h>
#include <netinet/ip.h>     // Socket API.
#include <arpa/inet.h>      // IP converting functions.

#include <utils/utils.hpp>

// ============================================================================

class Client {
 public:
  static constexpr size_t kMaxMessageSize = 508;

  // Creates socket.
  Client() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT(sock > -1, "Failed to create client socket.\n");
  }

  // Non-Copyable and Non-Movable
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  /// Performs NAT hole punching.
  int Connect(in_addr_t server_ip, in_port_t server_port) {
    sockaddr_in addr = CreateSockaddr(server_ip, server_port);

    int error = 0;

    error = sendto(sock, &error, sizeof(error), 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == sizeof(error), "Failed to send message to STUN server.\n");

    const size_t BufferSize = sizeof(friend_ip) + sizeof(friend_port);
    char buffer[BufferSize];

    error = recv(sock, buffer, BufferSize, 0);
    ASSERT(error == BufferSize, "Failed to receive STUN server response.\n");

    friend_ip = *reinterpret_cast<in_addr_t*>(buffer);
    friend_port = *reinterpret_cast<in_port_t*>(buffer + sizeof(friend_ip));

    addr = CreateSockaddr(friend_ip, friend_port);

    buffer[0] = '1';

    error = sendto(sock, buffer, 1, 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == 1, "Failed to send message to friend.\n");

    error = recv(sock, buffer, 1, 0);
    ASSERT(error == 1, "Failed to receive friend response.\n");

    if (*buffer == '1') {
      buffer[0] = '2';

      error = sendto(sock, buffer, 1, 0, (sockaddr*)(&addr), sizeof(addr));
      ASSERT(error == 1, "Failed to send message to friend.\n");

      return 1;
    }
    
    if (*buffer == '2') {
      return 2;
    }

    ASSERT(false, "Unknown package.\n");
  }

  /// Sends message with random bytes. 
  void Send(size_t msg_size) {
    ASSERT(msg_size < kMaxMessageSize, "Message size is too large.\n");

    char buffer[kMaxMessageSize];

    sockaddr_in addr = CreateSockaddr(friend_ip, friend_port);

    int error = sendto(sock, buffer, msg_size, 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == int(msg_size), "Failed to send message to friend.\n");
  }

  /// Receives random message. 
  void Receive(size_t msg_size) {
    ASSERT(msg_size < kMaxMessageSize, "Message size is too large.\n");

    char buffer[kMaxMessageSize];

    int error = recv(sock, buffer, msg_size, 0);
    ASSERT(error == int(msg_size), "Failed to receive message.\n");
  }

  ~Client() {
    ASSERT(close(sock) == 0, "Failed to close client socket.");
  }

 private:
  int sock = -1;
  in_addr_t friend_ip = 0;
  in_port_t friend_port = 0;
};

// ============================================================================

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: server_ip server_port\n");
    exit(1);
  }

  in_addr_t saddr = 0;
  ASSERT(inet_pton(AF_INET, argv[1], &saddr), "Invalid server ip.\n");
  
  in_port_t sport = atoi(argv[2]);
  sport = htons(sport);

  Client client;
  
  if (client.Connect(saddr, sport) == 1) {
    for (size_t i = 0; i < 10; i++) {
      client.Send(100);
      client.Receive(100);
    }
  } else {
    for (size_t i = 0; i < 10; i++) {
      client.Receive(100);
      client.Send(100);
    }
  }

  return 0;
}
