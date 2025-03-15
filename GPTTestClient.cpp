#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 address to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address / Address not supported" << std::endl;
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection failed" << std::endl;
        return -1;
    }

    // Communicate with the server
    const char* message = "Hello from client!";
    char buffer[1024] = {0};
    send(sock, message, strlen(message), 0);
    std::cout << "Message sent to server" << std::endl;
    read(sock, buffer, sizeof(buffer));
    std::cout << "Server says: " << buffer << std::endl;

    // Close socket
    close(sock);

    return 0;
}
