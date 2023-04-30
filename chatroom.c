#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int server_fd, client_fds[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create a TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        error("socket() failed");
    }

    // Bind the socket to a port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("bind() failed");
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        error("listen() failed");
    }

    printf("Server started on port %d\n", PORT);

    // Loop to handle incoming client connections
    int max_fd, activity, i, valread, sd;
    fd_set readfds;
    char *welcome_message = "Welcome to the chat room!\n";
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = 0;
    }
    while (1) {
        // Clear the file descriptor set
        FD_ZERO(&readfds);

        // Add the server socket to the file descriptor set
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        // Add the client sockets to the file descriptor set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_fds[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_fd) {
                max_fd = sd;
            }
        }

        // Wait for activity on one of the sockets
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            error("select() failed");
        }

        // If there is incoming activity on the server socket, accept the connection
        if (FD_ISSET(server_fd, &readfds)) {
            int new_fd;
            if ((new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                error("accept() failed");
            }

            // Add the new client to the list of client sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = new_fd;
                    break;
                }
            }

            // Send a welcome message to the new client

            if (send(new_fd, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message)) {
                error("send() failed");
            }

            printf("New client connected. Socket fd is %d, IP is %s, Port is %d\n", new_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }

        // Check for activity on the client sockets
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_fds[i];
            if (FD_ISSET(sd, &readfds)) {
                // Read incoming message from the client
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // Client disconnected, remove it from the list of client sockets
                    close(sd);
                    client_fds[i] = 0;
                } else {
                    // Send the message to all other clients in the chat room
                    buffer[valread] = '\0';
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        int dest_fd = client_fds[j];
                        if (dest_fd > 0 && dest_fd != sd) {
                            if (send(dest_fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
                                error("send() failed");
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
