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

#define PORT "45110" //the port client will be connecting to

#define MAXDATASIZE 100
/*
// get sockaddr,IPv4orIPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int serverConnect(int argc, char *argv[], string un, string pw)
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"usage:clienthostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loopthroughalltheresultsandconnecttothefirst wecan
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) ==-1) {
            perror("client:socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) ==-1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) ==-1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);

    close(sockfd);
    
    return 0;
}*/
struct credentials {
    string user;
    string pw;
}



int main()
{
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

    string user_id = username +";"+ password;

    int M_SOCK = socket();
    send(M_SOCK ,id.c_str(), id.size(),0);
    bool validity = false; //set to actual verification in serverM
    if(validity)
    {
        cout << "[Client] You have been granted access. \n";
    }


    return 0;
}
