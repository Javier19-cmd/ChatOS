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

volatile sig_atomic_t flag = 0;

int sockfd = 0;

char name[NAME_LEN];


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

void catch_ctrl_c_and_exit(){
    flag = 1;
}

void recv_msg_handler(){
    char message[BUFFER_SZ] = {};

    while(1){
        int receive = recv(sockfd, message, BUFFER_SZ, 0);
        
        if (receive > 0){
            printf("\n%s\n", message);
            str_overwrite_stdout();
        }else if (receive == 0){
            break;
        }
        bzero(message, BUFFER_SZ);
    }
}

void send_msg_handler(){
    char buffer[BUFFER_SZ] = {};
    char message[BUFFER_SZ + NAME_LEN] = {};

    printf("Opcion 1: mandar broadcast \n");
    printf("Opcion 2: mandar mensaje privado \n");
    printf("Opcion 3: ver lista de usuarios \n");
    printf("Opcion 4: salir \n");

    // int opc = 0;

    // // Agarrando el input del usuario.

    // if (opc == 1){
    //     printf("Ingresa el mensaje que quieres mandar: ");
    //     char *msg = NULL;
    //     fgets(msg, BUFFER_SZ, stdin);
    //     str_trim_lf(msg, BUFFER_SZ);
    //     send(sockfd, msg, BUFFER_SZ, 0);
    // }



    while(1){

        str_overwrite_stdout();

        printf("Ingrese la opcion que desee: ");

        char opcion[BUFFER_SZ] = {};

        fgets(opcion, BUFFER_SZ, stdin);

        str_trim_lf(opcion, BUFFER_SZ);

        if (strcmp(opcion, "1") == 0){
            printf("Ingresa el mensaje que quieres mandar: ");
            char msg[BUFFER_SZ + NAME_LEN] = {};
            fgets(msg, BUFFER_SZ, stdin);
            str_trim_lf(msg, BUFFER_SZ);
            send(sockfd, msg, BUFFER_SZ, 0);

            bzero(opcion, BUFFER_SZ);
            bzero(msg, BUFFER_SZ + NAME_LEN);

        }


        // fgets(buffer, BUFFER_SZ, stdin);
        
        

        //printf("Texto de prueba: %s", buffer);
        // str_trim_lf(buffer, BUFFER_SZ);

        // if (strcmp(buffer, "exit") == 0){
        //     break;
        // }else{
        //     sprintf(message, "%s: %s\n", name, buffer);
        //     send(sockfd, message, strlen(message), 0);
        // }

        // bzero(buffer, BUFFER_SZ);
        // bzero(message, BUFFER_SZ + NAME_LEN);
    }
    catch_ctrl_c_and_exit(2);
}

int main(int argc, char **argv){

    if (argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Please enter your name: ");
    
    fgets(name, NAME_LEN, stdin);
    
    str_trim_lf(name, strlen(name));

    if (strlen(name) > NAME_LEN - 1 || strlen(name) < 2){
        printf("Ingresa bien tu nombre, por favor \n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    // Socket settings
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    
    
    // Connect to the server.
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (err == -1){
        printf("ERROR: connect\n");
        printf("Error description: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Send the name
    send(sockfd, name, NAME_LEN, 0);
    
    printf("=============================\n");
    printf("\tCHATROOM el buen samaritano\n");
    printf("=============================\n");



    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    while (1){
        if(flag){
            printf("\nBye\n");
            break;
        }
    }

    close(sockfd);



    return EXIT_SUCCESS;
}


