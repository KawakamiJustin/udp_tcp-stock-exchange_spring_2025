#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sstream>
#include <vector>
#include <limits>

using namespace std;

#include <arpa/inet.h>

#define PORT 45110 //the port client will be connecting to
#define MAXBUFLEN 512
//#define MAXDATASIZE 100

// get sockaddr,IPv4orIPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Created initial socket
int createSocket()
{
    int sockfd;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("client socket creation error");
        exit(1);
    }
    return sockfd;
}

// Server to connect client to
struct sockaddr_in defineServer()
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // clear memory
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    return server_addr;
}

void connectServer(int sockfd, struct sockaddr_in *server_addr)
{
    if(connect(sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1)
    {
        perror("client could not connect");
        exit(1);
    }
}

int getLocalPort(int TCP_Connect_sock)
{
    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);
    int getsock_check = getsockname(TCP_Connect_sock, (struct sockaddr*)&my_addr, (socklen_t*)&addrlen);

    //Error checking
    if(getsock_check == -1)
    {
        perror("getsockname");
        exit(1);
    }

    int localPort = ntohs(my_addr.sin_port);
    return localPort;
}

string recvString(int M_SOCK)
{
    char buf[MAXBUFLEN];
    memset(buf, 0, sizeof(buf)); // clear buffer
    int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0);
    if(nbytes > 0)
    {
        buf[nbytes] = '\0';
        string recMessage(buf);
        return recMessage;
    }
    else if (nbytes == 0)
    {
        return "connection closed";
    }
    else
    {
        return "Error receiving";
    }
}

string login(int sockfd)
{
    string username = "";
    string password = "";
    cout << "Please enter the username: ";
    cin >> username;
    while(username.length() < 1 || username.length() > 50)
    {
        cout << "Please enter a valid username\n";
        cout << "Please enter the username: \t";
        cin >> username;
    }
    cout << "Please enter the password: ";
    cin >> password;
    while(password.length() < 1 || password.length() > 50)
    {
        cout << "Please enter a valid password\n";
        cout << "Please enter the password: ";
        cin >> password;
    }
    string user_id = string("credentials;") + username +";"+ password;
    send(sockfd ,user_id.c_str(), user_id.size(), 0);
    return username;
}

string parseAUTH(string receivedMsg)
{
    istringstream stream(receivedMsg);
    string parsedMsg, usersname, status;
    string newMsg = "";
    int parseNum = 0;
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 2)
        {
            usersname = parsedMsg;
        }
        if(parseNum == 3)
        {
            status = parsedMsg;
        }
    }
    cout << "[Client] Received username " << usersname << " and status: " << status << endl;
    return status;
}

void parseAllQuotes(int sockfd)
{
    string receivedMsg = recvString(sockfd);
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, startFlag;
    string newMsg = "";
    int parseNum = 0;
    int localPort = getLocalPort(sockfd);
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << ".\n" << endl;
    while(getline(stream, parsedMsg, ';'))
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
            cout << parsedMsg << "\t";
        }
        else
        {
            cout << parsedMsg << endl;
            cout << endl;
        }
    }
}

void parseQuote(int sockfd)
{
    string receivedMsg = recvString(sockfd);
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, stockName, stockPrice;
    string newMsg = "";
    int parseNum = 0;
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        if(parseNum == 2)
        {
            stockName = parsedMsg;
        }
        if(parseNum == 3)
        {
            stockPrice = parsedMsg;
        }
    }
    int localPort = getLocalPort(sockfd);
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << ".\n" << endl;
    if(stockPrice == "NA")
    {
        cout << stockName << " does not exist. Please try again."<< endl;
    }
    else
    {
        cout << stockName << "\t" << stockPrice << endl;
        cout << endl;
    }
    
}

string parseTransact(int sockfd, string operation)
{
    string receivedMsg = recvString(sockfd);
    cout << "Received Quote from serverM: " << receivedMsg << endl;
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, stockName, stockPrice;
    string newMsg = "";
    int parseNum = 0;
    string response;
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        if(parseNum == 2)
        {
            stockName = parsedMsg;
        }
        if(parseNum == 3)
        {
            stockPrice = parsedMsg;
        }
    }
    int localPort = getLocalPort(sockfd);
    //cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << ".\n" << endl;
    if(stockPrice == "NA")
    {
        cout << "[Client] Error: stock names does not exist. Please check again."<< endl;
        response = "N";
    }
    else
    {
        cout << "[Client] " << stockName << "'s current price is " << stockPrice << ". Proceed to " << operation << "? (Y/N)" << endl;
        cin.clear();
        getline(cin, response);
        if(response != "Y" && response != "N")
        {
            cout << "[Client] You have entered an invalid character." << endl;
            cout << "[Client] " << stockName << "'s current price is " << stockPrice << ". Proceed to " << operation << "? (Y/N)" << endl;
            cin.clear();
            getline(cin, response);
        }
    }
    return response;
    
}

void parsePortfolio(int sockfd)
{
    string receivedMsg = recvString(sockfd);
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, startFlag;
    string newMsg = "";
    int parseNum = 0;
    int localPort = getLocalPort(sockfd);
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << ".\n" << endl;
    while(getline(stream, parsedMsg, ';'))
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
            cout << parsedMsg << "\t";
        }
        else
        {
            cout << parsedMsg << endl;
            cout << endl;
        }
    }
}

void quote(int sockfd)
{
    string command = string("quote;all");
    cout << "[Client] Sent a quote request to the main server"<< endl;
    send(sockfd ,command.c_str(), command.size(), 0);
    parseAllQuotes(sockfd);
}

void quote(int sockfd, string name)
{
    string command = string("quote;") + name;
    cout << "[Client] Sent a quote request to the main server"<< endl;
    send(sockfd ,command.c_str(), command.size(), 0);
    parseQuote(sockfd);
}

void transact(int sockfd, string operation, string stock, string quantity, string user)
{
    string command = "transact;" + operation + ";" + user + ";" + stock + ";" + quantity;
    send(sockfd ,command.c_str(), command.size(), 0);
    if(operation == "sell")
    {
        string sellValid = recvString(sockfd);
        if(sellValid == "FAIL")
        {
            cout << "[Client] Error: " << user << " does not have enough shares of " << stock <<".\n ---Start a new request---\n" << endl;
            return;
        }
        else if (sellValid == "stock_FAIL")
        {
            cout << "[Client] Error: stock names does not exist. Please check again."<< endl;
            return;
        }
    }
    //cout << "After sell: " <<endl;
    string response = parseTransact(sockfd, operation);
    if (response == "Y")
    {
        command = response + string(";") + user + ";" + quantity;
    }
    else
    {
        command = response + string(";") + user + ";" + quantity;
    }
    send(sockfd ,command.c_str(), command.size(), 0);
}

void getPortfolio(int sockfd, string user)
{
    string command = string("position;") + user;
    send(sockfd ,command.c_str(), command.size(), 0);
    parsePortfolio(sockfd);
}

int main()
{
    char buf[MAXBUFLEN];    // Buffer for client data

    cout << "[Client] Booting up.\n[Client] Logging in.\n";
    int M_SOCK = createSocket();
    struct sockaddr_in addr = defineServer();
    connectServer(M_SOCK, &addr);
    string status = "FAILURE";
    string user = login(M_SOCK);
    int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0);
    if(nbytes > 0)
    {
        buf[nbytes] = '\0';
        cout << "received: "<< buf << endl;
        string recMessage(buf);
        status = parseAUTH(recMessage);
    }
    while(status == "FAILURE")
    {
        //send(M_SOCK ,user_id.c_str(), user_id.size(), 0);
        cout << "[Client] The credentials are incorrect. Please try again." <<endl;
        user = login(M_SOCK);
        int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0);
        if(nbytes > 0)
        {
            buf[nbytes] = '\0';
            cout << "received: "<< buf << endl;
            string recMessage(buf);
            status = parseAUTH(recMessage);
        }
    }
    string choice = "";
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clears cin from previous entries
    if(status == "SUCCESS")
    {
        cout << "[Client] You have been granted access. \n";
    }
    while(status == "SUCCESS")
    {
        cout << "[Client] Please enter the command:" << endl;
        cout << ">\tquote \n>\tquote <stock name>\n>\tbuy <stock name> <number of shares>\n>\tsell <stock name> <number of shares>\n>\tposition\n>\texit" << endl;
        vector<string> selectedOptions;
        string word = "";
        
        cout<< "Enter your command here: ";
        getline(cin, choice);
        istringstream parsedChoices(choice);
        selectedOptions.clear();
        while (parsedChoices >> word)
        {
            selectedOptions.push_back(word);
        }
        if(selectedOptions.empty())
        {
            cout << "No command entered. Please try again." << endl;
            continue;
        }
        
        if (selectedOptions[0] == "quote")
        {
            if(selectedOptions.size() == 2)
            {
                quote(M_SOCK, selectedOptions[1]);
            }
            else if (selectedOptions.size() == 1)
            {
                quote(M_SOCK);
            }
            else
            {
                cout << "Invalid use of quote" << endl;
            }
        }
        else if (selectedOptions[0] == "buy" || selectedOptions[0] == "sell")
        {
            if(selectedOptions.size() == 3)
            {
                transact(M_SOCK, selectedOptions[0], selectedOptions[1], selectedOptions[2], user);
            }
            else if (selectedOptions[0] == "buy" && selectedOptions.size() < 3)
            {
                cout << "[Client] Error: stock name/shares are required. Please specify a stock name to buy." << endl;
            }
            else
            {
                cout << "[Client] Error: stock name/shares are required. Please specify a stock name to sell." << endl;
            }
        }
        else if (selectedOptions[0] == "position" && selectedOptions.size() == 1)
        {
            getPortfolio(M_SOCK, user);
        }
        else if (selectedOptions[0] == "exit")
        {
            close(M_SOCK);
            break;
        }
        else
        {
            cout << "Invalid option selected. Please try again." << endl;
        }
    }
    /*
    cout << "What port am I?" << endl;
    int localPort = getLocalPort(M_SOCK);
    cout << localPort << endl;
    */
    

    return 0;
}
