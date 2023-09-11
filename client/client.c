#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 12346
#define ADDRESS "127.0.0.1"
#define MAXLEN 1024

// Function to send a delete request to the server
void sendDeleteRequest(int client_socket, const char* filename) {
    char request[MAXLEN];
    snprintf(request, sizeof(request), "delete %s", filename);
    send(client_socket, request, strlen(request), 0);
}

// Function to send a post request to the server along with file data
void sendPostRequest(int client_socket, const char* filename) {
    char request[MAXLEN];
    snprintf(request, sizeof(request), "post %s", filename);
    send(client_socket, request, strlen(request), 0);

    // Open the file for reading
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File opening error");
        return;
    }

    // Read and send the file data to the server
    char buffer[MAXLEN];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Close the file
    fclose(file);

    printf("Post request sent for file: %s\n", filename);
}


// Function to receive the file name from the server and create the file
int receiveAndCreateFile(int client_socket) {
    char buffer[MAXLEN];
    memset(buffer, 0, sizeof(buffer));

    // Receive the file name from the server
    ssize_t filename_length = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (filename_length <= 0) {
        perror("File name receive error");
        return -1; // Error
    }

    char received_filename[MAXLEN];
    strncpy(received_filename, buffer, filename_length);
    received_filename[filename_length] = '\0';

    // Prompt the user for the request
    printf("Received file name: %s\n", received_filename);

    // Open a file for writing using the received file name
    FILE *file = fopen(received_filename, "wb");
    if (file == NULL) {
        perror("File opening error");
        return -1; // Error
    }

    // Receive the file data from the server
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break; // Server disconnected or error
        }

        // Write the received data to the file
        fwrite(buffer, 1, bytes_received, file);
    }

    // Close the file
    fclose(file);

    return 0; // Success
}

int main() {
    int client_socket;
    struct sockaddr_in client_addr;
    char request[MAXLEN];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation error");
        exit(1);
    }
    printf("Socket created successfully\n");

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(ADDRESS);
    client_addr.sin_port = htons(PORT);

    printf("Connected successfully\n");

    if (connect(client_socket, (struct sockaddr *) &client_addr, sizeof(client_addr)) == -1) {
        perror("Connection error");
        close(client_socket);
        exit(1);
    }

    // Prompt the user for the request
    printf("Enter the request (e.g., 'get server_file.txt', 'delete file.txt', 'post file.txt'): ");
    fgets(request, sizeof(request), stdin);

    // Remove the trailing newline character
    size_t len = strlen(request);
    if (len > 0 && request[len - 1] == '\n') {
        request[len - 1] = '\0';
    }

    if (strncmp(request, "get", 3) == 0) {
        // Handle get request
        send(client_socket, request, strlen(request), 0);
        if (receiveAndCreateFile(client_socket) == -1) {
            exit(1);
        }
    } else if (strncmp(request, "delete", 6) == 0) {
        // Handle delete request
        sendDeleteRequest(client_socket, request + 7); // Skip "delete " prefix
        printf("Delete request sent for file: %s\n", request + 7);
    } else if (strncmp(request, "post", 4) == 0) {
        // Handle post request
        sendPostRequest(client_socket, request + 5); // Skip "post " prefix
        printf("Post request sent for file: %s\n", request + 5);
    } else {
        printf("Invalid request format.\n");
    }

    // Close the socket
    close(client_socket);
    printf("FTP Client closed.\n");

    return 0;
}