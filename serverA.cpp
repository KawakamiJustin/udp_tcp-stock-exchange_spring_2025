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
#include <algorithm>
#include <cctype>

#define MYPORT "41110" // UDP Port Number for Server A
#define PORTM "44110" // UDP Port Number for Server M
#define MAXBUFLEN 512 // Max supported buffer size for incoming messages
using namespace std;

// converts given string to lowercase
string lowercaseConvert(string userName)
{
    string lowercaseName = "";
    for(char letter : userName)
    {
        lowercaseName += tolower(letter);
    } 
    return lowercaseName;
}

// Converts members.txt to a map with usernames as keys and encrypted passwords as values
map<string, string> onStartUp()
{
    ifstream infile;
    infile.open("members.txt"); // opens file
    if (!infile.is_open()) {
        cerr << "Server A failed to open members.txt" << endl;
    }
    map<string, string> loginInfo; // creates new map
    string info;
    while(getline(infile, info))
    {
        int space = info.find(' '); // finds the space position between username and password in txt doc
        string username = info.substr(0, space);
        string password = info.substr(space + 1);
        string lowercaseUser = lowercaseConvert(username);
        loginInfo[lowercaseUser] = password; // populates map
    }
    infile.close(); // closes file
    return loginInfo; // returns map created from file
}

// Function to ensure username is valid and the encrypted password matches the username key
bool matchCredentials(string user_name, string encrypted_pw, map<string, string> users)
{
    //converts username is converted to lowercase to ensure compatibility
    string userKey = lowercaseConvert(user_name);
    if(users.count(userKey) > 0 && encrypted_pw == users[userKey])
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Taken from Beej listener.c (datagram sockets example)
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Partially inspired (converted to function instead of main) from Beej listener.c (datagram sockets example)
// Starts udp listener socket to listen for incoming transmissions
int startServer()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	int rv;

    memset(&hints, 0, sizeof hints); // clears buffer
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP datagram
	hints.ai_flags = AI_PASSIVE; // use my IP

    // Resolves address info for main server port number at 127.0.0.1
	if ((rv = getaddrinfo("localhost", MYPORT, &hints, &servinfo)) != 0) {
        // Error debugging commented out
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
            // Error debugging commented out
			//perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
            // Error debugging commented out
			//perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
        // Error debugging commented out
		//fprintf(stderr, "listener: failed to bind socket\n");
	}

    // frees up memory from getaddrinfo
	freeaddrinfo(servinfo);
	// printf("listener: waiting to recvfrom...\n");

    // socket file descriptor to use during send and receiving commands
    return sockfd;
}

// Processes incoming message from server M for authentication
string process_data(char *buf, int numbytes, map<string, string> users)
{
    buf[numbytes] = '\0';
    string rawData(buf);
    istringstream stream(rawData);
    string parsedMsg, MsgType, username, password, socketNum;
    string newMsg = "";
    int parseNum = 0;

    // Extract the message type (credentials), username, encrypted password, and assigned client socket number for server M
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
    cout << "[Server A] Received username " << username << " and password ******" << endl;

    // function calls credential match function to check if user credentials match the database 
    // returns a SUCCESS message if credentials match and FAILURE if they do not
    if(matchCredentials(username, password, users))
    {
        cout << "[Server A] Member " << username << " has been authenticated"<< endl;
        return MsgType + ";" + username + ";SUCCESS;" + socketNum;
    }
    else
    {
        cout << "[Server A] The username " << username << " or password ****** is incorrect"<< endl;
        return MsgType + ";" + username + ";FAILURE;" + socketNum;
    }
}

// Partially inspired (converted to function instead of main) from Beej listener.c (datagram sockets example)
// Listens for incoming transmissions to socket set up before and converts incoming char buffer to a string for post processing
string listen_pkts(int sockfd, map<string, string> users)
{
    int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	addr_len = sizeof their_addr;
    memset(buf, 0, sizeof(buf)); // clear buffer
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		//perror("recvfrom");
		exit(1);
	}

	//printf("listener: got packet from %s\n",
		//inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),	s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	//printf("listener: packet contains \"%s\"\n", buf);
    string status = process_data(buf, numbytes, users);
    return status;
}

// Partially inspired (converted to function instead of main) from Beej talker.c (datagram sockets example)
// Sends udp message to main server
// Takes in string as input and the socket file description of server A
void udpSendMsg(string message, int mysockfd)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints); // clears buffer before sending
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP datagram

    // Gets address info of the main server with PORTM (Main server Port Number)
	getaddrinfo("localhost", PORTM, &hints, &servinfo);

    sendto(mysockfd, message.c_str(), message.length(), 0, servinfo->ai_addr, servinfo->ai_addrlen);

    // Clears address info of server M from memory
	freeaddrinfo(servinfo);
}

// Main function that setups up listener and populates map with credentials
// Continously listens then responds to transmissions
int main()
{
    cout << "[Server A] Booting up using UDP on port 41110" << endl;
    string newMsg = "";
    map<string, string> users = onStartUp();
    int sockfd = startServer();
    while(true)
    {
        newMsg = listen_pkts(sockfd, users);
        udpSendMsg(newMsg, sockfd);
    }
    close(sockfd);
    
    return 0;
}
