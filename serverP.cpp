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
			cout << "Parsing User: " << userID << endl;
			stocks.clear();
		}
		else if (!info.empty() && spaceCount >= 2)
		{
			int space = info.find(' ');
        	stockInfo.stockName = info.substr(0, space);
			cout << "Adding stock: " << stockInfo.stockName << " for user: " << userID << endl;

			info = info.substr(space + 1);
			space = info.find(' ');
			stockInfo.quantity = stoi(info.substr(0, space));
			cout << "Adding quantity: "<< stockInfo.quantity <<" for stock " << stockInfo.stockName << " for user: " << userID << endl;

        	info = info.substr(space + 1);
			//space = info.find(' ');
			stockInfo.avgPrice = stod(info);
			cout << "Adding avgprice: "<< stockInfo.avgPrice <<" for stock " << stockInfo.stockName << " for user: " << userID << endl;

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


string process_data(char *buf, int numbytes, map<string, vector<stock>> &portfolio)
{
    buf[numbytes] = '\0';
    string rawData(buf);
    cout << "raw data: " << rawData << endl;
    istringstream stream(rawData);
    string parsedMsg, MsgType, transactType, quantity, price,  socketNum;
    string newMsg = "";
    int parseNum = 0;
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            MsgType = parsedMsg;
        }
        if(parseNum == 2 && MsgType != "position")
        {
            whichQuote = parsedMsg;
        }
        if(parseNum == 3)
        {
            socketNum = parsedMsg;
        }
    }
    if(MsgType == "position")
    {
        //updatePrice(quotes, whichQuote);
        return "";
    }
    else if (MsgType == "quote")
    {
        //string msg = getQuote(quotes, whichQuote);
        newMsg = MsgType + ";"  + ";" + socketNum;
        return newMsg;
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
    string status = process_data(buf, numbytes, portfolio);
    return status;
}


int main(void)
{
	int sockfd = startServer();
	map<string, vector<stock>> portfolio = onStartUp();
	listen_pkts(sockfd, portfolio);

	/*for (const auto& pair : portfolio)
	{
		cout <<"User: " <<pair.first <<'\n';
		for (const auto& s : pair.second)
		{
			cout << "Stock Name: " << s.stockName
				<< " Stock Quantity: " << s.quantity
				<< " Stock Price: " << s.avgPrice << '\n';
		}
		if (pair.second.empty())
		{
			cout << " No stocks assigned to this user. \n";
		}
	}*/
	close(sockfd);
	return 0;
}