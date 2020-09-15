#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define DEBUG 0

int socket_desc;
int quit;

void handle_error(char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void newline_remove(char *msg_in){
    int msg_in_len = strlen(msg_in);
   
    int i;
    for(i = 0; i < msg_in_len; i++){
        if(msg_in[i] == '\n'){
            msg_in[i] =  '\0';
            break;
        }
    }
}

void login(char* name){
    //login  part1
    printf("Enter your username: \n");
    if(fgets(name,32,stdin) != (char*) name){
        
        fprintf(stderr, "Can not read from stdin\n");
        exit(EXIT_FAILURE);
        
    }
    newline_remove(name);
    int name_len = strlen(name);
    if(name_len == 0){
        fprintf(stderr, "Name field can not be empty\n");
        exit(EXIT_FAILURE);
    }
   
}

void send_message_client(){
    
    if(DEBUG) fprintf(stderr, "We are in send_message\n");

    int ret;
    char buf[2048];
    size_t buf_len = sizeof(buf);
         
    memset(buf, 0, buf_len);
    while(1){
        if(fgets(buf, sizeof(buf), stdin) != (char*)buf){
            fprintf(stderr, "Can not read from stdin");
            exit(EXIT_FAILURE);
        }

        if(DEBUG) printf("The message we are about to send is: %s \n", buf);

        ret = send(socket_desc, buf,strlen(buf), 0);
        if(ret == -1){
            handle_error("Can not send message");
        }

        if(DEBUG){
            fprintf(stderr, "Message sent correctly\n");
        }
    }

}

void recv_message_client(){
    char buf_out[2048];
    if(DEBUG) fprintf(stderr, "We are in recv_message\n");
    int ret;
    int recv_bytes;
    //size_t buf_len = sizeof(buf_out);
    
    while(1){
        ret = recv(socket_desc, buf_out, 2048,0); 
        
        if(ret > 0){
            printf("RES: %s", buf_out);
        }

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
        //memset(buf_out, 0, buf_len);
    }
}

int main(int argc, char* argv[]){
    int ret;
    //provvisional address
    int port = 5000;
    char *ad = "127.0.0.1";
    char name[32];
    quit = 0;

    login(name);

    //TODO password

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

    ret = send(socket_desc, name, 32, 0);
    if(ret == -1){
        handle_error("send name");
    }

    //TO DO  send & recive
    pthread_t send_thread;
    pthread_t recv_thread;

    ret = pthread_create(&send_thread, NULL, (void*)send_message_client, NULL);
    ret = pthread_create(&recv_thread, NULL, (void*)recv_message_client, NULL);

    if(DEBUG) printf("Thread creato\n");

    while (1){
		if(quit){
			break;
        }
        sleep(10);
        if(DEBUG) printf("Loop\n");
	}
    
    ret = close(socket_desc);
    if(ret == -1){
        handle_error("Can not close socket");
    }

    if(DEBUG) printf("EXIT\n");

    exit(EXIT_SUCCESS);
}
