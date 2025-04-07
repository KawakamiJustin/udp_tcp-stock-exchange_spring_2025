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

#define MYPORT "43110"
#define PORTM "44110"
#define MAXBUFLEN 512
using namespace std;

struct priceData
{
    vector<double> prices;
    int priceIndex;
};

map<string, priceData> onStartUp()
{
    ifstream infile;
    infile.open("quotes.txt");
    if (!infile.is_open()) {
        cerr << "Server A failed to open quotes.txt" << endl;
    }
    map<string, priceData> stocks; // creates new map
    string info;
    vector<double> prices;
    while(getline(infile, info))
    {
        prices.clear();
        int space = info.find(' '); 
        string stockName = info.substr(0, space);
        info = info.substr(space + 1);
        string price;
        while(!info.empty())
        {   
            space = info.find(' ');
            
            if (space == string::npos)
            {
                price = info;
                info.clear();
            }
            else
            {
                price = info.substr(0, space);
                info = info.substr(space + 1);
            }
            prices.push_back(stod(price));
        }
        stocks[stockName] = {prices, 0}; // populates map
    }

    //string test = loginInfo.find("Mary");
    infile.close();

    return stocks;
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
	return "";
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
    cout << "[Server Q] Booting up using UDP on port 41110 " << endl;
    string newMsg = "";
    map<string, priceData> quotes = onStartUp();
    string stockName;
    int index;
    vector<double> prices;
    
    while(stockName != "quit")
    {
        prices.clear();
        cout << "enter stock name: ";
        cin >> stockName;
        cout << endl;
        if(quotes.find(stockName) != quotes.end())
        {
            prices = quotes[stockName].prices;
            index = quotes[stockName].priceIndex;
            double curprice = prices[index];
            cout << stockName << " current price: $" << curprice;
            if(index == prices.size() - 1)
            {
                quotes[stockName].priceIndex = 0;
            }
            else{
                quotes[stockName].priceIndex++;
            }
            cout << endl;
        }
    }
    /*
    int sockfd = startServer();
    while(true)
    {
        newMsg = listen_pkts(sockfd, quotes);
        cout << newMsg << endl;
        udpSendMsg(newMsg, sockfd);
    }
    close(sockfd);
    */
    return 0;
}
