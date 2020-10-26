# SO-Project
•  What 

A simple chat room with server and client. By default, all the user are in a global chat. Is possible to not receive the global message using a special command. Is also possible to send a private message to a logged user. 
These are the commands available: 

/HELP Show all the special commands available in the chat 

/MOD Switch between global mode and private only  

/USERS Show all the only line client /QUIT Close the client 

@user_name prova Will send a private message to the client named user_name with text prova. 

•  How 

CLIENT

The client will ask if the user is new or not and the user name and password. Then will check if the inputs are theoretically correct like length max or empty field. If this phase is correct will create a connection to the server to check if the credential are correct in the server 
BD
. If all is successful two thread will manage the sending and receiving of messages.

SERVER

The server has a struct for the user.
 When the server is launched will set the socket option bind the address and start listen. The Server will accept a new connection and create a new thread. The thread will check the login credential in the DB. The DB is a simple file and the access is synchronized with a mutex. Is this operation is successfully the server will receive and send the message back to the other client. All the online users are in an array of size 20. If a new user will join the server is added to the array, when he left he will be removed. The operation on this array are synchronised with a mutex. 
When a user leaves all resource will be freed. There are two function to send message back. The global one will check the sender id and send the message to the online user with different id and global_chat 1 The other one will send the message only to the client with desired name

•  How to run.

Use the make command to compile the server and client.
If you want to run it locally launch first the server by typing ./server and then the client ./client server_ip.
Be sure that the ip and port are correct. If you want to connect to an already active server launch only the client


