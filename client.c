/*******************************************************************************
 * Name        : client.c
 * Author      : Jake Paccione
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <sys/select.h>


int opt;
char* IP_address = "127.0.0.1";
int port = 25555;

/**
 * Displays the help message to the client.
 */
void display_help(char* name){
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n", name);
    printf("  -i IP_address         Default to \"127.0.0.1\";\n");
    printf("  -p port_number        Default to 25555;\n");
    printf("  -h                    Display this help info.\n");
}

/**
 * Handles all client-side game functionality.
 */
void parse_connect(int argc, char** argv, int server_fd){
        char buffer[1024];
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);;

    /* STEP 1:
       Create a socket to talk to the server;
       Set up socket for the client side;
    */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 3:
       Try to connect to the server.
    */
    connect(server_fd, (struct sockaddr *) &server_addr, addr_size);

    /* Read a line from client's terminal
    and send that line to the server
    */
    printf("Please type your name: "); fflush(stdout);
    int stdinbytes = read(0, buffer, 1024);
    buffer[stdinbytes] = 0;
    send(server_fd, buffer, strlen(buffer), 0);

    fd_set read_fd_set;
    int max_socket;

    while(1) {
        // Zero out active fd_set
        FD_ZERO(&read_fd_set);
        // Add client socket to the set
        FD_SET(server_fd, &read_fd_set);
        FD_SET(STDIN_FILENO, &read_fd_set);
        // Prep for finding max socket to pass to select()
        if(server_fd > STDIN_FILENO){
            max_socket = server_fd;
        } else {
            STDIN_FILENO;
        }

        // Use select() to actively monitor fds
        int select_ret;
        if ((select_ret = select(max_socket+1, &read_fd_set, NULL, NULL, NULL)) < 0) {
            perror("select()");
            exit(EXIT_FAILURE);
        }


        // Receive response from the server
        if (FD_ISSET(server_fd, &read_fd_set)) {
            int recvbytes = recv(server_fd, buffer, 1024, 0);
            if (recvbytes == 0) break;
            else {
                buffer[recvbytes] = 0;
                printf("%s", buffer); fflush(stdout);
            }
        }

        // Send answer to the server
        if (FD_ISSET(STDIN_FILENO, &read_fd_set)) {
            int stdinbytes = read(0, buffer, 1024);
            buffer[stdinbytes] = 0;
            send(server_fd, buffer, strlen(buffer), 0);
            fflush(stdout);
        }
    }

    close(server_fd);
}

/**
 * Main function that handles flags and calls parse_socket.
 */
int main(int argc, char* argv[]){
    // Handles flags from user 
    while((opt = getopt(argc, argv, ":i:p:h")) != -1){
        switch(opt){
            case 'i':
                if (inet_addr(optarg) == -1) {
                    fprintf(stderr, "Error: Must provide a valid IP address.\n");
                    exit(EXIT_FAILURE);
                }
                IP_address = optarg;
                break;
            case 'p':
                if (atoi(optarg) <= 0 || atoi(optarg) >= 49152){
                    fprintf(stderr, "Error: Must provide a valid usable port.\n");
                    exit(EXIT_FAILURE);
                }
                port = atoi(optarg);
                break;
            case 'h':
                display_help(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case ':':
                printf("Error: Option '-%c' expects an argument.\n", optopt);
                exit(EXIT_FAILURE);
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                exit(EXIT_FAILURE);
        }
    }


    int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    parse_connect(argc, argv, server_fd);
    
    return 0;
}
