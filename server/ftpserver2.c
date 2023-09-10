#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 12346
#define MAXLEN 1024
#define ADDRESS "127.0.0.1" // Use the server's IP address or hostname


void handleRequest(int client_socket) {
    char buffer[MAXLEN];
    ssize_t bytes_received;

    // Receive client's request
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("Receive error");
        close(client_socket);
        return;
    }

    // Parse the request
    char request_type[MAXLEN];
    char filename[MAXLEN];

    if (sscanf(buffer, "%s %s", request_type, filename) != 2) {
        printf("Invalid request format.\n");
        close(client_socket);
        return;
    }

    if (strcmp(request_type, "get") == 0) {
        // Handle "get" request
        FILE *file = fopen(filename, "rb");
        if (file == NULL) {
            printf("File not found: %s\n", filename);
            close(client_socket);
            return;
        }

        // Send the file name to the client
        send(client_socket, filename, strlen(filename), 0);

        // Send the file to the client
        while (1) {
            memset(buffer, 0, sizeof(buffer));
            size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
            if (bytes_read <= 0) {
                break; // End of file
            }

            send(client_socket, buffer, bytes_read, 0);
        }

        // Close the file
        fclose(file);
        printf("File sent to the client: %s\n");
    } else if (strcmp(request_type, "post") == 0) {
        // Handle "post" request
        FILE *file = fopen(filename, "wb");
        if (file == NULL) {
            perror("File opening error");
            close(client_socket);
            return;
        }

        // Receive the file data from the client and write it to the file
        while (1) {
            memset(buffer, 0, sizeof(buffer));
            bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break; // Client disconnected or error
            }

            // Write the received data to the file
            fwrite(buffer, 1, bytes_received, file);
        }

        // Close the file
        fclose(file);
        printf("File received from the client: %s\n", filename);
    } else if (strcmp(request_type, "delete") == 0) {
        // Handle "delete" request
        if (remove(filename) == 0) {
            printf("File deleted successfully: %s\n", filename);
        } else {
            printf("File deletion failed: %s\n", filename);
        }
    } else {
        printf("Unsupported request type: %s\n", request_type);
    }

    close(client_socket);
}
// ... (previous code)

int main() {
    int server_socket, client_socket;
    int serverlen, clientlen;
    struct sockaddr_in server_addr, client_addr;

    serverlen = sizeof(server_addr);
    bzero((char*)&server_addr, serverlen);

    // Creating the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ADDRESS);
    server_addr.sin_port = htons(PORT);

    // Bind the address with socket
    bind(server_socket, (struct sockaddr*)&server_addr, serverlen);

    // Listen for new connections
    listen(server_socket, 5);
    printf("FTP Server listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connections
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &clientlen);
        if (client_socket == -1) {
            perror("Accepting error");
            continue; // Continue to accept more connections
        }

        // Fork a new process to handle the client request
        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork error");
            close(client_socket);
        } else if (pid == 0) {
            // Child process handles the request
            close(server_socket); // Close the server socket in the child
            handleRequest(client_socket);
            exit(0); // Terminate the child process
        } else {
            // Parent process continues to accept more connections
            close(client_socket); // Close the client socket in the parent
        }
    }

    // Close the server socket (unreachable in this example)
    close(server_socket);
    printf("FTP Server closed.\n");

    return 0;
}
