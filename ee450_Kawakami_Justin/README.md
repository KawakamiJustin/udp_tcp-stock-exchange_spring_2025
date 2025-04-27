# ee450 project
a) Name: Justin Kawakami
b) Student ID: 1284352110
c) In this assignment, I have created a stock market server that allows at least two clients to connect to it using TCP. The server, server M, connects to 3 other servers, servers A, Q, and P. server M serves as a middleman that forwards to other servers depending on the message received. The client serves as the primary interface for the user to input commands and log into the stock system. Server A upon starting it, reads in a txt file that contains the encrypted passwords and unencrypted usernames of all valid users. It then validates any user sent to it from server M. Once logged in, the client can then request to view quotes, buy or sell stocks, and view their positions. Using UDP, the main server, server M communicates between the other servers to retrieve up to date stock information and current positions of the logged in client. Only the client can initialize commands to buy, sell, request quotes, and request positions.
d)  client.cpp  - This file opens a connection to serverM and then has the user login. The login is validated by the main server                     (through the authentication server).Once the user logs in successfully, they can request to view a quote, view                     all quotes, buy a stock, sell a stock, view the positions held, and exit and close the socket. Every command                       besides the exit command communicates with the main server using TCP. Any input requiring user feedback from                       other servers is sent back to the client before being sent back to the main server. The socket assigned to this                    client is dynamically assigned by the linux machine's OS.
                 
    serverM.cpp - This server acts as the primary and only communication between the client and the other servers as well as the                 only point of communication between servers. Unlike the client, the server can communicate with both TCP and UDP               and has two static port numbers (45110 TCP and 44110 UDP). Using a poll() server, the server can communicate                   with an N number of clients and because it's poll, it will mul;tiplex and allow each client to interact with the               server. When a client perforams any action, the operationType will determine what the next steps are, whether                  it's encrypting a password before forwarding to server A or checking if a sell is valid with server P and                      requesting confirmation of whether a user wants to sell their stock before updating other servers. For most data               processing, it is done in the main server. Profit, encryption, and average buy price are all doen within this                  server then sent to the necessary recipients. While the processing is done in this server, it does not actually                store anything on it as it immediately forwards any processed data for either storage or display.
    
    serverA.cpp - This server takes in a txt file, which contains the encrypted passwords and corresponding usernames to ensure a                user can log into the system. This server uses UDP and sends and receives using port 41110. Only the main server               can send encrypted passwords and unencrypted usernames to this server. The server does not send the encrypted                  password anywhere, rather it serves as just a confirmation that the user's credentials match the database. When                the server receives a username and password, it sends the main server a SUCCESS or FAILURE messge to validate                  whether a user's credentials can be used to login. To efficiently return the username and password combos, the                 txt file is converted to a map, with usernames as keys and encrypted passwords as values.
    
    serverQ.cpp - This server takes in a txt file, which contains a stock name followed by 10 prices. This server also uses UDP                  and sends and receives from its port, 43110. The server has three primary functions, it can return all quotes, a               specific quotes, and it keeps a database of every stock and their current prices. The server takes in the txt                  and constructs a map with the stock name  as a key, and structs as the values. The sturct alloows each stock to                have a vector of prices to change between, and an index of the current price it's using. The index allows the                  server to keep individuallzied positions of the price per stock rather than having to track each one as part of                global variables. When a server requests a quote, this server returns a message containing the current price of                the stock and the stock name itself. If all quotes are requested, it returns a string with a start and end flag                to tell the receiver where the start of the quotes are and where the last one is. This feature allows                          scalability for any amount of quotes as long as the receiver can support a buffer size that is greater than or                 equal to the string size with the quotes.
    
    serverP.cpp - This server takes in a txt file containing the positions held by every user in the database. This server also                  uses UDP and sends and receives from its port, 42110. This server has four primary functions for the users, to                 buy, sell, update positions and return positions held. It keeps an updated map, containing the usernames as keys,              and a vector of structs as the value. The structs contain the stock's name, how much the user owns, and the                    average buy price of the stock. The vector ensures the user can continue to add new positions on top of their                  current positions and remove them until they run out of positions to sell. Once the txt file is converted to the               string and vector of structs map, the client can then request to buy and sell, which will change the shares in                 the struct that's requested and the average buy price if the user is buying shares. The server only updates a                  user's portfolio with their permission and it will ensure a user has enough shares to sell before it will allow                the user to sell. It will also return all the positions held by a user to determine their profit in the main                   server.

e) All messages exchanged are concatenated and delimited by semicolons (;) depending on how many parameters are needed

Initial messages sent by client:
logging in              -   credentials;<username>;<unencrypted_password>
quote                   -   quote;all
quote <stock>           -   quote;<stock>;<username>
buy <stock> <quantity>  -   transact;buy;<username>;<stock>;<quantity>
sell <stock> <quantity> -   transact;sell;<username>;<stock>;<quantity>
position                -   position;user

Messages exchanged by operation after initial client message (not in any specific order and some operations, like quote and quote <stock>, are reused for other operations like buy, sell, and position):
logging in              -   credentials;<username>;<encrypted_password>;<client_assigned_child_socket>
                            credentials;<username>;<FAILURE/SUCCESS>;<client_assigned_child_socket>
quote                   -   quote;all;<client_assigned_child_socket>
                            quote;start;<stock_1>;<stock_1_price>;...;<stock_N>;<stock_N_price>;end;<client_assigned_child_socket>
quote <stock>           -   quote;<stock>;<username>;<client_assigned_child_socket>
                            quote;<stock>;<stockprice>;<client_assigned_child_socket>
buy <stock> <quantity>  -   transact;buy;<username>;<stock>;<quantity>;<client_assigned_child_socket>
                            <Y/N>;<username>;<quantity>
                            retrieve;<username>;<stock>;<client_assigned_child_socket>
                            <username>;<stock/NA>;<quantity/NA>;<stockprice/NA>;<stock_index_in_user_vector/-1>;<client_assigned_child_socket>
                            <Y/N>;<buy>
                            update;<username>;<stock>;<client_assigned_child_socket>
                            update;<username>;<stock>;<new_share_quantity>;<price>;<stock_index_in_user_vector/-1>;<client_assigned_child_socket>
                            buy;confirm;<username>;<stock>;<quantity>
sell <stock> <quantity> -   transact;sell;<username>;<stock>;<quantity>;<client_assigned_child_socket>
                            check;<username>;<stock>;<quantity>;<client_assigned_child_socket>
                            check;<username>;<stock>;<PASS/FAIL>;<client_assigned_child_socket>
                            <PASS/FAIL/stock_FAIL>
                            retrieve;<username>;<stock>;<client_assigned_child_socket>
                            <username>;<stock/NA>;<quantity/NA>;<stockprice/NA>;<stock_index_in_user_vector/-1>;<client_assigned_child_socket>
                            <Y/N>;<username>;<quantity>
                            <Y/N>;<sell>
                            update;<username>;<stock>;<client_assigned_child_socket>
                            update;<username>;<stock>;<new_share_quantity>;<price>;<stock_index_in_user_vector/-1>;<client_assigned_child_socket>
                            sell;confirm;<username>;<stock>;<quantity>
position                -   position;user;<client_assigned_child_socket>
                            position;<username>;<stock_1/NA>;<stock_1_price>;<stock_N_quantity>;...;<stock_N>;<stock_N_quantity>;<stock_N_price>;end;<client_assigned_child_socket>
                            position;<username>;<stock_1>;<stock_1_price>;<stock_N_quantity>;...;<stock_N>;<stock_N_quantity>;<stock_N_price>;end;<client_assigned_child_socket>;<profit>
                            position;<username>;position_FAIL;


f) Position, buy, and sell break if a non integer (including double, float, and string that is not a whole number) is used as a quantity in buy or sell. It prints out an error if the user neglects to enter the shares or stock but breaks if the quantity is not a whole number. When printing position, the grouping is by how it's read in the map, so if there's stocks labeled as S2 and S20, S20 may be printed before S3. The way the map prints it out is based on how it's spelled and not numerically or the original order.
g) I reused code for the udp talker and listener in all servers. server M and client also referenced code for TCP client and poll server in Beej. I reused the receiving code for nbytes from Beej throughout the project when receiving from servers and converting to a buffer.
h) Ubuntu Version: Ubuntu 16.04.3 LTS