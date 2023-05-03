#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

// Client structure
typedef struct{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    int status;
    char name[NAME_LEN];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout(){
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char *arr, int length){
    int i;
    for(i = 0; i < length; i++){
        if(arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}


void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(!clients[i]){
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);

    client_t *cl = NULL;

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                cl = clients[i];
                clients[i] = NULL;
                printf("Client disconnected: %d\n", uid);
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int get_uid_by_name(char *name){
    
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(strcmp(clients[i]->name, name) == 0){
                return clients[i]->uid;
            }
        }
    }
    return -1;
}

void show_user_info_by_uid(int uid, int uid_to_send){
    pthread_mutex_lock(&clients_mutex);
    // get the name of all the users
    // that do not have the same uid as the current user
    // and assign it to the variable users

    char users[BUFFER_SZ] = {};

    strcat(users, "********************\n");
    strcat(users, "User:\n");
    for (int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                strcat(users,"Name: ");
                strcat(users, clients[i]->name);
                strcat(users, "\n");
                strcat(users, "UID: ");
                char uid_str[12];
                sprintf(uid_str, "%d", clients[i]->uid);
                strcat(users, uid_str);
                strcat(users, "\n");
                strcat(users, "IP: ");
                strcat(users, inet_ntoa(clients[i]->address.sin_addr));
                strcat(users, "\n");
                strcat(users, "Port: ");
                char port_str[12];
                sprintf(port_str, "%d", ntohs(clients[i]->address.sin_port));
                strcat(users, port_str);
                strcat(users, "\n");
            }
        }
    }

    strcat(users, "********************\n");

    pthread_mutex_unlock(&clients_mutex);

    // 
    send_message_to_specific_client(users, uid_to_send, uid_to_send);


}

void show_users(int uid){
    pthread_mutex_lock(&clients_mutex);
    // get the name of all the users
    // that do not have the same uid as the current user
    // and assign it to the variable users

    char users[BUFFER_SZ] = {};

    strcat(users, "********************\n");
    strcat(users, "Users:\n");


    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid != uid){
                strcat(users, clients[i]->name);
                strcat(users, " (online)\n");
            }
        }
    }

    strcat(users, "********************\n");
    pthread_mutex_unlock(&clients_mutex);

    // Send the user with given uid the list of users
    send_message_to_specific_client(users, uid, uid);



}



void send_message(char *s, int uid){
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid != uid){
                if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void send_message_to_specific_client(char *s, int uid, int uid_to){
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid == uid_to){
                if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void *handle_client(void *arg){
    char buff_out[BUFFER_SZ];
    char name[NAME_LEN];
    int leave_flag = 0;

    cli_count++;
    client_t *cli = (client_t *)arg;

    // Name
    if(recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) <  2 || strlen(name) >= NAME_LEN - 1){
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    }else{
        strcpy(cli->name, name);
        sprintf(buff_out, "%s has joined\n", cli->name);
        printf("%s", buff_out);
        send_message(buff_out, cli->uid);
    }

    bzero(buff_out, BUFFER_SZ);

    while(1){
        if(leave_flag){
            break;
        }

        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
        if(receive > 0){
            if(strlen(buff_out) > 0){

                // split the string by $@$
                char *token = strtok(buff_out, "$@$");
                
                // first token is the message
                char *msg = token;

                // second token is the uid_to
                token = strtok(NULL, "$@$");
                char *opcion = token;

                // if opcion is 1, then send to all
                if(strcmp(opcion, "1") == 0){
                    char newMessage[BUFFER_SZ] = {};

                    strcat(newMessage, "********************\n");
                    strcat(newMessage, cli->name);
                    strcat(newMessage, ": ");
                    strcat(newMessage, msg);
                    strcat(newMessage, "\n");
                    strcat(newMessage, "********************\n");


                    send_message(newMessage, cli->uid);
                    str_trim_lf(buff_out, strlen(buff_out));
                    printf("%s -> %s\n", buff_out, cli->name);
                }else if(strcmp(opcion, "2") == 0){
                    char *name = strtok(NULL, "$@$");
                    int uid_to = get_uid_by_name(name);
                    if(uid_to != -1){

                        char newMessage[BUFFER_SZ] = {};

                        strcat(newMessage, "********************\n");
                        strcat(newMessage, "Private message from ");
                        strcat(newMessage, cli->name);
                        strcat(newMessage, "\n");
                        strcat(newMessage, msg);
                        strcat(newMessage, "\n");
                        strcat(newMessage, "********************\n");

                        send_message_to_specific_client(newMessage, cli->uid, uid_to);
                        str_trim_lf(buff_out, strlen(buff_out));
                        printf("%s -> %s\n", buff_out, cli->name);
                    }
                    else{
                        char error[BUFFER_SZ] = {};
                        strcat(error, "********************\n");
                        strcat(error, "ERROR: User not found\n");
                        strcat(error, "********************\n");
                        send_message_to_specific_client(error, cli->uid, cli->uid);
                    }

                }else if(strcmp(opcion, "3") == 0){
                    // print users  
                    printf("Showing users\n");
                    show_users(cli->uid);

                }
                else if(strcmp(opcion,"4")==0){
                    printf("Showing user info");
                    char *name = msg;

                    int uid_to = get_uid_by_name(name);

                    if(uid_to != -1){
                        show_user_info_by_uid(uid_to, cli->uid);                        
                    }
                    else{
                        char error[BUFFER_SZ] = {};
                        strcat(error, "********************\n");
                        strcat(error, "ERROR: User not found\n");
                        strcat(error, "********************\n");
                        send_message_to_specific_client(error, cli->uid, cli->uid);
                    }

                }
            

            }
        }else if(receive == 0 || strcmp(buff_out, "exit") == 0){
            sprintf(buff_out, "%s has left\n", cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->uid);
            leave_flag = 1;
        }else{
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SZ);
    }

    // Delete client from queue and yield thread
    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv){
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    int option = 1;

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    // Socket settings
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    // Ignore pipe signals
    signal(SIGPIPE, SIG_IGN);

    if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
        perror("ERROR: setsockopt failed");
        return EXIT_FAILURE;
    }

    // Bind
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    // Listen
    if(listen(listenfd, 10) < 0){
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("=============================\n");
    printf("\tCHATROOM el buen samaritano\n");
    printf("=============================\n");


    while(1){
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        // Check for max clients
        if((cli_count + 1) == MAX_CLIENTS){
            printf("Max clients reached. Rejected: ");
            print_client_addr(cli_addr);
            printf(":%d\n", cli_addr.sin_port);
            close(connfd);
            continue;
        }

        // Client settings
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;
        cli->status = 1;

        // Add client to the queue and fork thread
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        // Reduce CPU usage
        sleep(1);


    }


    return EXIT_SUCCESS;


}