#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define DEBUG 0
#define QUIT_COMMAND "/QUIT\n"

int socket_desc;
int quit, quit_command_len;
int max_attemps = 3;

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

//TODO QUIT COMMAND
//Welcome message + mod of use
//check if the user is already registered 
//TODO fix mesaaage short

void handle_ctrlc(){
    int ret;
    ret = send(socket_desc, QUIT_COMMAND, quit_command_len,0);
    if(ret == -1){
        handle_error("Can not send quit message to server");
    }
    quit = 1;
}

int new_user(){

    int current_attemps;
    char val[4];

    for(current_attemps = 1; current_attemps <= max_attemps; ++current_attemps){
        
        printf("Are you a new user?(y/n)\n");
        if(fgets(val,4,stdin) != (char*) val){
        
            fprintf(stderr, "Can not read from stdin\n");
            exit(EXIT_FAILURE);
        }

        newline_remove(val);
        
        if(strcmp(val,"y") == 0){
            return 1;
        }
        
        if(strcmp(val,"n") == 0){
            return 0;
        }

        printf("Please enter valid input\n");
        printf("Attemp(s): %d/3\n", current_attemps);
        
        if(current_attemps == max_attemps){
            printf("Maximum number of login attempts reached.Exiting...\n ");
            exit(EXIT_FAILURE);
            
        }
    }
    exit(EXIT_FAILURE);
}


//TODO CHEKC spacial char
//check login credentials
int login(char* name, char* password){
   
    int current_attemps;

    for(current_attemps = 1; current_attemps <= max_attemps; ++current_attemps){
        
        printf("Enter your username: \n");
        if(fgets(name,32,stdin) != (char*) name){
        
            fprintf(stderr, "Can not read from stdin\n");
            exit(EXIT_FAILURE);
        }
        printf("Enter your password (min 2 characters): \n");       //2 provv
        if(fgets(password,32,stdin) != (char*) password){
        
            fprintf(stderr, "Can not read from stdin\n");
            exit(EXIT_FAILURE);
        }
        
        newline_remove(name);
        int name_len = strlen(name);
        int pass_len = strlen(password);
        
        if(DEBUG) printf("Name len: %d \n", name_len);
        if(DEBUG) printf("Password len: %d\n",pass_len);

        if(name_len > 0 && pass_len >= 2){
            return 1;
        }
        
        if(name_len == 0){
            fprintf(stderr, "Name field can not be empty\n");
            printf("Attemp(s): %d/3\n", current_attemps);
        }
        
        if(pass_len < 2){
           fprintf(stderr, "The password is too short\n"); 
           printf("Attemp(s): %d/3\n", current_attemps);
        }

        if(current_attemps == max_attemps){
            printf("Maximum number of login attempts reached.Exiting...\n ");
            return 0;
        }
        
    }
    return 0;

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

        if(memcmp(buf,QUIT_COMMAND,quit_command_len) == 0){
            if (DEBUG) printf("can we quit?\n");
            quit = 1;
            break;
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
    size_t buf_len = sizeof(buf_out);
    //TODO messaggi per interrompere connessone
    
    while(1){
        ret = recv(socket_desc, buf_out, 2048,0); 

        if(ret > 0){
            printf(":> %s", buf_out);
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

        if(memcmp(buf_out,QUIT_COMMAND,quit_command_len) == 0){
            printf("Exiting...\n");
            quit = 1;
            break;
        }

        recv_bytes += ret;
        if(DEBUG) printf("We recive %d bytes\n", recv_bytes);
        memset(buf_out, 0, buf_len);
    }
}

int main(int argc, char* argv[]){
    int ret;
    //provvisional address
    int port = 5000;
    char *ad = "127.0.0.1";
    char name[32];
    char password[32];
    
    quit = 0;
    quit_command_len = strlen(QUIT_COMMAND);

    //TODO login in enad exit if failed
    //TODO attemps for login name and password check
    int new_user_opt = new_user();
    if(DEBUG) printf("New User: %d \n",new_user_opt);
    int login_val = login(name,password);
    if(DEBUG) printf("Login value: %d \n",login_val);

    //
    signal(SIGINT,handle_ctrlc);

    //variables for handling a socket
    
    if(login_val){
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

        //send if the user is a new one
        int opt_conv = htonl(new_user_opt);
        if(new_user_opt){
            ret = send(socket_desc, &opt_conv, sizeof(opt_conv), 0);
            if(ret == -1){
                handle_error("send new user");
            }
        }
        else{
            ret = send(socket_desc, &opt_conv, sizeof(opt_conv), 0);
            if(ret == -1){
                handle_error("send new user");
            }
        }
        
        //send name to the server
        ret = send(socket_desc, name, 32, 0);
        if(ret == -1){
            handle_error("send name");
        }

        //send password to the server
        ret = send(socket_desc, password, 32, 0);
        if(ret == -1){
            handle_error("send password");
        }
        
        //TO DO  send & recive
        printf("Spawnig Thread\n");

        pthread_t send_thread;
        pthread_t recv_thread;

        ret = pthread_create(&recv_thread, NULL, (void*)recv_message_client, NULL);
        ret = pthread_create(&send_thread, NULL, (void*)send_message_client, NULL);
        
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

    //close 3d

    }

    if(DEBUG) printf("EXIT\n");

    exit(EXIT_SUCCESS);
}
