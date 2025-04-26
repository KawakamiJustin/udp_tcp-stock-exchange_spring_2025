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

#define MYPORT "42110"	// the port server M will connect to
#define PORTM "44110" 	// server M UDP port

#define MAXBUFLEN 4096 // Max supported buffer size to receive
using namespace std;

// Taken from Beej listener.c (datagram sockets example)
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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

// struct to store values assigned to given stock
struct stock
{
	string stockName;
	int quantity;
	double avgPrice;
};

// converts portfolios.txt to a map with usernames as keys and a vector of stocks as values
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
	
	// read every line in txt file and parse out essential information
    while(getline(infile, info))
    {
		// parse out username (found by having 1 or 0 spaces in the line)
		int spaceCount = count(info.begin() , info.end(), ' ');
		if(!info.empty() && spaceCount < 2)
		{
			// if another username reached and the current username is populated, add the vector of stock postions to that user
			if(!userID.empty())
			{
				portfolio[userID] = stocks;
			}
			string userMix = info;
			userID = lowercaseConvert(userMix);
			stocks.clear();
		}

		// parse out the stock information
		// add the stock to a vector of other stocks
		else if (!info.empty() && spaceCount >= 2)
		{
			int space = info.find(' ');
        	stockInfo.stockName = info.substr(0, space);

			info = info.substr(space + 1);
			space = info.find(' ');
			stockInfo.quantity = stoi(info.substr(0, space));

        	info = info.substr(space + 1);
			stockInfo.avgPrice = stod(info);

			stocks.push_back(stockInfo);
		}
        
    }

	// update last user with vector of stocks (not done in while loop)
	if (!userID.empty())
	{
		portfolio[userID] = stocks;
	}
	return portfolio;
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

	if ((rv = getaddrinfo("localhost", MYPORT, &hints, &servinfo)) != 0) {
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

	//printf("listener: waiting to recvfrom...\n");
    return sockfd;
}

// returns the entirety of a user's portfolio
// searches for a user then concatenates their vector of stocks into a single string
string getPortfolio(string user,map<string, vector<stock>> &portfolio)
{
	string totalPosition = "";
	string stockName, quant;
	double avgPrice;
	vector<stock> assignedStocks;

	// finds the user in the map
	if(portfolio.find(user) != portfolio.end())
	{
		assignedStocks.clear();
		assignedStocks = portfolio[user];
		for (const auto& stockInfo : assignedStocks)
		{
			stockName = stockInfo.stockName;
			quant = to_string(stockInfo.quantity);
			avgPrice = stockInfo.avgPrice;
			ostringstream stream;
            stream << fixed << setprecision(2) << avgPrice;
			totalPosition += stockName + ";" + quant + ";" + stream.str() + ";";
		}
	}
	// user does not exist in database
	else
	{
		totalPosition = "NA;";
	}
	return totalPosition;
}

string getStock(string user, string targetStock, string socketNum, map<string, vector<stock>> &portfolio)
{
	string stockPosition = "";
	string quant;
	double price;
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
			stockPosition = user + ";" + targetStock + ";NA;NA;-1;" + socketNum; // User does not own stock currently
		}
	}
	else
	{
		stockPosition = user + ";NA;NA;NA;-1;" + socketNum; // User Not Found
	}
	return stockPosition; // user;stock;quantity;price;indexInVector;socketNum
}

string checkStock(string user, string targetStock, int quantity, string socketNum, map<string, vector<stock>> &portfolio)
{
	string checkSeq = "";
	int quant = 0;
	bool stockMatch = false;
	vector<stock> assignedStocks;
	if(portfolio.find(user) != portfolio.end())
	{
		vector<stock> stocks = portfolio[user];
		for (int index = 0; index < stocks.size(); index++)
		{
			if(stocks[index].stockName == targetStock)
			{
				quant = stocks[index].quantity;
				stockMatch = true;
			}
		}
	}
	if(quantity > quant && stockMatch)
	{
		checkSeq = "check;" + user + ";" + targetStock + ";FAIL;" + socketNum; 
		cout << "[Server P] Stock " << targetStock << " does not have enough shares in "<< user <<"'s portfolio. Unable to sell " << quantity << " shares of "  << targetStock << "." << endl;
	}
	else if (quantity <= quant && stockMatch)
	{
		checkSeq = "check;" + user + ";" + targetStock + ";PASS;" + socketNum;
	}
	else
	{
		checkSeq = "NOMATCH";
	}
	return checkSeq; // check;user;stock;pass/fail;socketNum
}

string updateStock(string user, string targetStock, int quantity, int indexNum, double price, map<string, vector<stock>> &portfolio)
{
	string tranasactConfirm;
	if (indexNum == -1)
	{
		stock newStock;
		newStock.stockName = targetStock;
		newStock.quantity = quantity;
		newStock.avgPrice = price;
		portfolio[user].push_back(newStock);
		cout << "[Server P] Successfully bought "<< to_string(quantity) << " shares of " << targetStock << " and updated "<< user << "'s portfolio." << endl;
		tranasactConfirm = "buy;confirm;" + user + ";" + targetStock + ";" + to_string(quantity);
	}
	else
	{
		if(quantity != 0)
		{
			if(portfolio[user][indexNum].quantity > quantity)
			{
				int dif = portfolio[user][indexNum].quantity - quantity;
				cout << "[Server P] Successfully sold "<< to_string(dif) << " shares of " << targetStock << " and updated "<< user << "'s portfolio." << endl;
				tranasactConfirm = "sell;confirm;" + user + ";" + targetStock + ";" + to_string(dif);
			}
			else
			{
				int dif =  quantity - portfolio[user][indexNum].quantity;
				cout << "[Server P] Successfully bought "<< to_string(dif) << " shares of " << targetStock << " and updated "<< user << "'s portfolio." << endl;
				tranasactConfirm = "buy;confirm;" + user + ";" + targetStock + ";" + to_string(dif);
			}
			portfolio[user][indexNum].quantity = quantity;
			portfolio[user][indexNum].avgPrice = price;
		}
		else if (portfolio[user][indexNum].quantity != 0 && quantity == 0)
		{
			int oldQuant = portfolio[user][indexNum].quantity;
			portfolio[user].erase(portfolio[user].begin() + indexNum);
			cout << "[Server P] Successfully sold "<< to_string(oldQuant) << " shares of " << targetStock << " and updated "<< user << "'s portfolio." << endl;
			tranasactConfirm = "sell;confirm;" + user + ";" + targetStock + ";" + to_string(oldQuant);
		}
	}
	return tranasactConfirm;
}

string process_data(char *buf, int numbytes, map<string, vector<stock>> &portfolio)
{
    buf[numbytes] = '\0';
    string rawData(buf);
	if(rawData == "N;sell")
	{
		cout << "[Server P] Sell denied." << endl;
		return "NOCHANGE";
	}
	else if(rawData == "Y;sell")
	{
		cout << "[Server P] User approves selling the stock." << endl;
		return "NOCHANGE";
	}
	else if(rawData == "Y;buy")
	{
		cout << "[Server P] Received a buy request from the client." << endl;
		return "NOCHANGE";
	}
    istringstream stream(rawData);
    string parsedMsg, MsgType, user, stockName, socketNum, rawMsg;
	int quantity, indexNum;
	double price;
    string newMsg = "";
    int parseNum = 0;
	// retrieve;<user>;<stock_name>;<socket_Num>
	// position;<user>;<socket_Num>
	// update;<user>;<stock_name>;<quantity>;<price>;<index_number>;<socket_Num>
	// check;<user>;<stock_name>;<quantity>;<socket_Num>
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            MsgType = parsedMsg;
        }
		else if(parseNum == 2)
        {
            user = lowercaseConvert(parsedMsg);
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
		else if(MsgType == "check" && parseNum >= 4)
		{
			if(parseNum == 4)
			{
				quantity = stoi(parsedMsg);
			}
			else if(parseNum == 5)
			{
				socketNum = parsedMsg;
			}
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
		cout << "[Server P] Received a position request from the main server for Member: "<< user << endl;
        rawMsg = getPortfolio(user, portfolio);
		newMsg = MsgType + ";" + user + ";" + rawMsg + "end;" + socketNum;
        return newMsg;
    }
	else if(MsgType == "check")
	{
		cout << "[Server P] Received a sell request from the main server." << endl;
		string checkMsg = checkStock(user, stockName, quantity, socketNum, portfolio);
		return checkMsg; // check;user;stock;pass/fail;socketNum
	}
    else if (MsgType == "retrieve")
    {
        newMsg = getStock(user, stockName, socketNum, portfolio);
        return newMsg;
    }
	else if(MsgType == "update")
	{
		string statusUpdate = updateStock(user, stockName, quantity, indexNum, price, portfolio);
		return statusUpdate;
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
		//perror("recvfrom");
		exit(1);
	}
	buf[numbytes] = '\0';
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
	string msgType;
	while(true)
    {
        string udpResponse = listen_pkts(sockfd, portfolio);
		if(udpResponse != "NOCHANGE")
		{
			udpSendMsg(udpResponse, sockfd);
			// cout << "Sending Msg: " << udpResponse << endl;
			istringstream positionReq(udpResponse);
			getline(positionReq, msgType, ';');
			if(msgType == "position")
			{
				getline(positionReq, msgType, ';');
				cout << "[Server P] Finished sending the gain and portfolio of " << msgType << endl;
			}
		}
    }
	close(sockfd);
	return 0;
}