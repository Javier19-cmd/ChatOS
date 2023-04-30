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

        // Opcion para mandar mensaje grupal.
        if (strcmp(opcion, "1") == 0){
            printf("Ingresa el mensaje que quieres mandar: ");
            char msg[BUFFER_SZ + NAME_LEN] = {};
            fgets(msg, BUFFER_SZ, stdin);
            str_trim_lf(msg, BUFFER_SZ);

            // Concatenando la opcion junto al mensaje.
            char msg2[BUFFER_SZ + NAME_LEN + 4] = {}; // Reservando espacio para agregar la opcion.
            strcpy(msg2, msg); // Copiando el mensaje original en una nueva variable.
            strcat(msg2, "$@$");
            strcat(msg2, opcion);

            // Imprimiendo el mensaje para verificar como se va a mandar.
            // printf("Mensaje a mandar: %s\n", msg2); 

            send(sockfd, msg2, BUFFER_SZ, 0);

            bzero(opcion, BUFFER_SZ);
            bzero(msg, BUFFER_SZ + NAME_LEN);
            bzero(msg2, BUFFER_SZ + NAME_LEN+4);

        }
        // Opcion 2: Mandar mensaje privado.
        else if (strcmp(opcion, "2") == 0){
            printf("Ingresa el nombre del usuario al que quieres mandar el mensaje: ");
            char user[NAME_LEN] = {};
            fgets(user, BUFFER_SZ, stdin);
            str_trim_lf(user, BUFFER_SZ);
            printf("Ingresa el mensaje que quieres mandar: ");
            char msg[BUFFER_SZ + NAME_LEN] = {};
            fgets(msg, BUFFER_SZ, stdin);
            str_trim_lf(msg, BUFFER_SZ);

            // Concatenando el nombre del usuario, el mensaje, la opción y la coma.
            char msg2[BUFFER_SZ + NAME_LEN + strlen(user) + strlen(", ") + strlen(opcion)];
            sprintf(msg2, "%s$@$%s$@$%s", msg, opcion, user);

            // Imprimiendo el mensaje para verificar cómo se va a mandar.
            // printf("Mensaje a mandar: %s\n", msg2);

            // Mandando el mensaje al servidor.
            send(sockfd, msg2, strlen(msg2), 0);

            bzero(opcion, BUFFER_SZ);
            bzero(msg, BUFFER_SZ + NAME_LEN);
            bzero(user, BUFFER_SZ);
        
        }// Opcion 3: Ver lista de usuarios.
        else if (strcmp(opcion, "3") == 0) {
            // Opcion para ver los usuarios en el servidor.

            // Creando una variable string que solo tenga tabla dentro de "".
            char tabla[NAME_LEN] = "tabla";

            // Concatenando en una variable la variable tabla y la opcion elegida.
            char msg[BUFFER_SZ + NAME_LEN + strlen(tabla) + strlen(", ") + strlen(opcion)];
            sprintf(msg, "%s$@$%s", tabla, opcion);

            // Imprimiendo el mensaje para verificar como se va a mandar.
            // printf("Mensaje a mandar: %s\n", msg);

            // Mandando el mensaje al servidor.
            send(sockfd, msg, strlen(msg), 0);

            bzero(opcion, BUFFER_SZ);
            bzero(msg, BUFFER_SZ + NAME_LEN + strlen(tabla) + strlen(", ") + strlen(opcion));
        
        }// Opcion 4: Salir del programa.
        else if (strcmp(opcion, "4") == 0){

            // Esta opcion es para salir de la ejecuion.
            printf("Saliendo del programa...\n");

            // Mandando el mensaje al servidor.
            break;


        }

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


