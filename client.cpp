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
#include <arpa/inet.h>
#include <sstream>
#include <vector>
#include <limits>

using namespace std;

#define PORT 45110 // the port client will be connecting to
#define MAXBUFLEN 4096 // Max size of buffer when accepting messages

// From Beej client.c (6.2 A Simple Stream Client)
// get sockaddr,IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Referenced from Beej (9.23 socket())
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

// Referenced from Beej (9.24 struct sockaddr and pals)
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

// Referenced from Beej (9.3 connect())
// connects client to a server defined by file descriptor
void connectServer(int sockfd, struct sockaddr_in *server_addr)
{
    if(connect(sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1)
    {
        perror("client could not connect");
        exit(1);
    }
}

// Referenced from Project Guidelines
// returns port useed by client
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

// returns string from a received buffer
string recvString(int M_SOCK)
{
    char buf[MAXBUFLEN];
    memset(buf, 0, sizeof(buf)); // clear buffer
    int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0); // receive from specified server file descriptor
    if(nbytes > 0)
    {
        buf[nbytes] = '\0';
        string recMessage(buf);
        return recMessage;
    }
}

// function for validating usernames and passwords 
string login(int sockfd)
{
    string username = "";
    string password = "";
    cout << "Please enter the username: ";
    getline(cin, username);

    // ensures the usernames is between 1 - 50 characters
    while(username.empty() || username.length() > 50 )
    {
        cout << "Please enter a valid username\n";
        cout << "Please enter the username: ";
        getline(cin, username);
    }
    cout << "Please enter the password: ";
    getline(cin, password);

    // ensures the password is between 1 - 50 characters
    while(password.empty() || password.length() > 50)
    {
        cout << "Please enter a valid password\n";
        cout << "Please enter the password: ";
        getline(cin, password);
    }

    // send username and password using TCP to server M to validate
    string user_id = string("credentials;") + username +";"+ password;
    send(sockfd, user_id.c_str(), user_id.size(), 0);

    // return username for future commands
    return username;
}

// read message received by server M to check if username and password combo are valid
string parseAUTH(string receivedMsg)
{
    istringstream stream(receivedMsg);
    string parsedMsg, usersname, status;
    string newMsg = "";
    int parseNum = 0;

    // parse message for username and login status (SUCCESS or FAILURE)
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

    // return SUCCESS or FAILURE
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
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << "." << endl;
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

// parse the quote message received from server M and display as required
void parseQuote(int sockfd)
{
    string receivedMsg = recvString(sockfd);
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, stockName, stockPrice;
    string newMsg = "";

    // parse the received message for quote information
    int parseNum = 0;
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        // returns the quote designation
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }
        // returns the stock name in the quote message
        if(parseNum == 2)
        {
            stockName = parsedMsg;
        }
        // returns the stock price in the quote message
        if(parseNum == 3)
        {
            stockPrice = parsedMsg;
        }
    }

    // displays the quote to the user
    int localPort = getLocalPort(sockfd);
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << "." << endl;
    if(stockPrice == "NA") // quote was not found
    {
        cout << stockName << " does not exist. Please try again."<< endl;
    }
    else // quote found for stock
    {
        cout << stockName << "\t" << stockPrice << endl;
        cout << endl;
    }
    
}

// parse the transact response
string parseTransact(int sockfd, string operation)
{
    string receivedMsg = recvString(sockfd);
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, stockName, stockPrice;
    string newMsg = "";
    int parseNum = 0;
    string response;

    // extract information for confirmation
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
    
    // User unable to purchase or sell stock that doesn't exist
    if(stockPrice == "NA")
    {
        cout << "[Client] Error: stock name does not exist. Please check again."<< endl;
        response = "N";
    }

    // user approves or denies transaction based on current price
    else
    {
        cout << "[Client] " << stockName << "'s current price is " << stockPrice << ". Proceed to " << operation << "? (Y/N)" << endl;
        cin.clear(); // without clear, getline reads previous \n
        getline(cin, response);

        // only accept Y or N as acceptable inputs
        if(response != "Y" && response != "N")
        {
            //cout << "[Client] You have entered an invalid character." << endl;
            cout << "[Client] " << stockName << "'s current price is " << stockPrice << ". Proceed to " << operation << "? (Y/N)" << endl;
            cin.clear(); // without clear, getline reads previous \n
            getline(cin, response);
        }
    }

    // return response to be sent to server M then P
    return response;   
}

// displays the user's positions
void parsePortfolio(int sockfd)
{
    string receivedMsg = recvString(sockfd);
    istringstream stream(receivedMsg);
    string parsedMsg, msgType, user, stockName, price, quantity, profit;
    string newMsg = "";
    int parseNum = 0;
    int localPort = getLocalPort(sockfd);
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << "." << endl;
    
    // parse and display position string received from server M
    // position;<user>;<stock_1>;<stock_1_quantity>;<stock_1_price>;...;<stock_N>;<stock_N_quantity>;<stock_N_price>;end;<serverM_assigned_socket_number>;<user_total_profit>
    while(getline(stream, parsedMsg, ';'))
    {
        parseNum++;
        // message type = position
        if(parseNum == 1)
        {
            msgType = parsedMsg;
        }

        // user for message displaying
        else if(parseNum == 2)
        {
            user = parsedMsg;
        }

        // user doesn't exist in portfolio.txt
        else if(parseNum == 3 && parsedMsg == "NA")
        {
            // cout << user << "does not own any stock." << endl;
            break;
        }
        // user doesn't own any shares of any stock
        else if (parsedMsg == "end" && parseNum == 3)
        {
            cout << "stock\tshares\tavg_buy_price" << endl;
            cout << "NA\tNA\t0.00" << endl;
            getline(stream, parsedMsg, ';'); // skips over the assigned server M socket number
            getline(stream, parsedMsg, ';'); // retrieves the profit appended at end of message
            profit = parsedMsg;
            cout << user <<"'s current profit is " << profit << "\n" << endl;
            break;
        }        
        // user owns shares of at least one stock and begins displaying portfolio
        else if(parseNum == 3 && parsedMsg != "NA" && parsedMsg != "end")
        {
            cout << "stock\tshares\tavg_buy_price" << endl;
            stockName = parsedMsg;
        }
        // prints the profit after getting end flag read in
        else if (parsedMsg == "end")
        {
            getline(stream, parsedMsg, ';'); // skips over the assigned server M socket number
            getline(stream, parsedMsg, ';'); // retrieves the profit appended at end of message
            profit = parsedMsg;
            cout << user <<"'s current profit is " << profit << "\n" << endl;
            break;
        }

        // parses out the stock name
        else if (parseNum % 3 == 0)
        {
            stockName = parsedMsg;
        }

        // parses out the share quantity
        else if (parseNum % 3 == 1)
        {
            quantity = parsedMsg;
        }

        // parse out share avg buy price then displays all information (comes as a set of three, stock, quantity, and price)
        else if (parseNum % 3 == 2)
        {
            price = parsedMsg;
            cout << stockName << "\t" << quantity << "\t" << price << endl;
        }
    }
}

// sets command to send the quote request for all quotes
void quote(int sockfd, string userID)
{
    string command = string("quote;all;") + userID; // request all quotes
    cout << "[Client] Sent a quote request to the main server"<< endl;
    send(sockfd ,command.c_str(), command.size(), 0); // send request to server M
    parseAllQuotes(sockfd); // parse and print the quotes received
}

// overloaded command to request a specific quote
void quote(int sockfd, string name, string userID)
{
    string command = string("quote;") + name + string(";") + userID; // request the specified quote
    cout << "[Client] Sent a quote request to the main server"<< endl;
    send(sockfd ,command.c_str(), command.size(), 0); // send request to server M
    parseQuote(sockfd); // parse and print quote
}

// construct message to buy or sell stocks then send to and receive from server M
void transact(int sockfd, string operation, string stock, string quantity, string user)
{
    // construct and send request to server M
    string command = "transact;" + operation + ";" + user + ";" + stock + ";" + quantity;
    send(sockfd ,command.c_str(), command.size(), 0);

    // sell command operations
    if(operation == "sell")
    {

        // ends sell request early if user does not own enough shares as requested to sell
        string sellValid = recvString(sockfd);
        if(sellValid == "FAIL")
        {
            cout << "[Client] Error: " << user << " does not have enough shares of " << stock <<".\n ---Start a new request---" << endl;
            return;
        }

        // ends sell request early if stock requested to sell does not exist
        else if (sellValid == "stock_FAIL")
        {
            cout << "[Client] Error: stock name does not exist. Please check again."<< endl;
            return;
        }
    }

    // user confirms or denys transaction then sends the response to the server
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
    
    // ends function early if user does not want to continue transaction
    if(response == "N")
    {
        return;
    }

    // receive buy and sell results from transaction then parse out information for display
    // received message format: buy/sell;confirm/deny;user;targetStock;quantity
    string statusUpdate = recvString(sockfd);
    istringstream transactResult(statusUpdate);
    string operationType, confirmation, userInfo, stockTransact, shareQuantity;
    getline(transactResult, operationType, ';');
    getline(transactResult, confirmation, ';');
    getline(transactResult, userInfo, ';');
    getline(transactResult, stockTransact, ';');
    getline(transactResult, shareQuantity, ';');

    // display user transaction and amounts 
    int localPort = getLocalPort(sockfd);
    cout << "[Client] Received the response from the main server using TCP over port "<< to_string(localPort) << "." << endl;
    if (operationType == "buy" && confirmation == "confirm")
    {
        cout << userInfo << " successfully bought " << shareQuantity <<" shares of " << stockTransact << ".\n ---Start a new request---" << endl;
    }
    else if (operationType == "sell" && confirmation == "confirm")
    {
        cout << userInfo << " successfully sold " << shareQuantity <<" shares of " << stockTransact << ".\n ---Start a new request---" << endl;
    }
}

// operation order after position request
void getPortfolio(int sockfd, string user)
{
    string command = string("position;") + user;
    send(sockfd ,command.c_str(), command.size(), 0);
    parsePortfolio(sockfd);
}


int main()
{
    char buf[MAXBUFLEN];    // Buffer for client data

    // initial display
    cout << "[Client] Booting up.\n[Client] Logging in.\n";

    // create file descriptor for server M and define server address
    int M_SOCK = createSocket();
    struct sockaddr_in addr = defineServer();

    // connect client to server
    connectServer(M_SOCK, &addr);

    // set initial login status
    string status = "FAILURE";

    // retrieve login status from server M
    string user = login(M_SOCK);
    int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0);
    if(nbytes > 0)
    {
        buf[nbytes] = '\0';
        string recMessage(buf);
        status = parseAUTH(recMessage);
    }

    // failed login
    while(status == "FAILURE")
    {
        cout << "[Client] The credentials are incorrect. Please try again." <<endl;
        user = login(M_SOCK);
        int nbytes = recv(M_SOCK, buf, sizeof(buf) - 1, 0);
        if(nbytes > 0)
        {
            buf[nbytes] = '\0';
            string recMessage(buf);
            status = parseAUTH(recMessage);
        }
    }

    // upon successful login
    string choice = "";
    cin.clear();
    if(status == "SUCCESS")
    {
        cout << "[Client] You have been granted access. \n";
    }
    
    // display commands to use and retrieve user input
    while(status == "SUCCESS")
    {
        cout << "[Client] Please enter the command:" << endl;
        cout << "\tquote \n\tquote <stock name>\n\tbuy <stock name> <number of shares>\n\tsell <stock name> <number of shares>\n\tposition\n\texit" << endl;
        vector<string> selectedOptions; // holds user commands delimited by spaces
        string word = "";
        
        // retrieves user input for command
        // cout<< "Enter your command here: ";
        getline(cin, choice);

        // parses user input for valid commands
        istringstream parsedChoices(choice);
        selectedOptions.clear();

        // adds parsed commands to vector
        while (parsedChoices >> word)
        {
            selectedOptions.push_back(word);
        }

        // user did not enter any command
        if(selectedOptions.empty())
        {
            // cout << "No command entered. Please try again." << endl;
            continue;
        }
        
        // user entered a quote command
        if (selectedOptions[0] == "quote")
        {
            if(selectedOptions.size() == 2)
            {
                quote(M_SOCK, selectedOptions[1], user);
            }
            else if (selectedOptions.size() == 1)
            {
                quote(M_SOCK, user);
            }
            else
            {
                cout << "Invalid use of quote" << endl;
            }
        }
        
        // user uses buy or sell command
        else if (selectedOptions[0] == "buy" || selectedOptions[0] == "sell")
        {
            if(selectedOptions.size() == 3)
            {
                transact(M_SOCK, selectedOptions[0], selectedOptions[1], selectedOptions[2], user);
            }

            // invalid buy or sell commands (syntax not position based)
            else if (selectedOptions[0] == "buy" && selectedOptions.size() < 3)
            {
                cout << "[Client] Error: stock name/shares are required. Please specify a stock name to buy." << endl;
            }
            else
            {
                cout << "[Client] Error: stock name/shares are required. Please specify a stock name to sell." << endl;
            }
        }

        // user enters position command and nothing else
        else if (selectedOptions[0] == "position" && selectedOptions.size() == 1)
        {
            getPortfolio(M_SOCK, user);
        }

        // user exits and closes the socket
        else if (selectedOptions[0] == "exit")
        {
            close(M_SOCK);
            break;
        }

        // other commands are entered which are not recognized
        else
        {
            //cout << "Invalid option selected. Please try again." << endl;
        }
    }
    return 0;
}
