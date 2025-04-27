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
#include <vector>
#include <iomanip>

#define MYPORT "43110" // UDP Port Number for Server Q
#define PORTM "44110" // UDP Port Number for Server M
#define MAXBUFLEN 4096 // Max supported buffer size for incoming messages
using namespace std;

// struct for stock price updating, contains the list of prices and the index of the current price to use
struct priceData
{
    vector<double> prices;
    int priceIndex;
};

// converts quotes.txt to a map
map<string, priceData> onStartUp()
{
    ifstream infile;
    infile.open("quotes.txt");
    if (!infile.is_open()) {
        cerr << "Server Q failed to open quotes.txt" << endl;
    }
    map<string, priceData> stocks; // creates new map
    string info;
    vector<double> prices; // vector of prices

    // reads in all lines from the txt file
    while(getline(infile, info))
    {
        prices.clear(); // clear vector every line
        int space = info.find(' '); // find the first space index
        string stockName = info.substr(0, space); // return the stock name
        info = info.substr(space + 1); // start sttring at the next character after the first space
        string price;

        // copy all prices assigned to stock until end, denoted by next line
        while(!info.empty())
        {   
            space = info.find(' ');
            
            // end of line, no spaces left
            if (space == string::npos)
            {
                price = info;
                info.clear();
            }
            // keep adding prices and find next space
            else
            {
                price = info.substr(0, space);
                info = info.substr(space + 1);
            }
            
            // add the prices to the vector as a double
            prices.push_back(stod(price));
        }
        stocks[stockName] = {prices, 0}; // populates map
    }
    infile.close();
    return stocks;
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

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo("localhost", MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
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

	//printf("listener: waiting to recvfrom...\n");
    return sockfd;
}

// increments index of stock price for specified stock
void updatePrice(map<string, priceData> &quotes, string stockName)
{
    int index;
    vector<double> prices;
    prices.clear();

    // finds stock by name (redundant step)
    if(quotes.find(stockName) != quotes.end())
    {
        prices = quotes[stockName].prices;
        index = quotes[stockName].priceIndex;
        double curprice = prices[index];
        cout << "[Server Q] Received a time forward request for " << stockName << " the current price of that stock is " << curprice << " at time " << index << endl;

        // wrap around if at end of prices
        if(index == prices.size() - 1)
        {
            quotes[stockName].priceIndex = 0;
        }

        // increment index by one
        else
        {
            quotes[stockName].priceIndex++;
        }
    }
}

// return the quote for specified stock(s)
string getQuote(map<string, priceData> &quotes, string stockName)
{
    string quoteMsg = "";
    string quoteName = "";
    double quotePrice;
    priceData priceInfo;

    // server or user requests all available stocks
    if (stockName == "all")
    {
        cout << "[Server Q] Received a quote request from the main server." << endl;
        quoteMsg += "start;";

        // append the stock and current price for each stock to one string
        for (const auto &entry : quotes)
        {
            quoteName = entry.first;
            priceInfo = entry.second;
            quotePrice = priceInfo.prices[priceInfo.priceIndex];
            
            // keep the price in the string to 100ths place
            ostringstream stream;
            stream << fixed << setprecision(2) << quotePrice;
            quoteMsg += quoteName + ";" + stream.str() + ";";
        }
        quoteMsg += "end";
        cout << "[Server Q] Returned all stock quotes." << endl;
    }

    // server or user requests speccific stock
    else
    {
        cout << "[Server Q] Received a quote request from the main server for stock " << stockName << "." << endl;

        // try to find the stock and return the price
        if(quotes.find(stockName) != quotes.end())
        {
            priceInfo = quotes[stockName];
            quotePrice = priceInfo.prices[priceInfo.priceIndex];
            
            // keep the price in the string to 100ths place
            ostringstream stream;
            stream << fixed << setprecision(2) << quotePrice;
            quoteMsg = stockName + ";" + stream.str();
        }

        // stock not found
        else
        {
            quoteMsg = stockName + ";NA";
        }
        cout << "[Server Q] Returned the stock quote of " << stockName << "." << endl;
    }
    return quoteMsg;
}

// convert the data receied to a string then parse out the important information
string process_data(char *buf, int numbytes, map<string, priceData> &quotes)
{
    buf[numbytes] = '\0';
    string rawData(buf);
    istringstream stream(rawData);
    string parsedMsg, MsgType, whichQuote, socketNum;
    string newMsg = "";
    int parseNum = 0;

    // parse out the operation and dependencies
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            MsgType = parsedMsg;
        }

        // parse out the quote requested
        if(parseNum == 2 && MsgType != "update")
        {
            whichQuote = parsedMsg;
        }

        // parse out the socket number for quote type message
        if(parseNum == 3 && MsgType != "update")
        {
            socketNum = parsedMsg;
        }

        // parse out which stock price to update 
        else if (parseNum == 3 && MsgType == "update")
        {
            whichQuote = parsedMsg;
        }
    }

    // update next steps and returns string to ensure no response is sent back
    if(MsgType == "update")
    {
        updatePrice(quotes, whichQuote);
        return "DONE";
    }

    // return the requested quote back to the main function, which will send back to the server
    else if (MsgType == "quote")
    {
        string msg = getQuote(quotes, whichQuote);
        newMsg = MsgType + ";" + msg + ";" + socketNum;
        return newMsg;
    }
}

// Partially inspired (converted to function instead of main) from Beej listener.c (datagram sockets example)
// Listens for incoming transmissions to socket set up before and converts incoming char buffer to a string for post processing
string listen_pkts(int sockfd, map<string, priceData> &quotes)
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
	buf[numbytes] = '\0';
    string status = process_data(buf, numbytes, quotes);
    return status;
}

// Partially inspired (converted to function instead of main) from Beej talker.c (datagram sockets example)
// Sends udp message to main server
// Takes in string as input and the socket file description of server Q
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
    cout << "[Server Q] Booting up using UDP on port 43110 " << endl;
    string newMsg = "";
    map<string, priceData> quotes = onStartUp();
    int sockfd = startServer();
    string stockName;
    while(true)
    {
        newMsg = listen_pkts(sockfd, quotes);

        // sends a response if necessary
        if(newMsg != "DONE")
        {
            udpSendMsg(newMsg, sockfd);
        }
    }
    close(sockfd);
    return 0;
}