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
#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

#define MYPORT "42110"	// the port users will be connecting to
#define PORTM "44110"

#define MAXBUFLEN 512
using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

struct stock
{
	string stockName;
	int quantity;
	double avgPrice;
};

map<string, vector<stock>> onStartUp()
{
    ifstream infile;
    infile.open("portfolios.txt");
    if (!infile.is_open()) {
        cerr << "Server P failed to open portfolios.txt" << endl;
    }
    map<string, vector<stock>> portfolio; // creates new map
    string info, userID;
    vector<stock> stocks;
	stock stockInfo;

    while(getline(infile, info))
    {
		int spaceCount = count(info.begin() , info.end(), ' ');
		if(!info.empty() && spaceCount < 2)
		{
			if(!userID.empty())
			{
				portfolio[userID] = stocks;
			}
			userID = info;
			//cout << "Parsing User: " << userID << endl;
			stocks.clear();
		}
		else if (!info.empty() && spaceCount >= 2)
		{
			int space = info.find(' ');
        	stockInfo.stockName = info.substr(0, space);
			//cout << "Adding stock: " << stockInfo.stockName << " for user: " << userID << endl;

			info = info.substr(space + 1);
			space = info.find(' ');
			stockInfo.quantity = stoi(info.substr(0, space));
			//cout << "Adding quantity: "<< stockInfo.quantity <<" for stock " << stockInfo.stockName << " for user: " << userID << endl;

        	info = info.substr(space + 1);
			//space = info.find(' ');
			stockInfo.avgPrice = stod(info);
			//cout << "Adding avgprice: "<< stockInfo.avgPrice <<" for stock " << stockInfo.stockName << " for user: " << userID << endl;

			stocks.push_back(stockInfo);
		}
        
    }

	if (!userID.empty())
	{
		portfolio[userID] = stocks;
	}

	return portfolio;
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

string getPortfolio(string user,map<string, vector<stock>> &portfolio)
{
	string totalPosition = "";
	string stockName, quant;
	double avgPrice;
	vector<stock> assignedStocks;
	if(portfolio.find(user) != portfolio.end())
	{
		assignedStocks.clear();
		assignedStocks = portfolio[user];
		for (const auto& stockInfo : assignedStocks)
		{
			stockName = stockInfo.stockName;
			quant = to_string(stockInfo.quantity);
			avgPrice = stockInfo.avgPrice;
			//cout << avgPrice << endl;;
			ostringstream stream;
            stream << fixed << setprecision(2) << avgPrice;
			//cout << stream.str();
			totalPosition += stockName + ";" + quant + ";" + stream.str() + ";";
			//cout << totalPosition << endl;
		}
	}
	else
	{
		totalPosition = "NA;";
	}
	return totalPosition;
}

string getStock(string user, string targetStock, string socketNum, map<string, vector<stock>> &portfolio)
{
	string stockPosition = "";
	string quant, price;
	vector<stock> assignedStocks;
	if(portfolio.find(user) != portfolio.end())
	{
		vector<stock> stocks = portfolio[user];
		for (int index = 0; index < stocks.size(); index++)
		{
			if(stocks[index].stockName == targetStock)
			{
				quant = to_string(stocks[index].quantity);
				price = stocks[index].avgPrice;
				ostringstream stream;
            	stream << fixed << setprecision(2) << price;
				stockPosition = user + ";" + targetStock + ";" + quant + ";" + stream.str() +";" + to_string(index) + ";" + socketNum;
				return stockPosition;
			}
		}
		if(stockPosition == "")
		{
			stockPosition = user + ";" + targetStock + "NA;NA;-1;" + socketNum; // User does not own stock currently
		}
	}
	else
	{
		stockPosition = user + ";NA;NA;NA;-1;" + socketNum; // User Not Found
	}
	return stockPosition; // user;stock;quantity;price;indexInVector;socketNum
}

void updateStock(string user, string targetStock, int quantity, int indexNum, double price, map<string, vector<stock>> &portfolio)
{
	if (indexNum == -1)
	{
		stock newStock;
		newStock.stockName = targetStock;
		newStock.quantity = quantity;
		newStock.avgPrice = price;
		portfolio[user].push_back(newStock);
	}
	else
	{
		portfolio[user][indexNum].quantity = quantity;
		portfolio[user][indexNum].avgPrice = price;
	}
}

string process_data(char *buf, int numbytes, map<string, vector<stock>> &portfolio)
{
    buf[numbytes] = '\0';
    string rawData(buf);
    cout << "raw data: " << rawData << endl;
    istringstream stream(rawData);
    string parsedMsg, MsgType, user, stockName, socketNum, rawMsg;
	int quantity, indexNum;
	double price;
    string newMsg = "";
    int parseNum = 0;
	// retrieve;<user>;<stock_name>;<socket_Num>
	// position;<user>;<socket_Num>
	// update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            MsgType = parsedMsg;
        }
		else if(parseNum == 2)
        {
            user = parsedMsg;
        }
        else if(parseNum == 3 && MsgType != "position")
        {
            stockName = parsedMsg;
        }
        else if(parseNum == 3 && MsgType == "position")
        {
            socketNum = parsedMsg;
        }
        else if(parseNum == 4 && MsgType == "retrieve")
        {
            socketNum = parsedMsg;
        }
		else if(MsgType == "update" && parseNum >= 4)
        {
			if(parseNum == 4)
			{
				quantity = stoi(parsedMsg);
			}
			else if(parseNum == 5)
			{
				price = stod(parsedMsg);
			}
			else if(parseNum == 6)
			{
				indexNum = stoi(parsedMsg);
			}
			else if(parseNum == 7)
			{
				socketNum = parsedMsg;
			}
        }
    }
    if(MsgType == "position")
    {
        rawMsg = getPortfolio(user, portfolio);
		newMsg = MsgType + ";" + user + ";" + rawMsg + socketNum;
        return newMsg;
    }
    else if (MsgType == "retrieve")
    {
        newMsg = getStock(user, stockName, socketNum, portfolio);
        return newMsg;
    }
	else if(MsgType == "update")
	{
		updateStock(user, stockName, quantity, indexNum, price, portfolio);
		return "update";
	}
}

string listen_pkts(int sockfd, map<string, vector<stock>> &portfolio)
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
		perror("recvfrom");
		exit(1);
	}

	printf("listener: got packet from %s\n",
		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),	s, sizeof s));
	printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);
    string messageToSend = process_data(buf, numbytes, portfolio);
    return messageToSend;
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

int main(void)
{
	cout << "[Server P] Booting up using UDP on port 42110 " << endl;
	int sockfd = startServer();
	map<string, vector<stock>> portfolio = onStartUp();
	while(true)
    {
        string udpResponse = listen_pkts(sockfd, portfolio);
		if(udpResponse != "update")
		{
			udpSendMsg(udpResponse, sockfd);
		}
    }
	close(sockfd);
	return 0;
}