/**
 * \file
 * \brief Contains client implementation.
*/

// ============================================================================

#include <stdio.h>        // printf
#include <unistd.h>       // close
#include <netinet/ip.h>   // Socket API.
#include <arpa/inet.h>    // IP converting functions.
#include <errno.h>        // errno
#include <sys/time.h>     // timeval

#include <utils/utils.hpp>

// ============================================================================

class Client {
 public:
  static constexpr size_t kMaxMessageSize = 508;

  // Creates socket.
  Client(in_addr_t ip, in_port_t port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT(sock > -1, "Failed to create client socket.\n");

    sockaddr_in addr = CreateSockaddr(ip, port);

    int error = bind(sock, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == 0, "Failed to bind client socket.\n");

    // Костыль, чтобы запомнить наш адрес и передать в Connect
    friend_ip = ip;
    friend_port = port;
  }

  // Non-Copyable and Non-Movable
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  /// Performs NAT hole punching.
  int Connect(in_addr_t server_ip, in_port_t server_port) {
    ClientInfo info = {};
    info.local_ip = friend_ip;
    info.local_port = friend_port;

    // Sending local address to rendezvous server

    sockaddr_in addr = CreateSockaddr(server_ip, server_port);

    int error = sendto(sock, &info, sizeof(ClientInfo), 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == sizeof(ClientInfo), "Failed to send message to STUN server.\n");

    // Waiting for rendezvous server response

    error = recv(sock, &info, sizeof(ClientInfo), 0);
    ASSERT(error == sizeof(ClientInfo), "Failed to receive STUN server response.\n");

    set_recv_timeout(0, 1000);

    // Punching via friend global address

    addr = CreateSockaddr(info.global_ip, info.global_port);

    if (GlobalPunch(addr)) {
      printf("Global connection established.\n");

      set_recv_timeout(10, 0);

      friend_ip = info.global_ip;
      friend_port = info.global_port;

      return info.id;
    }

    // Punching via friend local address

    addr = CreateSockaddr(info.local_ip, info.local_port);

    if (LocalPunch(addr)) {
      printf("Local connection established.\n");

      set_recv_timeout(10, 0);

      friend_ip = info.local_ip;
      friend_port = info.local_port;

      return info.id;
    }

    // Failed to punch

    ASSERT(false, "Can't establish connection.\n");
  }

  /// Sends message with random bytes. 
  void Send(size_t msg_size) {
    ASSERT(msg_size < kMaxMessageSize, "Message size is too large.\n");

    char buffer[kMaxMessageSize];

    sockaddr_in addr = CreateSockaddr(friend_ip, friend_port);

    int error = sendto(sock, buffer, msg_size, 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == int(msg_size), "Failed to send message to friend.\n");

    printf("Message sent.\n");
  }

  /// Receives random message. 
  void Receive(size_t msg_size) {
    ASSERT(msg_size < kMaxMessageSize, "Message size is too large.\n");

    char buffer[kMaxMessageSize];

    int error = recv(sock, buffer, msg_size, 0);
    ASSERT(error == int(msg_size), "Failed to receive message.\n");

    printf("Message received.\n");
  }

  ~Client() {
    ASSERT(close(sock) == 0, "Failed to close client socket.");
  }

 private:
  /// Works if clients are in the same local network.
  bool LocalPunch(sockaddr_in addr) {
    char buffer = '1';

    int error = sendto(sock, &buffer, 1, 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == 1, "Failed to send message to friend.\n");

    error = recv(sock, &buffer, 1, 0);
    if (error == -1) {
      if (errno == EAGAIN) {
        return false;
      }

      ASSERT(false, "Failed to receive friend response.\n");
    }

    return true;
  }

  /// Always works except for clients in the same network.
  bool GlobalPunch(sockaddr_in addr) {
    char buffer = '1';

    int error = sendto(sock, &buffer, 1, 0, (sockaddr*)(&addr), sizeof(addr));
    ASSERT(error == 1, "Failed to send message to friend.\n");

    error = recv(sock, &buffer, 1, 0);
    if (error == -1) {
      if (errno == EAGAIN) {
        return false;
      }

      ASSERT(false, "Failed to receive friend response.\n");
    }

    if (buffer == '1') {
      buffer = '2';

      error = sendto(sock, &buffer, 1, 0, (sockaddr*)(&addr), sizeof(addr));
      ASSERT(error == 1, "Failed to send message to friend.\n");

      error = recv(sock, &buffer, 1, 0);
      ASSERT(error == 1, "Failed to receive confirmation from friend.\n");

      return true;
    }
    
    if (buffer == '2') {
      error = sendto(sock, &buffer, 1, 0, (sockaddr*)(&addr), sizeof(addr));
      ASSERT(error == 1, "Failed to send message to friend.\n");

      return true;
    }

    ASSERT(false, "Unknown package.\n");
  }

  void set_recv_timeout(size_t seconds, size_t microseconds) {
    struct timeval timeout;      
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    int error = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    ASSERT(error != -1, "Failed to set receive timeout.\n");
  }

  void set_send_timeout(size_t seconds, size_t microseconds) {
    struct timeval timeout;      
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    int error = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    ASSERT(error != -1, "Failed to set send timeout.\n");
  }

  int sock = -1;
  in_addr_t friend_ip = 0;
  in_port_t friend_port = 0;
};

// ============================================================================

int main(int argc, char* argv[]) {
  if (argc != 5) {
    printf("Usage: client_ip client_port server_ip server_port\n");
    exit(1);
  }

  in_addr_t caddr = 0;
  ASSERT(inet_pton(AF_INET, argv[1], &caddr), "Invalid server ip.\n");
  
  in_port_t cport = atoi(argv[2]);
  cport = htons(cport);

  in_addr_t saddr = 0;
  ASSERT(inet_pton(AF_INET, argv[3], &saddr), "Invalid server ip.\n");
  
  in_port_t sport = atoi(argv[4]);
  sport = htons(sport);

  Client client(caddr, cport);
  
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
