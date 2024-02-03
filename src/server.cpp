#include <string_view>
#include "expected.hpp"

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std::literals;

constexpr auto VANILLA_200_OK_RESPONSE = "HTTP/1.1 200 OK\r\n\r\n"sv;
constexpr int DEFAULT_PORT = 4221;

enum class socket_error {
    cant_create_socket,
    setsockopt,
    cant_bind,
    cant_listen,
};

auto setup_server_socket() -> std::expected<int, socket_error>
{
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   return std::unexpected_23(socket_error::cant_create_socket);
  }

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    return std::unexpected_23(socket_error::setsockopt);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(DEFAULT_PORT);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
      return std::unexpected_23(socket_error::cant_bind);
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
      return std::unexpected_23(socket_error::cant_listen);
  }

  return server_fd;
}

int main(int argc, char **argv) {
  if (const auto server = setup_server_socket(); server.has_value()) {
      // Serve...
      auto server_fd = *server;

      while (true) {
          struct sockaddr_in client_addr;
          int client_addr_len = sizeof(client_addr);

          std::cout << "Waiting for a client to connect...\n"sv;

          auto new_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
          // Assume we didn't get -1
          if (new_fd != -1) {
              std::cout << "Client connected\n"sv;
              if (send(new_fd, (const void *)VANILLA_200_OK_RESPONSE.data(), VANILLA_200_OK_RESPONSE.length(), 0) == -1) {
                  std::cout << "Ouch, couldn't send\n"sv;
              }
              close(new_fd);
          }
      }

      // Won't ever happen (sad!)
      close(server_fd);
  } else {
      switch (server.error()) {
          case socket_error::cant_create_socket:
              std::cerr << "Failed to create server socket\n"sv;
              break;
          case socket_error::setsockopt:
              std::cerr << "setsockopt failed\n"sv;
              break;
          case socket_error::cant_bind:
              std::cerr << "Failed to bind to port "sv << DEFAULT_PORT << '\n';
              break;
          case socket_error::cant_listen:
              std::cerr << "listen failed\n"sv;
              break;
      }
  }
  return 0;
}
