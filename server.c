#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <bits/getopt_core.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define MAX_CONNECTIONS 3
#define max(a, b) ((a) > (b) ? (a) : (b))

// Storing Questions
struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

// Storing Player Data
struct Player {
    int fd;
    int score;
    char name[128];
};

struct Player game[3];

/**
 * Stores data from the file into an empty Entry array.
 */
int read_questions(struct Entry* arr, char* filename){
    FILE* stream;
    char* line = NULL;
    size_t len = 0;
    ssize_t nread = 0;

    if ((stream = fopen(filename, "r")) == NULL){
        fprintf(stderr, "Error in opening file!\n");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    int i = 0;
    while((nread = getline(&line, &len, stream)) != -1){
        if (strcmp(line, "\n") == 0){
            continue;
        }
        
        switch(i){
            case 0:
                strcpy(arr[count].prompt, line);
                i++;
                break;
            case 1:
                char* token = strtok(line, " ");
                for(int i = 0; i < 3; i++){
                    strcpy(arr[count].options[i], token);
                    token = strtok(NULL, " ");
                }
                i++;
                break;
            case 2:
                for(int i = 0; i < 3; i++){
                    if (strcmp(line, arr[count].options[i]) == 0){
                        arr[count].answer_idx = i;
                        break;
                    }
                }
                i = 0;
                count++;
                break;
        }
    }

    free(line);
    fclose(stream);
    return count;
}

/**
 * Displays the help message to the server.
 */
void display_help(char* name){
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", name);
    printf("  -f question_file      Default to \"qshort.txt\";\n");
    printf("  -i IP_address         Default to \"127.0.0.1\";\n");
    printf("  -p port_number        Default to 25555;\n");
    printf("  -h                    Display this help info.\n");
}

/**
 * Main function that handles all server-side game functionality.
 */
int main(int argc, char* argv[]){

    int opt;
    char* question_file = "qshort.txt";
    char* IP_address = "127.0.0.1";
    int port = 25555;

    // Handles flags from user 
    while((opt = getopt(argc, argv, ":f:i:p:h")) != -1){
        switch(opt){
            case 'f':
                question_file = optarg;
                break;
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
                fprintf(stderr, "Error: Option '-%c' expects an argument.\n", optopt);
                exit(EXIT_FAILURE);
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    int  server_fd;
    int  client_fd;
    char buffer[1024];
    struct sockaddr_in server_addr;
    struct sockaddr_in incoming_msg_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    // Initialize Question Entry struct
    struct Entry arr[50];
    int num = read_questions(arr, question_file);

    /*  STEP 1
        Create and set up a socket
    */
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 2
       Bind the file descriptor with address structure
       so that users can find the address
    */
    bind(server_fd,
         (struct sockaddr *) &server_addr,
         sizeof(server_addr));

    /* STEP 3
       Listen to at most 3 incoming messages from the user
    */
    if (listen(server_fd, MAX_CONNECTIONS) == 0){
        printf("Welcome to 392 Trivia!\n");
    } else {
        fprintf(stderr, "Connection failed.\n");
        exit(EXIT_FAILURE);
    }

    /* STEP 4
       Accept connections from clients
       to enable communication
    */
    int    recvbytes = 0;
    int    max_socket;
    fd_set active_fd_set;

    // Initialize players array
    int players[MAX_CONNECTIONS];
    for (size_t i = 0; i < MAX_CONNECTIONS; i++) players[i] = -1;

    int state = 0;
    int q = 0;
    while(1) {
        // Check question count
        if(state == 1){
            if(q >= num){
                break;
            }

            // Generate and send question to server & clients
            char question[4096];
            sprintf(question, "Question %d: %s1: %s\n2: %s\n3: %s", q+1, arr[q].prompt, arr[q].options[0], arr[q].options[1], arr[q].options[2]);
            printf("%s", question);

            strcpy(question, ""); 
            sprintf(question, "\nQuestion %d: %sPress 1: %s\nPress 2: %s\nPress 3: %s", q+1, arr[q].prompt, arr[q].options[0], arr[q].options[1], arr[q].options[2]);
            for(int i = 0; i < MAX_CONNECTIONS; i++){
                send(players[i], question, strlen(question), 0);
            }
            state++;
            continue;
        }


        // Zero out active fd_set
        FD_ZERO(&active_fd_set);
        // Add server socket to the set
        FD_SET(server_fd, &active_fd_set);
        // Prep for finding max socket to pass to select()
        max_socket = server_fd;

        // Check all fds: if not -1, add to the fd_set
        for (int i = 0; i < MAX_CONNECTIONS; i ++) {
            if (players[i] > -1) {FD_SET(players[i], &active_fd_set);}
            if (players[i] > max_socket) max_socket = players[i];
        }

        // Use select() to actively monitor fds
        int select_ret;
        if ((select_ret = select(max_socket+1, &active_fd_set, NULL, NULL, NULL)) < 0) {
            perror("select()");
            exit(EXIT_FAILURE);
        }

        /* Once there's a fd that's ready,
        meaning we're unblocked from select() function,
        we start accepting connection,
        and add new fd to the set */
        if (FD_ISSET(server_fd, &active_fd_set)) {
            client_fd  = accept(server_fd, (struct sockaddr *) &incoming_msg_addr, &addr_size);

            int j;
            for (j = 0; j < MAX_CONNECTIONS; j ++) {
                if (players[j] == -1) {
                    players[j] = client_fd;
                    break;
                }
            }

            if (j == MAX_CONNECTIONS) {
                close(client_fd);
                fprintf(stderr, "Max connection reached!\n");
            }
            else {
                printf("New connection detected!\n");
            }
        }

        switch(state){
            case 0:
                // Now we're ready to get player names!
                for (int i = 0; i < MAX_CONNECTIONS; i ++) {
                    if (players[i] > -1 && FD_ISSET(players[i], &active_fd_set)) {
                        recvbytes = recv(players[i], buffer, 1024, 0);
                        if (recvbytes == 0) {
                            close(players[i]);
                            FD_CLR(players[i], &active_fd_set);
                            players[i] = -1;
                            printf("Lost connection!\n");
                        }
                        else {
                            buffer[recvbytes - 1] = 0;
                            printf("Hi %s!\n", buffer);
                            strcpy(game[i].name, buffer);
                            game[i].fd = i+3;
                        }
                    }
                }

                // Check if all players are connected
                if ((strcmp(game[0].name, "") != 0) && (strcmp(game[1].name, "") != 0 && (strcmp(game[2].name, "") != 0))){
                    printf("The game starts now!\n");
                    state++;
                }


                break;
            case 2:
                for (int i = 0; i < MAX_CONNECTIONS; i ++) {
                    if (players[i] > -1 && FD_ISSET(players[i], &active_fd_set)) {
                        recvbytes = recv(players[i], buffer, 1024, 0);

                        // Check if player disconnected
                        if (recvbytes == 0) {
                            close(players[i]);
                            FD_CLR(players[i], &active_fd_set);
                            players[i] = -1;
                            printf("Lost connection!\n");
                        } else {

                            // Calcualte scores and go to next question
                            buffer[recvbytes - 1] = 0;
                            char* correct = arr[q].options[arr[q].answer_idx];
                            correct[strlen(correct)] = 0;
                            printf("\n%s answered: %s", game[i].name, arr[q].options[atoi(buffer) - 1]);
                            printf("The correct answer is: %s\n", correct);

                            if (atoi(buffer) == arr[q].answer_idx + 1){
                                game[i].score++;
                            } else {
                                game[i].score--;
                            }
                            printf("Score: %s: %d, %s: %d, %s: %d\n\n", game[0].name, game[0].score, game[1].name, game[1].score, game[2].name, game[2].score);
                            q++;
                            state = 1;
                        }
                    }
                }              
                break;
        }
    }

    // Calculate the winner(s)
    int max_score = max(game[0].score, (max(game[1].score, game[2].score)));
    for(int i = 0; i < MAX_CONNECTIONS; i++){
        if(game[i].score == max_score){
            printf("Congrats, %s!\n", game[i].name);
        }
    }
    printf("Ending....\n");

    // Close all file descriptors
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (players[i] > -1) {
            close(players[i]);
            players[i] = -1;
        }
    }
    close(server_fd);
}
