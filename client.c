#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define DEBUG 1

int socket_desc;

void handle_error(char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}



void send_message_client(){
    
    if(DEBUG) fprintf(stderr, "We are in send_message\n");

    int ret;
    char buf[2048];
    size_t buf_len = sizeof(buf);
    int msg_len;
    memset(buf, 0, buf_len);

        
    if(fgets(buf, sizeof(buf), stdin) != (char*)buf){
        fprintf(stderr, "[ERROR] Can not read from stdin");
        exit(EXIT_FAILURE);
    }

    if(DEBUG) printf("The message we are about to send is: %s \n", buf);

    ret = write(socket_desc, buf,strlen(buf));
    if(ret == -1){
        handle_error("Can not send message");
    }

    if(DEBUG){
        fprintf(stderr, "Message sent correctly\n");
    }

}

void recv_message_client(){
    char buf_out[2048];
    if(DEBUG) fprintf(stderr, "We are in recv_message\n");
    int ret;
    int recv_bytes;
    size_t buf_len = sizeof(buf_out);
    do{
       ret = recv(socket_desc, buf_out + recv_bytes, buf_len - recv_bytes ,0); 
       if(ret == -1 && errno == EINTR){
            continue;
            }
        if(ret == -1){
           handle_error("Can not recv message");
        }
        if(ret == 0){
           break; 
        }
        
        recv_bytes += ret;
        if(DEBUG) printf("We recive %d bytes\n", recv_bytes);
    }while(buf_out[recv_bytes-1] != '\n');
    
    printf("RES: %s\n", buf_out);

}

int main(int argc, char* argv[]){
    int ret;
    char buf[2048];
    //provvisional address
    int port = 5000;
    char *ad = "127.0.0.1";

    //variables for handling a socket
    
    struct sockaddr_in server_addr = {0};

    //create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1){
        handle_error("Can not create socket");
    }

    //socket setting
    server_addr.sin_addr.s_addr = inet_addr(ad);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    //connetion
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret){
        handle_error("Can not create a connection");
    } 

    fprintf(stderr, "Connesione Creata\n");

    //TO DO  send & recive
    
    while(1){
        if(DEBUG) fprintf(stderr, "Start send message\n");
       
        send_message_client();
        
        if(DEBUG) fprintf(stderr, "Start recive message\n");
        recv_message_client();

        
    }
    
    exit(EXIT_SUCCESS);
}
