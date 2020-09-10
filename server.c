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
#define MAX_USER 20

int logged_users = 0;
int uid = 0;

//TODO Inserire struc user
typedef struct{
    char name[32];
    int uid;
    int sokcet_desc;
    struct sockaddr_in* client_addr;
}user_t;

user_t *user_list[MAX_USER];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_error(char* msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void add_queque(user_t *user){
    pthread_mutex_lock(&mutex);
    int i;
    for(i = 0; i<MAX_USER; i++){
        if(!user_list[i]){
            user_list[i] = user;
            break;
        }
    }
    
    pthread_mutex_unlock(&mutex);
}

void remove_queque(int uid){
    pthread_mutex_lock(&mutex);
    int i;
    for(i = 0; i<MAX_USER; i++){
        if(user_list[i]->uid == uid){
            user_list[i] = NULL;
            break;
        }

    }
    pthread_mutex_unlock(&mutex);

}

void send_message(char* msg, int sokcet_desc){

    int res;

    if(DEBUG) printf("The message we are about to send is: %s \n", msg);
    
    res = write(sokcet_desc, msg,strlen(msg));
    
    if(res == -1){
        handle_error("Ca not send message");
    }

    if(DEBUG) fprintf(stderr, "Message sent successfully by the server\n");
}


void *connection_handler(void *arg){
    
    if(DEBUG) fprintf(stderr, "We are in connection handler\n");
    
    int ret;
    char buf[2048];
    size_t bud_len = sizeof(buf);

    char name[32];
    logged_users++;

    user_t *user = (user_t*)arg;

    ret = recv(user->sokcet_desc, name, 32 ,0);
    if(ret == -1){
        handle_error("Can not recive user name");
    }
    if(ret){
        if(strlen(name) == 0){
            fprintf(stderr, "Name field can not be empty");
            exit(EXIT_FAILURE);
        }
        else{
            strcpy(user->name, name);
        }
    }

    if(DEBUG) printf("Name: %s\n", name);

    while(1){
        //read message from client
        int bytes_read = 0;
        do{
            ret = recv(user->sokcet_desc, buf + bytes_read, 1, 0);
            if(ret == -1 && errno == EINTR){
                continue;
            }
            if(ret == -1){
                handle_error("Can nopt read from the socket");
            }
            if(ret == 0){
                break;
            }
        }while(buf[bytes_read++] != '\n');
        
        if(DEBUG) printf("We recive %d bytes\n", bytes_read);
        if(DEBUG) printf("The message we recived is: %s \n", buf);
        
        send_message(buf, user->sokcet_desc);
        
    }

    //cclose ocket and remove user
    ret = close(user->sokcet_desc);
    if(ret == -1){
        handle_error("Can not close socket");
    }
    remove_queque(user->uid);
    free(user);
    logged_users--;

    pthread_detach(pthread_self());
}

int main(int argc, char* argv[]){
    int ret;
    int socket_desc, client_desc;
    int port = 5000;

    pthread_t thread; 

    printf("---Starting the server---\n");

    struct sockaddr_in server_addr = {0};
    struct sockaddr_in client_addr = {0};

    int sockaddr_len = sizeof(struct sockaddr_in);

    //create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1){
        handle_error("Can not create sokcet");
    }
    //setting
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port  = htons(port); 

    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt,sizeof(reuseaddr_opt));
    if(ret){
        handle_error("Can not setsockopt");
    }

    //bind address
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret){
        handle_error("Can not bind");
    }

    //listen on socket
    ret = listen(socket_desc, 10);      //need change 
    if(ret){
        handle_error("Can not listen");
    }

    printf("--SERVER UP--\n");

    while(1){

        struct sockaddr_in* client_addr =  calloc(1, sizeof(struct sockaddr_in));

        //accept connection
        client_desc = accept(socket_desc, (struct sockaddr*) &client_addr,(socklen_t*) &sockaddr_len);
        if(client_desc == -1){
            handle_error("Can not open socket to accept connection");
        }

        uid++;

        //thrad arg
        user_t *user_args = malloc(sizeof(user_t));
        user_args->uid = uid;
        user_args->sokcet_desc = client_desc;
        user_args->client_addr = client_addr;
        add_queque(user_args);
        ret = pthread_create(&thread, NULL, connection_handler, (void*)user_args);
        if(ret){
            handle_error("Can not create a new thread");
        }

        fprintf(stderr, "Thread creato con successo\n");

        

    }
    return EXIT_SUCCESS;

}