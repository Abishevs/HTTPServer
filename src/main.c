#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h> 

#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 1024
#define PORT 6969

typedef enum Method 
{
    GET,
    POST,
} Method;

typedef struct Header  
{
    Method method;
    char *path;
    char *msg;
} Header;

int handler(char buf[BUFFER_SIZE], int client_fd)
{
    char method[256];
    int i = 0;

    while (i < BUFFER_SIZE && buf[i] != '\n' && i < 255) {
        method[i] = buf[i];
        i++;
    }

    // Null-terminate the method string
    method[i] = '\0';

    printf("First line: %s\n", method);
    return 1;
}

int main() 
{

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

        // char *welcome_message = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        // 
        //
        // send(client_fd, welcome_message, strlen(welcome_message), 0);
        ssize_t bytes_received = recv(client_fd, buf, BUFFER_SIZE - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
        } else {
            buf[bytes_received] = '\0'; // Null-terminate the received data
            printf("Received: %s\n", buf);
            handler(buf, client_fd);
        }
        
        close(client_fd);
    }


    close(file_descriptor);
    return 0;
}

