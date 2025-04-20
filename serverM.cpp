#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <iostream>
#include <sstream>
#include <tuple>

#define PORT "45110"   // Port we're listening on
#define UDP_PORT "44110"
#define AUTH_PORT "41110"
#define PORT_PORT "42110"
#define QUOT_PORT "43110"
#define MAXBUFLEN 512

using namespace std;

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    freeaddrinfo(ai); // All done with this

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = (struct pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read
    (*pfds)[*fd_count].revents = 0;

    (*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

string passwordEncrypt(string receivedMsg)
{
    istringstream stream(receivedMsg);
    string parsedMsg, MsgType, usersname, password, socketNum;
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
            usersname = parsedMsg;
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
    cout << "[Server M] Received username " << usersname << " and password ****" << endl;
    string encryptedPassword = "";
    int shift = 3;
    string uAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string lAlphabet = "abcdefghijklmnopqrstuvwxyz";
    string numbers = "0123456789";
    for (char c : password)
    {
        if(isupper(c))
        {
            int index = uAlphabet.find(c);
            int newIndex = (index + shift) % uAlphabet.length();
            encryptedPassword += uAlphabet[newIndex];
        }
        else if(islower(c))
        {
            int index = lAlphabet.find(c);
            int newIndex = (index + shift) % lAlphabet.length();
            encryptedPassword += lAlphabet[newIndex];
        }
        else if(isdigit(c))
        {
            int index = numbers.find(c);
            int newIndex = (index + shift) % numbers.length();
            encryptedPassword += numbers[newIndex];
        }
        else
        {
            encryptedPassword += c;
        }
    }
    newMsg = MsgType + ";" + usersname + ";" + encryptedPassword + ";" + socketNum;
    return newMsg;
}

void updateQuote(string message)
{

}


/*
string udpForwarding(string receivedMsg)
{
    istringstream stream(receivedMsg);
    string parsedMsg, MsgType, usersname, password, socketNum;
    string newMsg = "";
    cout << "operationType received message: " << receivedMsg << endl;
    string choice;
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
            usersname = parsedMsg;
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
    return "";
}
*/
int startUDPServer()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	int rv;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, UDP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
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

	cout << "[Server M] Booting up using UDP on port " << UDP_PORT << endl;
    return sockfd;
}

string UDPrecv(int sockfd)
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
    buf[numbytes] = '\0';
    string rawData(buf);
    return rawData;
}

void UDPsend(string serverPort, string message, int mysockfd)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo("localhost", serverPort.c_str(), &hints, &servinfo);

    sendto(mysockfd, message.c_str(), message.length(), 0, servinfo->ai_addr, servinfo->ai_addrlen);

	freeaddrinfo(servinfo);
}

string udpSendMsg(string serverPort, string message, int mysockfd, bool updatePrice)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo("localhost", serverPort.c_str(), &hints, &servinfo);

    sendto(mysockfd, message.c_str(), message.length(), 0, servinfo->ai_addr, servinfo->ai_addrlen);

	freeaddrinfo(servinfo);
    if (!updatePrice)
    {
        string listenData = UDPrecv(mysockfd);
        cout << "Received "<< listenData <<" from server port " << serverPort << endl;

        return listenData;
    }
    else
    {
        return "";
    }
}

int sockNum(string message)
{
    int socketpos = message.find_last_of(';') + 1;
    string socketString = message.substr(socketpos);
    return stoi(socketString);
}
/*
tuple<string, string, bool> operationType(string receivedMsg)
{
    string msg = "";
    istringstream stream(receivedMsg);
    cout << "operationType received message: " << receivedMsg << endl;
    string choice;
    getline(stream, choice, ';');
    if(choice == "credentials")
    {
        msg = passwordEncrypt(receivedMsg);
        return make_tuple(msg, AUTH_PORT, false);
    }
    if(choice == "position")
    {
        msg = receivedMsg;
        return make_tuple(msg, PORT_PORT, false);
    }
    if(choice == "transact")
    {
        msg = receivedMsg;
        return make_tuple(msg, PORT_PORT, true);
    }
    if(choice == "quote")
    {
        msg = receivedMsg;
        return make_tuple(msg, QUOT_PORT, false);
    }
    return make_tuple("ERROR","", false);
}*/
string TCPrecv(int client__sockfd)
{
    char buf[MAXBUFLEN];
    memset(buf, 0, sizeof(buf)); // clear buffer
    int nbytes = recv(client__sockfd, buf, sizeof buf, 0);
    buf[nbytes] = '\0';
    string rawData(buf);
    return rawData;
}

double avgBuyPrice(int quantity, double price)
{
    double avgPrice = (quantity * price)/quantity;
    return avgPrice;
}

double avgBuyPriceNew(int quantity, double price, double avgPriceOld, double quantityOld)
{
    double oldCost = quantityOld * avgBuyPrice(quantityOld, avgPriceOld);
    double newCost = quantity * avgBuyPrice(quantity, price);
    double avgPrice = (oldCost + newCost)/(quantity + quantityOld);
    return avgPrice;
}

void operationType(string receivedMsg, int sockfd, int client__sockfd)
{
    string msg;
    istringstream stream(receivedMsg);
    cout << "operationType received message: " << receivedMsg << endl;
    string choice, subOp, userID, stockName, shares, price, clientSock, confirmation;
    getline(stream, choice, ';');
    if(choice == "credentials")
    {
        msg = passwordEncrypt(receivedMsg);
        UDPsend(AUTH_PORT, msg, sockfd);
        msg = UDPrecv(sockfd);
        send(client__sockfd, msg.c_str(), msg.size(), 0);
    }
    if(choice == "position")
    {
        // position;<user>;<socket_Num>
        msg = receivedMsg;
        UDPsend(QUOT_PORT, msg, sockfd);
        msg = UDPrecv(sockfd);
        // Parse all stocks/quantities/prices
        // Request stocks for current prices
        // Parse stocks/current prices
        // Perform profit calcs
        // make new message to send to client
        // send to client
    }
    if(choice == "transact")
    {
        getline(stream, subOp, ';');
        getline(stream, userID, ';');
        getline(stream, stockName, ';');
        getline(stream, shares, ';');
        getline(stream, clientSock, ';');
        msg = string("quote;") + stockName + string(";") + clientSock;
        UDPsend(QUOT_PORT, msg, sockfd);
        string quoteMsg = UDPrecv(sockfd);
        // retrieve;<user>;<stock_name>;<socket_Num>
	    // update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
        if(subOp == "buy")
        {
            send(client__sockfd, quoteMsg.c_str(), quoteMsg.size(), 0);
            msg = TCPrecv(client__sockfd);
            istringstream stream(msg);
            getline(stream, confirmation, ';');
            if (confirmation == "N")
            {
                return;
            }
            else 
            {
                string requestMsg = "retrieve;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(PORT_PORT, requestMsg, sockfd);
                string clientPos = UDPrecv(sockfd); 
                // user;stock;quantity;price;indexInVector;socketNum    Valid
                // user;targetStock;NA;NA;-1;socketNum                  User does not own stock currently
                // user;NA;NA;NA;-1;socketNum;                          User Not Found
                cout << "received from portfolio: " << clientPos << endl;
                // parse message to extract information about the user's position
            }
        }
        else if (subOp == "sell")
        {
            string posMsg = "retrieve;" + userID + ";" + stockName + ";" + clientSock;
            UDPsend(PORT_PORT, posMsg, sockfd);
            string sellPos = UDPrecv(sockfd);
            // user;stock;quantity;price;indexInVector;socketNum    Valid
            // user;targetStock;NA;NA;-1;socketNum                  User does not own stock currently
            // user;NA;NA;NA;-1;socketNum;                          User Not Found
            istringstream userPos(sellPos);
            string operation, avgPrice;
            getline(userPos, operation, ';');
            getline(userPos, userID, ';');
            getline(userPos, stockName, ';');
            if(stockName == "NA")
            {
                
            }
            else
            {
                getline(userPos, shares, ';');
                getline(userPos, avgPrice, ';');
            }
            
        }
        
    }
    if(choice == "quote")
    {
        msg = receivedMsg;
        UDPsend(QUOT_PORT, msg, sockfd);
        msg = UDPrecv(sockfd);
        send(client__sockfd, msg.c_str(), msg.size(), 0);
    }
    //return msg;
}

// Main
int main(void)
{
    int listener;     // Listening socket descriptor
    // tuple<string, string, bool> operation;
    int newfd;        // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;
    int udp_sockfd; // udp socket descriptor

    char buf[MAXBUFLEN];    // Buffer for client data

    char remoteIP[INET6_ADDRSTRLEN];

    udp_sockfd = startUDPServer();

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = (struct pollfd *)malloc(sizeof *pfds * fd_size);

    // Set up and get a listening socket
    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    // Add the listener to set
    pfds[0].fd = listener;
    pfds[0].events = POLLIN; // Report ready to read on incoming connection

    fd_count = 1; // For the listener


    // Main loop
    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for(int i = 0; i < fd_count; i++) {

            // Check if someone's ready to read
            if (pfds[i].revents & (POLLIN | POLLHUP)) { // We got one!!

                if (pfds[i].fd == listener) {
                    // If listener is ready to read, handle new connection

                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on ""socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                            get_in_addr((struct sockaddr*)&remoteaddr),
                            remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // If not the listener, we're just a regular client
                    memset(buf, 0, sizeof(buf)); // clear buffer
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

                    int sender_fd = pfds[i].fd;

                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);

                    } else {
                        buf[nbytes] = '\0';
                        // printf("Server received from socket %d: %.*s\n", pfds[i].fd, nbytes, buf);


                        // We got some good data from a client
                        string recMessage(buf);
                        recMessage += ";" + to_string(sender_fd);
                        // operation = operationType(recMessage);
                        /*string newMessage = get<0>(operation);
                        string serverPort = get<1>(operation);
                        bool updatePrice = get<2>(operation);
                        cout << "Sending: " << newMessage << " to server " << serverPort << endl;
                        string responseMsg = udpSendMsg(serverPort, newMessage, udp_sockfd, updatePrice);
                        if (updatePrice)
                        {
                            string upMsg = "update;" + newMessage;
                            string updateMsg = udpSendMsg(QUOT_PORT, upMsg, udp_sockfd, updatePrice);
                        }*/
                        operationType(recMessage, udp_sockfd, sender_fd); // new version

                        // Except the listener
                        /*
                        if (C_SOCK != listener) {
                            if (send(C_SOCK, responseMsg.c_str(), responseMsg.size(), 0) == -1) {
                                perror("send");
                            }
                            else
                            {
                                // Log what was sent
                                printf("Server sent to socket %d: %.*s\n", C_SOCK, int(responseMsg.size()), responseMsg.c_str());
                            }
                        }*/
                        
                    }
                } // END handle data from client
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
}
