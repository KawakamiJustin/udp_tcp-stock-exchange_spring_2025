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

#define PORT "45110"        // TCP port
#define UDP_PORT "44110"    // UDP port
#define AUTH_PORT "41110"   // Authentication Server UDP port
#define PORT_PORT "42110"   // Portfolio Server UDP port
#define QUOT_PORT "43110"   // Quote Server UDP port
#define MAXBUFLEN 4096       // Max buffer size when receiving

using namespace std;

// Taken from pollserver.c in Beej 7.2 poll() synchronous I/O multiplexing
// Get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Taken from pollserver.c in Beej 7.2 poll() synchronous I/O multiplexing
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
        //fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
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

// Taken from pollserver.c in Beej 7.2 poll() synchronous I/O multiplexing
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

// Taken from pollserver.c in Beej 7.2 poll() synchronous I/O multiplexing
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

// Encryts the password from the client
string passwordEncrypt(string receivedMsg)
{
    istringstream stream(receivedMsg); // converst string to stream to parse the string
    string parsedMsg, MsgType, usersname, password, socketNum;
    string newMsg = "";
    int parseNum = 0; // counter to determine which value is currently extracted from the stream

    // parses stream based on semicolon delimiters
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1) // extracts "credentials"
        {
            MsgType = parsedMsg;
        }
        if(parseNum == 2) // extracts username
        {
            usersname = parsedMsg;
        }
        if(parseNum == 3) // extracts unencrypted password
        {
            password = parsedMsg;
        }
        if(parseNum == 4) // extracts the appended client socket (assigned by serverM poll)
        {
            socketNum = parsedMsg;
        }
    }
    cout << "[Server M] Received username " << usersname << " and password ****" << endl;
    string encryptedPassword = ""; // empty string that gets appended to when converting characters
    int shift = 3; // sets encryption shift

    // alphabet and numbers to reference when shifting
    string uAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string lAlphabet = "abcdefghijklmnopqrstuvwxyz";
    string numbers = "0123456789";

    // shift password characters by 3
    for (char c : password)
    {
        // Shift uppercase
        if(isupper(c))
        {
            int index = uAlphabet.find(c);
            int newIndex = (index + shift) % uAlphabet.length();
            encryptedPassword += uAlphabet[newIndex];
        }
        // Shift lowercase
        else if(islower(c))
        {
            int index = lAlphabet.find(c);
            int newIndex = (index + shift) % lAlphabet.length();
            encryptedPassword += lAlphabet[newIndex];
        }
        // Shift digits
        else if(isdigit(c))
        {
            int index = numbers.find(c);
            int newIndex = (index + shift) % numbers.length();
            encryptedPassword += numbers[newIndex];
        }
        // Leave unknown characters alone
        else
        {
            encryptedPassword += c;
        }
    }
    // New message to forward to server A
    newMsg = MsgType + ";" + usersname + ";" + encryptedPassword + ";" + socketNum;
    return newMsg;
}

// Partially inspired (converted to function instead of main) from Beej listener.c (datagram sockets example)
// Starts udp listener socket to listen for incoming transmissions
int startUDPServer()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	int rv;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;// UDP datagram
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, UDP_PORT, &hints, &servinfo)) != 0) {
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			//perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			//perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		//fprintf(stderr, "listener: failed to bind socket\n");
	}

	freeaddrinfo(servinfo);

	cout << "[Server M] Booting up using UDP on port " << UDP_PORT << endl;
    return sockfd;
}

// Partially inspired (converted to function instead of main) from Beej listener.c (datagram sockets example)
// Listens for incoming transmissions to socket and converts incoming char buffer to a string
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
		//inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),	s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	//printf("listener: packet contains \"%s\"\n", buf);
    buf[numbytes] = '\0';
    string rawData(buf);
    return rawData;
}

// Partially inspired (converted to function instead of main) from Beej talker.c (datagram sockets example)
// Sends udp message to designated server (A, Q, and P)
// Takes in string as input and the socket file description of server A
void UDPsend(string serverPort, string message, int mysockfd)
{
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof hints); // Clears buffer
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP datagram

	getaddrinfo("localhost", serverPort.c_str(), &hints, &servinfo);

    sendto(mysockfd, message.c_str(), message.length(), 0, servinfo->ai_addr, servinfo->ai_addrlen);

	freeaddrinfo(servinfo);
}

// struct that defines the stock object
// includes stock name, the quantity in integers, and buy price represented as a double
struct stock
{
    string stockName;
	int quantity;
	double avgPrice;
};

// calculates the profit in a user's profile
// takes in the vector of stocks in the user profile consisting of stock objects
// indexes through the vector and finds the corresponding quote in the map and subtracts 
// the buy price from the current price before multiplying by the number of shares
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
    }
    return profit;
}

// Converts the messages received from serverQ and serverP containing all the quotes 
//  listed on the market and all the positions for a user to a map and vector
// Both the vector and map are used to calculate profit
// The vector containing the user positions is converted to a string to be sent to the 
//  client with the newly calculated profit attached
string constructNewMsg(string userPort, string stockQuotes)
{
    map<string, double> quotes; // map to contain all listed quotes
    vector<stock> userPosition; // vector to contain entirety of user portfolio
    double stockPrice;
    int shareQuantity;
    string stockName, userName, sockNum;
    stock newStock;

    // Example message formats received from server Q
    // Server Q (stockQuotes): quote;start;S1;697.46;S2;464.61;end;
    istringstream stockString(stockQuotes); // string converted to stream to parse quotes received by server Q
    string parsedMsg, msgType, startFlag;
    int parseNum = 0;
    // Parsing values from stream
    while(getline(stockString, parsedMsg, ';'))
    {
        parseNum++;
        // Message Type (should be "quote")
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        // Start flag to determine first stock quote in message
        else if(parseNum == 2)
        {
            startFlag = parsedMsg;
        }
        // Ends loop if end flag found for last quote
        else if (parsedMsg == "end")
        {
            break;
        }
        // Odd indices in message represent stock names
        else if (parseNum % 2 == 1)
        {
            stockName = parsedMsg;
        }
        // Even indicies represent the assigned price to each stock
        else
        {
            stockPrice = stod(parsedMsg); // convert the price to a double to allow usage in calculations
            quotes[stockName] = stockPrice; // assign price to stock as a value to a key in map
        }
    }

    istringstream userString(userPort);
    parseNum = 0;

    // Example message formats received from server P
    // Server P (userPort): position;James;S1;400;353.47;S2;178;500.00;5
    // Parsing values from stream
    while(getline(userString, parsedMsg, ';'))
    {
        parseNum++;
        // Message Type (should be "position)
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        // Parse the user whose portfolio is being processed and presented
        else if(parseNum == 2)
        {
            userName = parsedMsg;
        }
        // Ends message parsing if user has no positions in portfolio
        else if (parsedMsg == "NA")
        {
            string clientMsg = msgType + ";" + userName + ";position_FAIL;";
            break;
        }
        // Breaks loop if last buy price has be parsed out based on wher "end" flag is
        else if (parsedMsg == "end")
        {
            break;
        }
        // parses stock name based on if index is divisable by 3
        else if (parseNum % 3 == 0)
        {
            stockName = parsedMsg;
        }
        // parses stock share quantity held based on if index has remainder 1 if divided by 3
        else if (parseNum % 3 == 1)
        {
            shareQuantity = stoi(parsedMsg);
        }
        // parses stock price based on if index has remainder 2 if divided by 3
        // Creates new stock object with other parsed attributes and appends it to vector
        else if (parseNum % 3 == 2)
        {
            stockPrice = stod(parsedMsg);
            newStock.stockName = stockName;
            newStock.avgPrice = stockPrice;
            newStock.quantity = shareQuantity;
            userPosition.push_back(newStock);
        }
    }
    // passes vector and map for profit calculations
    // sets formatting of prices to 100ths place and returns message to send to client
    double totalProfit = profitCalc(quotes, userPosition);
    ostringstream concatProfit;
    concatProfit << fixed << setprecision(2) << totalProfit;
    string clientMsg = userPort + ";" + concatProfit.str();
    return clientMsg;
}

// returns the assigned child socket number for the client if given message with socket appended at end
int sockNum(string message)
{
    int socketpos = message.find_last_of(';') + 1;
    string socketString = message.substr(socketpos);
    return stoi(socketString);
}

// Partially inspired (extracted function from main) from Beej client.c "6.2 A Simple Stream Client"
// Receives from specified client child socket then converts char buffer to a string
string TCPrecv(int client_sockfd)
{
    char buf[MAXBUFLEN]; // Max data size to receive
    memset(buf, 0, sizeof(buf)); // clear buffer
    int nbytes = recv(client_sockfd, buf, sizeof buf, 0);
    buf[nbytes] = '\0';
    string rawData(buf);
    return rawData;
}

// Function to return avg buy price given the number of shares purchased and price per share
double avgBuyPrice(int quantity, double price)
{
    double avgPrice = (quantity * price)/quantity;
    return avgPrice;
}

// Function to return new avg buy price given the old quantity of shares and old average price and the new quaantity and price per share
double avgBuyPriceNew(int quantity, double price, int quantityOld, double avgPriceOld)
{
    double oldCost = quantityOld * avgBuyPrice(quantityOld, avgPriceOld);
    double newCost = quantity * avgBuyPrice(quantity, price);
    double avgPrice = (oldCost + newCost)/(quantity + quantityOld);
    return avgPrice;
}

// Parses incoming message from client to determine next steps
void operationType(string receivedMsg, int sockfd, int client_sockfd)
{
    string msg;
    istringstream stream(receivedMsg);
    string choice, subOp, userID, stockName, shares, price, clientSock, confirmation;
    getline(stream, choice, ';');

    // Steps for authentication
    if(choice == "credentials")
    {
        // encrypt received password then send to server A
        msg = passwordEncrypt(receivedMsg);
        UDPsend(AUTH_PORT, msg, sockfd);
        cout << "[Server M] Sent the authetication request to Server A" << endl;

        // Receive response from server A then convert to string and forward to client
        msg = UDPrecv(sockfd);
        cout << "[Server M] Received the response from server A using UDP over " << UDP_PORT << endl;
        send(client_sockfd, msg.c_str(), msg.size(), 0);
        cout << "[Server M] Sent the response from server A to the client using TCP over port " << PORT << endl;
    }

    // Steps for when a client wants their positions
    if(choice == "position")
    {
        // Message received from client + appended server assigned socket number: position;<user>;<socket_Num>
        // parse out the username
        msg = receivedMsg;
        getline(stream, userID, ';');

        // send position request to server P
        cout << "[Server M] Received a position request from Member to check " << userID << "'s gain using TCP over port " << PORT << "." << endl;
        UDPsend(PORT_PORT, msg, sockfd);
        cout << "[Server M] Forwarded the position request to server P." << endl;

        // receive the response from server P
        string userPort = UDPrecv(sockfd);
        cout << "[Server M] Received user's portfolio from server P using UDP over " << UDP_PORT << endl;

        // send a request to server Q for all current prices of all quotes
        UDPsend(QUOT_PORT, "quote;all", sockfd);
        string stockQuotes = UDPrecv(sockfd);

        // Construct a new message to forward to the client containing the positions of the user and the total profit
        string clientPos = constructNewMsg(userPort, stockQuotes);
        cout << "[Server M] Forwarded the gain to the client." << endl;
        send(client_sockfd, clientPos.c_str(), clientPos.size(), 0);
    }

    // Steps for when a client wants to buy or sell shares
    if(choice == "transact")
    {
        // Message received from client: transact;<buy/sell>;<user>;<stock_name>;<number_of_shares_requested>;<socket_Num>
        // parse out the necessary information from the message
        getline(stream, subOp, ';');
        getline(stream, userID, ';');
        getline(stream, stockName, ';');
        getline(stream, shares, ';');
        getline(stream, clientSock, ';');

        // prints out type of transaction received
        if(subOp == "sell")
        {
            cout << "[Server M] Received a sell request from member " << userID << " using TCP over port " << PORT << endl;
        }
        else
        {
            cout << "[Server M] Received a buy request from member " << userID << " using TCP over port " << PORT << endl;
        }

        // construct message to retrieve quote for stock involed in transaction and request from server Q
        // message to send to server Q: retrieve;<user>;<stock_name>;<socket_Num>
        string Quote_msg = string("quote;") + stockName + string(";") + clientSock;
        UDPsend(QUOT_PORT, Quote_msg, sockfd);
        cout << "[Server M] Sent quote request to server Q." << endl;

        // receive all quotes from server Q
        string quoteMsg = UDPrecv(sockfd);
        cout << "[Server M] Received quote response from server Q." << endl;
        
	    // buy operation
        if(subOp == "buy")
        {
            // Forward the quote to the client for them to confirm or deny the purchase
            send(client_sockfd, quoteMsg.c_str(), quoteMsg.size(), 0);
            cout << "[Server M] Sent the buy confirmation to the client." << endl;

            // Receive buy response from the client (Y/N)
            string conMsg = TCPrecv(client_sockfd);
            istringstream conStream(conMsg);
            getline(conStream, confirmation, ';');

            // If buy is denied, update the price of the quote to next index
            if (confirmation == "N")
            {
                cout << "[Server M] Buy denied." << endl;
                string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                string forwardConf = confirmation + ";buy";
                UDPsend(PORT_PORT, forwardConf, sockfd);
                cout << "[Server M] Forwarded the buy confirmation response to Server P." << endl;
                
                // Exit function once server Q has been notified to update
                // Message sent to server Q: update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                UDPsend(QUOT_PORT, updateQuote, sockfd);
                cout << "[Server M] Sent a time forward request for " << stockName << endl;
                return;
            }

            // If buy is approved
            else 
            {
                // Forward the confirmation to server P to display message
                cout << "[Server M] Buy approved." << endl;
                string forwardConf = confirmation + ";buy";
                UDPsend(PORT_PORT, forwardConf, sockfd);
                cout << "[Server M] Forwarded the buy confirmation response to Server P." << endl;
                string quote_MsgType, quote_stockName, quote_price, quote_clientSock;

                // parse original quote response for current stock information
                istringstream quotStream(quoteMsg);
                getline(quotStream, quote_MsgType, ';');
                getline(quotStream, quote_stockName, ';');
                getline(quotStream, quote_price, ';');
                getline(quotStream, quote_clientSock, ';');

                // Retrieve information about the requested stocck from the user's portfolio
                // Types of received messages
                // user;stock;quantity;price;indexInVector;socketNum    Valid
                // user;targetStock;NA;NA;-1;socketNum                  User does not own stock currently
                // user;NA;NA;NA;-1;socketNum;                          User Not Found
                string requestMsg = "retrieve;" + userID + ";" + stockName + ";" + clientSock;
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
                
                // If the stock exists and the user does not own any
                if(position_shares == "NA" && position_stockName == stockName)
                {
                    // Calculate average buy price
                    double avgBuy = avgBuyPrice(stoi(shares),stod(quote_price));
                    
                    // Update the user's position in server P
                    string updatePos = "update;" + position_userID + ";" + position_stockName + ";" + shares + ";" + to_string(avgBuy) + ";" + position_index + ";" + position_clientSock;
                    UDPsend(PORT_PORT, updatePos, sockfd);
                    string transactMsg = UDPrecv(sockfd);

                    // forward transaction message to client to notiify of successful transaction
                    send(client_sockfd, transactMsg.c_str(), transactMsg.size(), 0);
                    cout << "[Server M] Forwarded the buy result to client." << endl;

                    // Send notice to server Q to update stock price
                    // Message sent to server Q: update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    UDPsend(QUOT_PORT, updatePos, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                }

                // If the stock exists and the userr owns some shares already
                else if(position_shares != "NA" && position_stockName == stockName)
                {
                    // Calculate new quantity of shares and new average buy price
                    double avgBuy = avgBuyPriceNew(stoi(shares), stod(quote_price), stoi(position_shares), stod(position_price)) ;
                    int newShares = stoi(shares) + stoi(position_shares);
                    
                    // Update the user's position in server P
                    string updatePos = "update;" + position_userID + ";" + position_stockName + ";" + to_string(newShares) + ";" + to_string(avgBuy) + ";" + position_index + ";" + position_clientSock;
                    UDPsend(PORT_PORT, updatePos, sockfd);
                    string transactMsg = UDPrecv(sockfd);

                    // forward transaction message to client to notiify of successful transaction
                    send(client_sockfd, transactMsg.c_str(), transactMsg.size(), 0);
                    cout << "[Server M] Forwarded the buy result to client." << endl;

                    // Send notice to server Q to update stock price
                    // Message sent to server Q: update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    UDPsend(QUOT_PORT, updatePos, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                }
            }
        }

        // sell operation
        else if (subOp == "sell")
        {
            // Construct message to check if user quantity is a valid amount of shares to sell
            // Message to send to server P: check;<user>;<stock_name>;<quantity>;<socket_Num>
            cout << "[Server M] Received a sell request from member " << userID << " using TCP over port " << PORT << endl;
            string checkMsg = "check;" + userID + ";" + stockName + ";" + shares + ";" + clientSock; 

            // Construct message to get user's current holdings of stock
            // Message to send to server P: retrieve;<user>;<stock_name>;<socket_Num>
            string requestMsg = "retrieve;" + userID + ";" + stockName + ";" + clientSock;

            // send sell request to server P and check if it's a valid request
            UDPsend(PORT_PORT, checkMsg, sockfd); 
            cout << "[Server M] Forwarded the sell request to server P." << endl;

            // check if sell is a valid request
            // Message received from server P: check;user;stock;pass/fail;socketNum
            string checkResponse = UDPrecv(sockfd); 
            string resStatus;

            // parse out pass or fail from resposne from server P
            istringstream responseVal(checkResponse);
            for(int i = 0; i < 4; i++)
            {
                getline(responseVal, resStatus, ';'); // PASS or FAIL
            }

            // Retrieve information about the requested stocck from the user's portfolio
            // Types of received messages
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

            // parse message to extract information about the stock quote the user want to sell
            string quote_MsgType, quote_stockName, quote_price, quote_clientSock;
            string modQuote = quoteMsg;
            istringstream quotStream(modQuote);
            getline(quotStream, quote_MsgType, ';');
            getline(quotStream, quote_stockName, ';');
            getline(quotStream, quote_price, ';');
            getline(quotStream, quote_clientSock, ';');

            // sell failed because user doesn't own any shares and quote does not exist
            if(position_shares == "NA" && quote_price == "NA")
            {
                // notify client of failed transaction due to stock name not found
                string TCPReturnMsg = "stock_FAIL";
                send(client_sockfd, TCPReturnMsg.c_str(), TCPReturnMsg.size(), 0);
                return;
            }

            // sell failed because user doesn't own any shares of the stock (i.e. there is no record of the stock in the user porfolio)
            else if(position_shares == "NA" && quote_price != "NA")
            {
                // notify client of failed transaction due to insufficient number of shares
                string TCPReturnMsg = "FAIL";
                send(client_sockfd, TCPReturnMsg.c_str(), TCPReturnMsg.size(), 0);
                
                // Send notice to server Q to update stock price
                // Message sent to server Q: update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(QUOT_PORT, updateQuote, sockfd);
                cout << "[Server M] Sent a time forward request for " << stockName << endl;
                return;
            }

            // sell failed not enough shares (i.e. shares requested to sell exceed shares owned (>= 1))
            else if(resStatus == "FAIL")
            {
                // notify client of failed transaction due to insufficient number of shares
                send(client_sockfd, resStatus.c_str(), resStatus.size(), 0);

                // Send notice to server Q to update stock price
                // Message sent to server Q: update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;
                UDPsend(QUOT_PORT, updateQuote, sockfd);
                cout << "[Server M] Sent a time forward request for " << stockName << endl;
                return;
            }

            // sell is valid but need user's confirmation
            else
            {
                // notify the user the transaction is valid
                string TCPReturnMsg = "PASS";
                send(client_sockfd, TCPReturnMsg.c_str(), TCPReturnMsg.size(), 0);
                sleep(1); // Need to sleep to allow first send to finish

                // request the user to send confirmation of sell at given quote price
                send(client_sockfd, quoteMsg.c_str(), quoteMsg.size(), 0); 
                cout << "[Server M] Forwarded the sell confirmation to the client." << endl;

                // receive confirmation or denial of sell request
                string conMsg = TCPrecv(client_sockfd);
                istringstream conStream(conMsg);
                getline(conStream, confirmation, ';');
                
                // if client declines the sell request
                if (confirmation == "N")
                {
                    // Message sent to server Q: update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    string updateQuote = "update;" + userID + ";" + stockName + ";" + clientSock;

                    // notify server P of sell request denial
                    string forwardConf = confirmation + ";sell";
                    UDPsend(PORT_PORT, forwardConf, sockfd);
                    cout << "[Server M] Forwarded the sell confirmation response to Server P." << endl;

                    // Send notice to server Q to update stock price
                    UDPsend(QUOT_PORT, updateQuote, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                    return;
                }

                // if client confirms the sell request
                else 
                {
                    // notify server P of sell request approval
                    string forwardConf = confirmation + ";sell";
                    UDPsend(PORT_PORT, forwardConf, sockfd);
                    cout << "[Server M] Forwarded the sell confirmation response to Server P." << endl;
                    int newShares = stoi(position_shares) - stoi(shares); // update number of shares left in position
                    
                    // update server P with new number of shares left
                    // update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
                    string updatePos = "update;" + position_userID + ";" + position_stockName + ";" + to_string(newShares) + ";" + position_price + ";" + position_index + ";" + position_clientSock;
                    UDPsend(PORT_PORT, updatePos, sockfd);

                    // receive update from server P with number of shares sold
                    string transactMsg = UDPrecv(sockfd); // "sell;confirm/deny;user;targetStock;quantity

                    // notify client of sell transaction
                    send(client_sockfd, transactMsg.c_str(), transactMsg.size(), 0);
                    cout << "[Server M] Forwarded the sell result to client." << endl;

                    // Send notice to server Q to update stock price
                    UDPsend(QUOT_PORT, updatePos, sockfd);
                    cout << "[Server M] Sent a time forward request for " << stockName << endl;
                }
            }
        }
        
    }

    // steps for quote requests from client
    if(choice == "quote")
    {
        // parse quote request for quote type
        getline(stream, stockName, ';');
        getline(stream, userID, ';');
        getline(stream, clientSock, ';');

        // request all quotes in server Q
        if(stockName == "all")
        {
            cout << "[Server M] Received a quote request from " << userID << ", using TCP over port " << PORT << endl;
        }

        // request specific quote from server Q
        else
        {
            cout << "[Server M] Received a quote request from " << userID << " for stock "<< stockName << ", using TCP over port " << PORT << endl;
        }

        // send quote request to server Q
        msg = string("quote;") + stockName + string(";") + clientSock;
        UDPsend(QUOT_PORT, msg, sockfd);
        cout << "[Server M] Forwarded the quote request to server Q." << endl;

        // receive quote and verify what type
        string msgResponse = UDPrecv(sockfd);
        if(stockName == "all")
        {
            cout << "[Server M] Received a quote response from server Q using UDP over " << UDP_PORT << endl;
        }
        else
        {
            cout << "[Server M] Received a quote request from server Q for stock "<< stockName << ", using UDP over " << UDP_PORT << endl;
        }

        // forward the quote response to the client
        send(client_sockfd, msgResponse.c_str(), msgResponse.size(), 0);
        cout << "[Server M] Forwarded the quote response to the client." << endl;
    }
}

// Main (taken from Beej poll() Synchronous I/O Multiplexing)
// Modified to run OperationType instead of sending back received messages to all connected clients
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
        //fprintf(stderr, "error getting listening socket\n");
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
           // perror("poll");
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

                        //printf("pollserver: new connection from %s on ""socket %d\n",
                        //    inet_ntop(remoteaddr.ss_family,
                        //    get_in_addr((struct sockaddr*)&remoteaddr),
                        //    remoteIP, INET6_ADDRSTRLEN),
                        //    newfd);
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
                            //printf("pollserver: socket %d hung up\n", sender_fd);
                        } else {
                            //perror("recv");
                        }

                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);

                    } else {
                        buf[nbytes] = '\0';

                        // Convert the buffer of char to a string then append which socket it was from
                        string recMessage(buf);
                        recMessage += ";" + to_string(sender_fd);

                        // runs list of operations based on received message from client
                        operationType(recMessage, udp_sockfd, sender_fd); 
                    }
                } // END handle data from client
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
}
// Main code heavily referenced from Beej poll()