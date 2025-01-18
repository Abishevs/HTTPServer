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
} Header;

const char* method_to_string(Method method)
{
    switch (method) {
        case GET:       return "GET";
        case POST:      return "POST";
        case INVALID:   return "INVALID";
    }
}

void print_header(Header *h)
{
    printf("Method %s ", method_to_string(h->method));
    printf("%s\n", h->path);
}

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

int header_parser(char buf[BUFFER_SIZE], Header *h)
{
    char value[PATH_SIZE] = {0};
    int value_counter = 0;
    int value_len = 0;

    for (int i = 0; buf[i] != '\0' && i < BUFFER_SIZE; i++) {
        if (buf[i] == ' ') {
/*          When we found space, value is ready for use */
/*          Null-terminate the method string */
            value[value_len] = '\0';

            if (value_counter == 0){
                set_method(h, value);
                // printf("[DEBUG] value: %s, METHOD: %d\n", value, h->method);

                if (h->method == INVALID) {
                    return -1; /* EXit Early */
                }

            } else if (value_counter == 1) {

                const char *index = "/index.html";
                if (strcmp(value, "/") == 0) {
                    strcpy(h->path, index);
                    return 1;
                }

                strcpy(h->path, value);
                // printf("[DEBUG]value: %s, PATH: %s\n", value, h->path);
            /*  Only intrested in these two rn, so break out */
                return 1;
            }
/*          Update for next value */
            value_counter ++;
            value_len = 0;
        } else {
            value[value_len++] = buf[i];
        }

    }

    return 1;
}

const char* get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream"; /* Default binary type */

    if (strcmp(ext, ".png") == 0)   return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)   return "image/gif";
    if (strcmp(ext, ".html") == 0)  return "text/html";
    if (strcmp(ext, ".txt") == 0)   return "text/plain";
    if (strcmp(ext, ".css") == 0)   return "text/css";
    if (strcmp(ext, ".js") == 0)    return "application/javascript";

    return "application/octet-stream"; 
}

void serve_file(Header *h, int client_fd, char *root_dir) {
    char file_path[PATH_SIZE];
    snprintf(file_path, PATH_SIZE, "%s/.%s", root_dir, h->path); // Prepend '.' for relative paths

    // Open the file in binary mode
    FILE *input_file = fopen(file_path, "rb");
    if (!input_file) {
        char *not_found_msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
        send(client_fd, not_found_msg, strlen(not_found_msg), 0);
        return;
    }

/*      Determine file size */
    fseek(input_file, 0, SEEK_END);
    size_t file_size = ftell(input_file);
    rewind(input_file);

/*     Prepare HTTP response headers */
    char response_headers[BUFFER_SIZE];
    const char *content_type = get_content_type(h->path);
    snprintf(response_headers, BUFFER_SIZE,
         "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",
         content_type, file_size);

/*      Send headers first */
    send(client_fd, response_headers, strlen(response_headers), 0);

/*      Read and send file content in chunks */
    char file_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, input_file)) > 0) {
        send(client_fd, file_buffer, bytes_read, 0);
    }

    fclose(input_file);
}

void handler(char buf[BUFFER_SIZE], int client_fd, char *root_dir) {
    Header header;
    memset(&header, 0, sizeof(Header));

    if (header_parser(buf, &header) < 0) {
        char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
        send(client_fd, bad_request, strlen(bad_request), 0);
        return;
    }

    if (header.method == GET) {
        serve_file(&header, client_fd, root_dir);
    } else if (header.method == POST) {
        char *not_implemented = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 15\r\n\r\nNot Implemented";
        send(client_fd, not_implemented, strlen(not_implemented), 0);
    } else {
        char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
        send(client_fd, bad_request, strlen(bad_request), 0);
    }

    print_header(&header);
}

int main(int argc, char *argv[]) 
{
    char *root_dir = NULL;

    if (argc == 2) {
/*      Optional dir */
        root_dir = argv[1];
    } else {
        root_dir = "";
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

    printf("Server listning at 127.0.0.1:%i ...\n", PORT);

    for (;;) {
        int client_fd = accept(file_descriptor, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue; /* Try Again? */
        }

        // printf("DEBUG: New connection from addr: %s:%d\n",
        //        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));


        ssize_t bytes_received = recv(client_fd, buf, BUFFER_SIZE - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
        } else {
            // printf("[DEBUG] Received:%s\n", buf);
            handler(buf, client_fd, root_dir);
        }
        
        close(client_fd);
    }


    close(file_descriptor);
    return 0;
}

