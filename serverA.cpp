#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <string>
#include <iostream>
#include <map>
#include <sstream>

#define MYPORT "41110"
#define PORTM "44110"
#define MAXBUFLEN 512
using namespace std;

map<string, string> onStartUp()
{
    ifstream infile;
    infile.open("members.txt");
    if (!infile.is_open()) {
        cerr << "Server A failed to open members.txt" << endl;
    }
    map<string, string> loginInfo; // creates new map
    string info;
    while(getline(infile, info))
    {
        // cout << info << endl;
        int space = info.find(' '); // finds the space position between username and password in txt doc
        string username = info.substr(0, space);
        // cout << username <<endl;
        string password = info.substr(space + 1);
        // cout << password << endl;
        loginInfo[username] = password; // populates map
    }

    //string test = loginInfo.find("Mary");
    infile.close();

    return loginInfo;
}

bool matchCredentials(string user_name, string encrypted_pw, map<string, string> users)
{
    if(users.count(user_name) > 0 && encrypted_pw == users[user_name])
    {
        return true;
    }
    else
    {
        return false;
    }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int startServer()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	int rv;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
	}

	freeaddrinfo(servinfo);

	printf("listener: waiting to recvfrom...\n");
    return sockfd;
}


string process_data(char *buf, int numbytes, map<string, string> users)
{
    buf[numbytes] = '\0';
    string rawData(buf);
    cout << "raw data: " << rawData << endl;
    istringstream stream(rawData);
    string parsedMsg, MsgType, username, password, socketNum;
    string newMsg = "";
    int parseNum = 0;
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            MsgType = parsedMsg;
        }
        if(parseNum == 2)
        {
            username = parsedMsg;
        }
        if(parseNum == 3)
        {
            password = parsedMsg;
        }
        if(parseNum == 4)
        {
            socketNum = parsedMsg;
        }
    }
    cout << "[Server A] Received username " << username << "and password "<< password << endl;
    if(matchCredentials(username, password, users))
    {
        cout << "[Server A] Member " << username << "has been authenticated"<< endl;
        return MsgType + ";" + username + ";SUCCESS;" + socketNum;
    }
    else
    {
        cout << "[Server A] Member " << username << " has been authenticated"<< endl;
        return MsgType + ";" + username + ";FAILURE;" + socketNum;
    }
}

string listen_pkts(int sockfd, map<string, string> users)
{
    int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	printf("listener: got packet from %s\n",
		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),	s, sizeof s));
	printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);
    string status = process_data(buf, numbytes, users);
    return status;
}

void udpSendMsg(string message, int mysockfd)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo("localhost", PORTM, &hints, &servinfo);

    sendto(mysockfd, message.c_str(), message.length(), 0, servinfo->ai_addr, servinfo->ai_addrlen);

	freeaddrinfo(servinfo);
}

int main()
{
    cout << "[Server A] Booting up using UDP on port 41110 ";
    string newMsg = "";
    map<string, string> users = onStartUp();
    int sockfd = startServer();
    while(true)
    {
        newMsg = listen_pkts(sockfd, users);
        cout << newMsg << endl;
        udpSendMsg(newMsg, sockfd);
    }
    close(sockfd);
    
    return 0;
}
