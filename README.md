# ee450 project
a) Name: Justin Kawakami
b) Student ID: 1284352110
c) In this assignment, I have created a stock market server that allows at least two clients to connect to it using TCP. The server, server M, connects to 3 other servers, servers A, Q, and P. server M serves as a middleman that forwards to other servers depending on the message received. The client serves as the primary interface for the user to input commands and log into the stock system. Server A upon starting it, reads in a txt file that contains the encrypted passwords and unencrypted usernames of all valid users. It then validates any user sent to it from server M. Once logged in, the client can then request to view quotes, buy or sell stocks, and view their positions. Using UDP, the main server, server M communicates between the other servers to retrieve up to date stock information and current positions of the logged in client. Only the client can initialize commands to buy, sell, request quotes, and request positions.
d)  client.cpp - 
    serverM.cpp - 
    serverA.cpp - 
    serverQ.cpp - 
    serverP.cpp - 
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


f) Position, buy, and sell break if a non integer (including double, float, and string that is not a whole number) is used as a quantity in buy or sell. It prints out an error if the user neglects to enter the shares or stock but breaks if the quantity is not a whole number. When printing position, the grouping is by how it's read in the map, so if htere's stocks labeled as S2 and S20, S20 may be before S9. The way the map prints it out is based on how it's spelled and not numerically or the original order.
g) I reused code for the udp talker and listener in all servers. server M and client also referenced code for TCP client and poll server in Beej. I reused the receiving code for nbytes from Beej throughout the project when receiving from servers and converting to a buffer.
h) Ubuntu Version: Ubuntu 16.04.3 LTS