#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

#include <arpa/inet.h>

#define PORT 45110 //the port client will be connecting to

#define MAXDATASIZE 100

// get sockaddr,IPv4orIPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Created initial socket
int createSocket()
{
    int sockfd;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("client socket creation error");
        exit(1);
    }
    return sockfd;
}

// Server to connect client to
struct sockaddr_in defineServer()
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // clear memory
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    return server_addr;
}

void connectServer(int sockfd, struct sockaddr_in *server_addr)
{
    if(connect(sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1)
    {
        perror("client could not connect");
        exit(1);
    }
}

int main()
{
    char buf[512];    // Buffer for client data

    cout << "[Client] Booting up.\n[Client] Logging in.\n";
    string username = "";
    string password = "";
    cout << "Please enter the username: ";
    cin >> username;
    while(username.length() < 1 || username.length() > 50)
    {
        cout << "Please enter a valid username\n";
        cout << "Please enter the username: \t";
        cin >> username;
    }
    cout << "Please enter the password: ";
    cin >> password;
    while(password.length() < 1 || password.length() > 50)
    {
        cout << "Please enter a valid password\n";
        cout << "Please enter the password: ";
        cin >> password;
    }

    string user_id = string("credentials;") + username +";"+ password;

    int M_SOCK = createSocket();
    struct sockaddr_in addr = defineServer();
    connectServer(M_SOCK, &addr);
    string stopSend = "y";
    while(stopSend == "y")
    {
        send(M_SOCK ,user_id.c_str(), user_id.size(), 0);
        int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0);
        if(nbytes > 0)
        {
            buf[nbytes] = '\0';
            cout << "recevied: "<< buf << endl;
        }
        cout << "continue? y/n" << endl;
        cin >> stopSend;
    }
    /*bool validity = recv();
    if(validity)
    {
        cout << "[Client] You have been granted access. \n";
    }*/
    close(M_SOCK);

    return 0;
}
