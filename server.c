#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#define DEBUG 1
#define MAX_USER 20
#define BUFFER_SIZE 2048
#define QUIT_COMMAND "/QUIT\n"
#define HELP_COMMAND "/HELP\n"
#define LIST_COMMAND "/USERS\n"
#define CHAT_MOD_COMMAND "/MOD\n"

int logged_users = 0, success_conv = 0;
int uid = 0;
int login_fd;
int new_user_recv, quit_command_len;

//TODO opwn port
//TODO Inserire struc user
typedef struct{
    char name[32];
    int uid;
    int global_chat;
    int sokcet_desc;
    struct sockaddr_in* client_addr;
}user_t;

user_t *user_list[MAX_USER];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

void send_message(char* msg, int uid){

    int res;

    if(DEBUG) printf("The message we are about to send is: %s \n", msg);
    
    if(DEBUG) printf("Logged user: %d \n",logged_users);

    pthread_mutex_lock(&mutex);
    int i;
    for(i = 0; i < logged_users; i++){
        
        if(user_list[i]->uid != uid && user_list[i]->global_chat == 1){
            res = send(user_list[i]->sokcet_desc, msg,strlen(msg),0);

            if(res == -1 && errno == EINTR){
                continue;
            }
            if(res == -1){
                handle_error("Ca not send message");
            }
        }

    }
    
    pthread_mutex_unlock(&mutex);

    if(DEBUG) fprintf(stderr, "Message sent successfully by the server\n");
}

//TODO send message to only 1 client
void send_message_single(char* msg, int sok_dest){
    
    int res;

    if(DEBUG) printf("The message we are about to send is: %s \n", msg);

    res = send(sok_dest, msg, strlen(msg),0);

    if(res == -1){
        handle_error("Ca not send message");
    }

    if(DEBUG) fprintf(stderr, "Message sent successfully by the server\n");

}

//user list for debug
void print_user_list(){
    int i;
    
    printf("Current logged users with id:\n");
    for(i = 0; i < logged_users; i++){
        printf("UserName: %s ID: %d\n",user_list[i]->name, user_list[i]->uid);
    }
}

int login_handler(user_t *user){

    //TODO send message back to client

    int success = 0,name_len = 0;
    int ret;
    char name[64];
    char password[32];

    if(logged_users == MAX_USER){
        printf("Server cannot accept new connections. Maximum number of users reached\n");
        send_message_single("Server cannot accept new connections. Maximum number of users reached, please try again later.\n", user->sokcet_desc);
        return 0;
    }

    //Riceve il valore new_usser
    ret = recv(user->sokcet_desc,&new_user_recv,sizeof(new_user_recv),0);
    if(ret == -1){
        handle_error("can not rcv new user opt");
    }

    int new_user_opt = ntohl(new_user_recv);

    if(DEBUG) printf("new_user:opt %d\n", new_user_opt);

    ret = recv(user->sokcet_desc, name, 32 ,0);
    if(ret == -1){
        handle_error("Can not recive user name");
    }
    if(ret){
        newline_remove(name);
        int i;
        for(i=0;i < logged_users; i++){
            if(!strcmp(user_list[i]->name,name)){
                printf("User already logged\n");
                send_message_single("User already logged\n", user->sokcet_desc);
                
                return 0;
            }
        }
        strcpy(user->name, name);
        name_len = strlen(name);
        
    }
    ret = recv(user->sokcet_desc, password, 32 ,0);
    if(ret == -1){
        handle_error("Can not recive password\n");
    }

    if(DEBUG) printf("Name: %s\n", name);
    if(DEBUG) printf("Password: %s", password);

    pthread_mutex_lock(&mutex);

    login_fd = open("login.txt", O_CREAT|O_RDWR|O_APPEND, 0660);
    if(login_fd == -1){
        handle_error("Can not open login_fd");
    }
    //check if the name is on db
    int file_size = lseek(login_fd,0,SEEK_END);
    lseek(login_fd,0,SEEK_SET);
    if(DEBUG) printf("File size %d\n: ",file_size);

    char login_buf[file_size];
    ret = read(login_fd,login_buf,file_size);
    if(ret == -1){
        handle_error("Read login");
    }
    
    char *credentials = strtok(login_buf,"\n");

    if(new_user_opt){
        if(DEBUG) printf("New user regi\n");
        success = 1;
        while(credentials != NULL){
            if(memcmp(credentials,name,name_len) == 0){
                
                printf("User name already taken\n");
                success = 0;
                send_message_single(QUIT_COMMAND,user->sokcet_desc);
                send_message_single("User name already taken\n",user->sokcet_desc);
                break;

            }
            credentials = strtok(NULL,"\n");
        }
        //Controllare condizione non registra piu
        if(success != 0){
            strcat(name,password);
            if(DEBUG) printf("Concat: %s\n",name);
            ret = write(login_fd,name,strlen(name));
            if(ret == -1){
                handle_error("Can not write new user in file");
            }
            printf("New user registred!\n");
            success = 1;
        }
        
    }
    else{
        
        if(DEBUG) printf("Old User\n");
        
        strcat(name,password);
        newline_remove(name);
        
        if(DEBUG) printf("Concat: %s\n",name);
        
        while(credentials != NULL){
            
            if(DEBUG) printf("Credenzianli: %s\n",credentials);
           
            if(strcmp(credentials,name) == 0){
                printf("User %s logged in succesfully\n",user->name);
                success = 1;
                break;
            }
            
            credentials = strtok(NULL,"\n");
        }
        
        if(success == 0){
            printf("Login failed\n");
            send_message_single(QUIT_COMMAND,user->sokcet_desc);
            send_message_single("User name or password are incorrect\n",user->sokcet_desc);
        }
    }

    ret = close(login_fd);
    if(ret == -1){
        handle_error("Can not close login_fd");
    }
    pthread_mutex_unlock(&mutex);

    printf("User: %s has joined the server with uid: %d\n",user->name, user->uid);

    return success;
}

void *connection_handler(void *arg){
    
    if(DEBUG) fprintf(stderr, "We are in connection handler\n");

    int ret, running;
    char buf[BUFFER_SIZE];
    char buf_out[BUFFER_SIZE+32];

    user_t *user = (user_t*)arg;

    // recive username
    
    running = login_handler(user);
    if(DEBUG) printf("Runnig %d \n",running);

    logged_users++;

    //if(DEBUG) print_user_list();

    while(running){
        //read message from client
        int bytes_read = 0;
        size_t buf_len = sizeof(buf_out);
        memset(buf, 0, buf_len);
        if(DEBUG) printf("Bau Ruunig\n");
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

        if(memcmp(buf,QUIT_COMMAND,quit_command_len) == 0){
            if(DEBUG) printf("can we quit?\n");
            running = 0;
            break;
        }

        buf_len = sizeof(buf_out);
        memset(buf_out, 0, buf_len);
        //handle the server option and the response
        if(memcmp(buf,"@",1) == 0){
            if(DEBUG) printf("@ OK\n");
            char buf2[BUFFER_SIZE];
            char recv_name[32];
            //remove @
            memcpy(buf,buf+1,sizeof(buf));
            
            strcpy(buf2,buf);
            printf("buf2: %s",buf2);
            //take name
            char *recv_tok = strtok(buf," ");
            
            strcpy(recv_name,recv_tok);
            if(DEBUG) printf("recv NAMA %s \n",recv_name);
            int name_len = strlen(recv_tok);
            printf("Name len: %d\n",name_len);
            
            //remove sapce
            memcpy(buf2,buf2+1,sizeof(buf2));
            memcpy(buf2,buf2+name_len,sizeof(buf2));
            
            printf("buf2 after cpoy %s",buf2);
            strcpy(buf,buf2);
            
            if(DEBUG) printf("recv NAME %s \n",recv_name);
            int i;
            for(i = 0;i<logged_users;i++){
                if(memcmp(user_list[i]->name, recv_name, name_len) == 0){
                    if(DEBUG) printf("Trovato!\n");
                    sprintf(buf_out, "[@%s]: %s",user->name, buf);
                    send_message_single(buf_out,user_list[i]->sokcet_desc);
                    break;
                }
                if(i == logged_users-1){

                    send_message_single("[Server] Sorry but the desired user is not online\n",user->sokcet_desc);
                }
                
            }
            
        }
        else if(memcmp(buf,"/",1) == 0){
            int command_len;
            command_len = strlen(HELP_COMMAND);
            //HELP
            if(memcmp(buf,HELP_COMMAND,command_len) == 0){
                
                send_message_single("Welcome to the Help Menu. This is the list of available commands:\n\n/QUIT   Close the client and the connection.\n/USERS  Print the list of all online users.\n/MOD Switch from public chat mode to private only and vice versa.\n@user_name  Send a private message to an online user.\n", user->sokcet_desc);
            }
            command_len = strlen(LIST_COMMAND);
            ///USER_LIST
            if(memcmp(buf,LIST_COMMAND,command_len) == 0){
                if(DEBUG) print_user_list();

                send_message_single("Current logged users with id:\n",user->sokcet_desc);
                int i;
                for(i = 0; i < logged_users; i++){

                    sprintf(buf_out,"UserName: %s ID: %d\n",user_list[i]->name, user_list[i]->uid);
                    send_message_single(buf_out,user->sokcet_desc);
                }
                
            }
            command_len = strlen(CHAT_MOD_COMMAND);
            //MOD
            if(memcmp(buf,CHAT_MOD_COMMAND,command_len) == 0){
               if(DEBUG) printf("mod start %d \n",user->global_chat);
               user->global_chat = 1 - user->global_chat;
               if(DEBUG) printf("mod start %d \n",user->global_chat);
               send_message_single("Mode changed successfully!\n",user->sokcet_desc); 
            }
            
        }
        else{
            
            //formatting the message with name
            sprintf(buf_out, "%s: %s",user->name, buf);

            send_message(buf_out, user->uid);
        }
        
    }

    //User Exit Notification
    printf("User %s is leaving...\n",user->name);
   
    //cclose ocket and remove user
    ret = close(user->sokcet_desc);
    if(ret == -1){
        handle_error("Can not close socket");
    }
   
    remove_queque(user->uid);
    
    free(user->client_addr);
    free(user);
    
    logged_users--;
   
    pthread_detach(pthread_self());

    printf("User has left the server\n");
    //TODO nitification
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    int ret;
    int socket_desc, client_desc;
    int port = 5000;

    quit_command_len = strlen(QUIT_COMMAND);
    
    pthread_t thread; 

    printf("---Starting the server---\n");

    struct sockaddr_in server_addr = {0};
    //struct sockaddr_in client_addr = {0};

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
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    if(ret){
        handle_error("Can not bind");
    }

    //listen on socket
    ret = listen(socket_desc, 10);      //need change 
    if(ret){
        handle_error("Can not listen");
    }

    printf("---SERVER UP---\n");

    while(1){

        struct sockaddr_in* client_addr =  calloc(1, sizeof(struct sockaddr_in));

        //accept connection

        client_desc = accept(socket_desc, (struct sockaddr*) client_addr,(socklen_t*) &sockaddr_len);
    
        if(client_desc == -1){
            handle_error("Can not open socket to accept connection");
        }

        //thrad arg
        user_t *user_args = malloc(sizeof(user_t));
        user_args->uid = uid++;
        user_args->global_chat = 1;
        user_args->sokcet_desc = client_desc;
        user_args->client_addr = client_addr;
        add_queque(user_args);
        ret = pthread_create(&thread, NULL, &connection_handler, (void*)user_args);
        if(ret){
            handle_error("Can not create a new thread");
        }

        fprintf(stderr, "Thread creato con successo\n");

    }
    //free(client_addr);
    return EXIT_SUCCESS;

}