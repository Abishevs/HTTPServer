#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h> 

#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 1024
#define PATH_SIZE 256
#define METHOD_MAX_BYTES 5 // 5 bytes max
#define PORT 6969

typedef enum Method 
{
    GET,
    POST,
    INVALID
} Method;

typedef struct Header  
{
    Method method;
    char path[PATH_SIZE];
    char msg;
} Header;

void set_method(Header *h, char *method)
{
    if (strcmp(method, "GET") == 0) {
        h->method = GET;
    } else if (strcmp(method, "POST" ) == 0) {
        h->method = POST;
    }  else {
        h->method = INVALID;
    }
}

int handler(char buf[BUFFER_SIZE], int client_fd)
{
    Header header;
    memset(&header, 0, sizeof(Header));
    
    char value[PATH_SIZE] = {0};
    int value_counter = 0;
    int value_len = 0;
    char *msg;

    for (int i = 0; buf[i] != '\0' && i < BUFFER_SIZE; i++) {
        if (buf[i] == ' ') {
/*          When we found space, value is ready for use */
/*          Null-terminate the method string */
            printf("Space found at index %d\n", i);
            value[value_len] = '\0';

            if (value_counter == 0){
                // memset(value, 0, sizeof(value)); /* CLR VALUE */
                set_method(&header, value);
                printf("value: %s, METHOD: %d\n", value, header.method);
                if (header.method == INVALID) {
                /* TODO: Throw 400 Bad Request Instantlyy */
                    msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 4\r\n\r\nBAD:";
                    send(client_fd, msg, strlen(msg), 0);
                    fprintf(stderr,"%s",msg);
                    return -1;
                }


            } else if (value_counter == 1) {
                strcpy(header.path, value);
                printf("value: %s, PATH: %s\n", value, header.path);
                /*          Only intrested in these two rn, so break out */
                break;
            }
            value_counter ++;
            value_len = 0;
        } else {
            value[value_len++] = buf[i];
        }

    }

    printf("METHOD: %d, PATH: %s", header.method, header.path);
   //  char *welcome_message = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
   // 
   //  
   //  send(client_fd, welcome_message, strlen(welcome_message), 0);
    msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 1000\r\n\r\n<html> <head><title>My Page</title></head> <body>Welcome to my website!</body> </html>";

    send(client_fd, msg, strlen(msg), 0);
    return 1;
}

int main(int argc, char *argv[]) 
{
    char *root_dir = NULL;

    if (argc == 2) {
        root_dir = argv[1];
    } else {
        root_dir = argv[0];
    }

    printf("Root dir: '%s'\n", root_dir);

    char buf[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    int file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (file_descriptor < 0) {
        perror("Failed to open socket");
        return 1;
    }

    // Set the SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(file_descriptor);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);           
    server_addr.sin_addr.s_addr = INADDR_ANY;     /* Bind to (0.0.0.0) */

    if (bind(file_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(file_descriptor, MAX_CONNECTIONS) < 0) {
        perror("Failed to listen");
        return 1;
    }

    printf("Server listening on port %i...\n", PORT);

    for (;;) {
        int client_fd = accept(file_descriptor, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue; /* Try Again? */
        }

        printf("DEBUG: New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));


        ssize_t bytes_received = recv(client_fd, buf, BUFFER_SIZE - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
        } else {
            // buf[bytes_received] = '\0'; // Null-terminate the received data
            printf("Received:%s\n", buf);
            handler(buf, client_fd);
        }
        
        close(client_fd);
    }


    close(file_descriptor);
    return 0;
}

