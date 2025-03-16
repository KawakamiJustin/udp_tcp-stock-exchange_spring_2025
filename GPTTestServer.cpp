#include <cstdio>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        return -1;
    }

    // Bind socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(8080);      // Port number

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return -1;
    }

    std::cout << "Server is listening on port 8080..." << std::endl;

    // Accept client connection
    client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        perror("Accept failed");
        return -1;
    }

    // Communicate with the client
    char buffer[1024] = {0};
    const char* response = "Hello from server!";
    read(client_socket, buffer, sizeof(buffer));
    std::cout << "Client says: " << buffer << std::endl;
    send(client_socket, response, strlen(response), 0);

    // Close sockets
    close(client_socket);
    close(server_fd);

    return 0;
}