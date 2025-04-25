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

f) Position, buy, and sell break if a non integer (including double, float, and string that is not a whole number) is used as a quantity in buy or sell. It prints out an error if the user neglects to enter the shares or stock but breaks if the quantity is not a whole number.
g) I reused code for the udp talker and listener in all servers. server M and client also referenced code for TCP client and poll server in Beej. I reused the receiving code for nbytes from Beej throughout the project when receiving from servers and converting to a buffer.
h) Ubuntu Version: Ubuntu 16.04.3 LTS