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
#include <iomanip>
#include <vector>
#include <map>

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

    memset(buf, 0, MAXBUFLEN);
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	//printf("listener: got packet from %s\n",
	//	inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),	s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	//buf[numbytes] = '\0';
	//printf("listener: packet contains \"%s\"\n", buf);
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

struct stock
{
    string stockName;
	int quantity;
	double avgPrice;
};

double profitCalc(map<string, double> quotes, vector<stock> userPosition)
{
    double profit = 0.00;
    double quotePrice, tempProfit;
    int quantity;
    for (int i = 0; i < userPosition.size(); i++)
    {
        quotePrice = quotes[userPosition[i].stockName];
        quantity = userPosition[i].quantity;
        tempProfit = quantity * (quotePrice - userPosition[i].avgPrice);
        profit += tempProfit;
        //cout << userPosition[i].stockName << " profit: " << tempProfit <<"\nUser Profit: " << to_string(profit) << endl;
    }
    return profit;
}

string constructNewMsg(string userPort, string stockQuotes)
{
    map<string, double> quotes;
    vector<stock> userPosition;
    double stockPrice;
    int shareQuantity;
    string stockName, userName, sockNum;
    stock newStock;
    /* 
    User Portfolio: position;James;S1;400;353.47;S2;178;500.00;5
    Current Quotes: quote;start;S1;697.46;S2;464.61;end;
    */
    istringstream stockString(stockQuotes);
    string parsedMsg, msgType, startFlag;
    int parseNum = 0;
    while(getline(stockString, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        else if(parseNum == 2)
        {
            startFlag = parsedMsg;
        }
        else if (parsedMsg == "end")
        {
            break;
        }
        else if (parseNum % 2 == 1)
        {
            stockName = parsedMsg;
        }
        else
        {
            stockPrice = stod(parsedMsg);
            quotes[stockName] = stockPrice;
        }
    }

    istringstream userString(userPort);
    parseNum = 0;
    //position;James;S1;400;353.47;S2;178;500.00;end;5
    while(getline(userString, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        else if(parseNum == 2)
        {
            userName = parsedMsg;
        }
        else if (parsedMsg == "NA")
        {
            string clientMsg = msgType + ";" + userName + ";position_FAIL;";
            break;
        }
        else if (parsedMsg == "end")
        {
            break;
        }
        else if (parseNum % 3 == 0)
        {
            stockName = parsedMsg;
        }
        else if (parseNum % 3 == 1)
        {
            shareQuantity = stoi(parsedMsg);
        }
        else if (parseNum % 3 == 2)
        {
            stockPrice = stod(parsedMsg);
            newStock.stockName = stockName;
            newStock.avgPrice = stockPrice;
            newStock.quantity = shareQuantity;
            userPosition.push_back(newStock);
        }
    }
    double totalProfit = profitCalc(quotes, userPosition);
    ostringstream concatProfit;
    concatProfit << fixed << setprecision(2) << totalProfit;
    string clientMsg = userPort + ";" + concatProfit.str();
    return clientMsg;
}

int sockNum(string message)
{
    int socketpos = message.find_last_of(';') + 1;
    string socketString = message.substr(socketpos);
    return stoi(socketString);
}

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

double avgBuyPriceNew(int quantity, double price, int quantityOld, double avgPriceOld)
{
    double oldCost = quantityOld * avgBuyPrice(quantityOld, avgPriceOld);
    double newCost = quantity * avgBuyPrice(quantity, price);
    double avgPrice = (oldCost + newCost)/(quantity + quantityOld);
    return avgPrice;
}

void operationType(string receivedMsg, int sockfd, int client_sockfd)
{
    string msg;
    istringstream stream(receivedMsg);
    // cout << "operationType received message: " << receivedMsg << endl;
    string choice, subOp, userID, stockName, shares, price, clientSock, confirmation;
    getline(stream, choice, ';');
    if(choice == "credentials")
    {
        msg = passwordEncrypt(receivedMsg);
        UDPsend(AUTH_PORT, msg, sockfd);
        cout << "[Server M] Sent the authetication request to Server A" << endl;
        msg = UDPrecv(sockfd);
        cout << "[Server M] Received the response from server A using UDP over " << UDP_PORT << endl;
        send(client_sockfd, msg.c_str(), msg.size(), 0);
        cout << "[Server M] Sent the respoonse from server A to the client using TCP over port " << PORT << endl;
    }
    if(choice == "position")
    {
        // position;<user>;<socket_Num>
        msg = receivedMsg;
        getline(stream, userID, ';');
        cout << "[Server M] Received a position request from Member to check " << userID << "'s gain using TCP over port " << PORT << "." << endl;
        UDPsend(PORT_PORT, msg, sockfd);
        cout << "[Server M] Forwarded the position request to server P." << endl;
        string userPort = UDPrecv(sockfd);
        cout << "[Server M] Received user's portfolio from server P using UDP over " << UDP_PORT << endl;
        UDPsend(QUOT_PORT, "quote;all", sockfd);
        string stockQuotes = UDPrecv(sockfd);
        string clientPos = constructNewMsg(userPort, stockQuotes);
        cout << "[Server M] Forwarded the gain to the client." << endl;
        send(client_sockfd, clientPos.c_str(), clientPos.size(), 0);
    }
    if(choice == "transact")
    {
        getline(stream, subOp, ';');
        getline(stream, userID, ';');
        getline(stream, stockName, ';');
        getline(stream, shares, ';');
        getline(stream, clientSock, ';');
        if(subOp == "sell")
        {
            cout << "[Server M] Received a sell request from member " << userID << " using TCP over port " << PORT << endl;
        }
        else
        {
            cout << "[Server M] Received a buy request from member " << userID << " using TCP over port " << PORT << endl;
        }
        string Quote_msg = string("quote;") + stockName + string(";") + clientSock;
        UDPsend(QUOT_PORT, Quote_msg, sockfd);
        cout << "[Server M] Sent quote request to server Q." << endl;
        string quoteMsg = UDPrecv(sockfd);
        cout << "[Server M] Received quote response from server Q." << endl;
        // retrieve;<user>;<stock_name>;<socket_Num>
	    // update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
        if(subOp == "buy")
        {
            send(client_sockfd, quoteMsg.c_str(), quoteMsg.size(), 0);
            cout << "[Server M] Sent the buy confirmation to the client." << endl;
            string conMsg = TCPrecv(client_sockfd);
            istringstream conStream(conMsg);
            getline(conStream, confirmation, ';');
            if (confirmation == "N")
            {

                cout << "[Server M] Buy denied." << endl;
                string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(QUOT_PORT, updateQuote, sockfd);
                cout << "[Server M] Sent a time forward request for " << stockName << endl;
                return;
            }
            else 
            {
                cout << "[Server M] Buy approved." << endl;
                string forwardConf = confirmation + ";buy";
                UDPsend(PORT_PORT, forwardConf, sockfd);
                cout << "[Server M] Forwarded the buy confirmation response to Server P." << endl;
                string quote_MsgType, quote_stockName, quote_price, quote_clientSock;
                // quote;S1;697.46;5
                istringstream quotStream(quoteMsg);
                getline(quotStream, quote_MsgType, ';');
                getline(quotStream, quote_stockName, ';');
                getline(quotStream, quote_price, ';');
                getline(quotStream, quote_clientSock, ';');
                string requestMsg = "retrieve;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(PORT_PORT, requestMsg, sockfd);
                //sleep(1);
                string clientPos = UDPrecv(sockfd); 
                // user;stock;quantity;price;indexInVector;socketNum    Valid
                // user;targetStock;NA;NA;-1;socketNum                  User does not own stock currently
                // user;NA;NA;NA;-1;socketNum;                          User Not Found
                // cout << "received from portfolio: " << clientPos << endl;
                // parse message to extract information about the user's position
                string position_userID, position_stockName, position_shares, position_index, position_price, position_clientSock;
                istringstream clientInfo(clientPos);
                getline(clientInfo, position_userID, ';');
                getline(clientInfo, position_stockName, ';');
                getline(clientInfo, position_shares, ';');
                getline(clientInfo, position_price, ';');
                getline(clientInfo, position_index, ';');
                getline(clientInfo, position_clientSock, ';');

                if(position_shares == "NA" && position_stockName == stockName)
                {
                    double avgBuy = avgBuyPrice(stoi(shares),stod(quote_price));
                    // update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    string updatePos = "update;" + position_userID + ";" + position_stockName + ";" + shares + ";" + to_string(avgBuy) + ";" + position_index + ";" + position_clientSock;
                    UDPsend(PORT_PORT, updatePos, sockfd);
                    string transactMsg = UDPrecv(sockfd);
                    send(client_sockfd, transactMsg.c_str(), transactMsg.size(), 0);
                    cout << "[Server M] Forwarded the buy result to client." << endl;
                    UDPsend(QUOT_PORT, updatePos, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                }
                else if(position_shares != "NA" && position_stockName == stockName)
                {
                    double avgBuy = avgBuyPriceNew(stoi(shares), stod(quote_price), stoi(position_shares), stod(position_price)) ;
                    int newShares = stoi(shares) + stoi(position_shares);
                    // update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    string updatePos = "update;" + position_userID + ";" + position_stockName + ";" + to_string(newShares) + ";" + to_string(avgBuy) + ";" + position_index + ";" + position_clientSock;
                    UDPsend(PORT_PORT, updatePos, sockfd);
                    string transactMsg = UDPrecv(sockfd); // "buy;confirm;" + user + ";" + targetStock + ";" + to_string(quantity);
                    // forward transactMsg to client (might need to update client)
                    send(client_sockfd, transactMsg.c_str(), transactMsg.size(), 0);
                    cout << "[Server M] Forwarded the buy result to client." << endl;
                    UDPsend(QUOT_PORT, updatePos, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                }
            }
        }
        else if (subOp == "sell")
        {
            cout << "[Server M] Received a sell request from member " << userID << " using TCP over port " << PORT << endl;
            string checkMsg = "check;" + userID + ";" + stockName + ";" + shares + ";" + clientSock; // check;<user>;<stock_name>;<quantity>;<socket_Num>
            //cout << "check message: " << checkMsg << endl;
            string requestMsg = "retrieve;" + userID + ";" + stockName + ";" + clientSock;
            UDPsend(PORT_PORT, checkMsg, sockfd); 
            cout << "[Server M] Forwarded the sell request to server P." << endl;
            string checkResponse = UDPrecv(sockfd); // check;user;stock;pass/fail;socketNum
            string resStatus;
            istringstream responseVal(checkResponse);
            for(int i = 0; i < 4; i++)
            {
                getline(responseVal, resStatus, ';'); // PASS or FAIL
            }
            // user;stock;quantity;price;indexInVector;socketNum    Valid
            // user;targetStock;NA;NA;-1;socketNum                  User does not own stock currently
            // user;NA;NA;NA;-1;socketNum;                          User Not Found
            UDPsend(PORT_PORT, requestMsg, sockfd);
            string clientPos = UDPrecv(sockfd);
            // parse message to extract information about the user's position
            string position_userID, position_stockName, position_shares, position_index, position_price, position_clientSock;
            istringstream clientInfo(clientPos);
            getline(clientInfo, position_userID, ';');
            getline(clientInfo, position_stockName, ';');
            getline(clientInfo, position_shares, ';');
            getline(clientInfo, position_price, ';');
            getline(clientInfo, position_index, ';');
            getline(clientInfo, position_clientSock, ';');

            string quote_MsgType, quote_stockName, quote_price, quote_clientSock;
            // quote;S1;697.46;5
            string modQuote = quoteMsg;
            istringstream quotStream(modQuote);
            getline(quotStream, quote_MsgType, ';');
            getline(quotStream, quote_stockName, ';');
            getline(quotStream, quote_price, ';');
            getline(quotStream, quote_clientSock, ';');
            if(position_shares == "NA" && quote_price == "NA")
            {
                string TCPReturnMsg = "stock_FAIL";
                sleep(1);
                send(client_sockfd, TCPReturnMsg.c_str(), TCPReturnMsg.size(), 0);
                // Sell failed not enough shares
                // string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                // UDPsend(QUOT_PORT, updateQuote, sockfd);
                return;
            }
            else if(position_shares == "NA" && quote_price != "NA")
            {
                string TCPReturnMsg = "FAIL";
                //sleep(1);
                send(client_sockfd, TCPReturnMsg.c_str(), TCPReturnMsg.size(), 0);
                // Sell failed not enough shares
                string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(QUOT_PORT, updateQuote, sockfd);
                cout << "[Server M] Sent a time forward request for " << stockName << endl;
                return;
            }
            else if(resStatus == "FAIL")
            {
                //sleep(1);
                send(client_sockfd, resStatus.c_str(), resStatus.size(), 0);
                // Sell failed not enough shares
                string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(QUOT_PORT, updateQuote, sockfd);
                cout << "[Server M] Sent a time forward request for " << stockName << endl;
                return;
            }
            else
            {
                string TCPReturnMsg = "PASS";
                //sleep(1);
                send(client_sockfd, TCPReturnMsg.c_str(), TCPReturnMsg.size(), 0);
                //send(client_sockfd, quoteMsg.c_str(), quoteMsg.size(), 0);
                sleep(1);
                send(client_sockfd, quoteMsg.c_str(), quoteMsg.size(), 0); // Need to sleep to allow first send to finish
                cout << "[Server M] Forwarded the sell confirmation to the client." << endl;
                string conMsg = TCPrecv(client_sockfd);
                // cout << "ServerM from Client Confirmation Message: " << conMsg << endl;
                istringstream conStream(conMsg);
                getline(conStream, confirmation, ';');
                
                if (confirmation == "N")
                {
                    string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                    string forwardConf = confirmation + ";sell";
                    UDPsend(PORT_PORT, forwardConf, sockfd);
                    cout << "[Server M] Forwarded the sell confirmation response to Server P." << endl;
                    UDPsend(QUOT_PORT, updateQuote, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                    return;
                }
                else 
                {
                    string forwardConf = confirmation + ";sell";
                    UDPsend(PORT_PORT, forwardConf, sockfd);
                    cout << "[Server M] Forwarded the sell confirmation response to Server P." << endl;
                    int newShares = stoi(position_shares) - stoi(shares);
                    // update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    string updatePos = "update;" + position_userID + ";" + position_stockName + ";" + to_string(newShares) + ";" + position_price + ";" + position_index + ";" + position_clientSock;
                    UDPsend(PORT_PORT, updatePos, sockfd);
                    sleep(1);
                    string transactMsg = UDPrecv(sockfd); // "sell;confirm/deny;" + user + ";" + targetStock + ";" + to_string(quantity);
                    //cout << "Receiving update from serverP: " << transactMsg << endl;
                    send(client_sockfd, transactMsg.c_str(), transactMsg.size(), 0);
                    cout << "[Server M] Forwarded the sell result to client." << endl;
                    UDPsend(QUOT_PORT, updatePos, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                }
            }
        }
        
    }
    if(choice == "quote")
    {
        string user, name;
        getline(stream, user, ';');
        getline(stream, name, ';');
        if(name == "all")
        {
            cout << "[Server M] Received a quote request from " << user << ", using TCP over port " << PORT << endl;
        }
        else
        {
            cout << "[Server M] Received a quote request from " << user << " for stock "<< name << ", using TCP over port " << PORT << endl;
        }
        msg = receivedMsg;
        UDPsend(QUOT_PORT, msg, sockfd);
        cout << "[Server M] Forwarded the quote request to server Q." << endl;
        string msgResponse = UDPrecv(sockfd);
        if(name == "all")
        {
            cout << "[Server M] Received a quote response from server Q using UDP over " << UDP_PORT << endl;
        }
        else
        {
            cout << "[Server M] Received a quote request from server Q for stock "<< name << ", using UDP over " << UDP_PORT << endl;
        }
        

        send(client_sockfd, msgResponse.c_str(), msgResponse.size(), 0);
    }
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
                        operationType(recMessage, udp_sockfd, sender_fd); // new version
                    }
                } // END handle data from client
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
}