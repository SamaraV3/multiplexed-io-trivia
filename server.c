//Name: Samara Vassell
//Pledge: I pledge my honor that I have abided by the Stevens Honor System - Samara Vassell

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CONN 2

//structure used to store each question entry
struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

//structure used to store information of each player
struct Player {
    int fd;
    int score;
    char name[128];
};

//function that reads the question database
//returns acc # question entries read from file
int read_questions(struct Entry* arr, char* filename) {
    /*Thought Process:
    variable idx keeps track of position in file 
    do idx % 4 to get the acc position in file: can either amount to
    0 (question), 1 (choices), 2 (correct_choice) or 3 (space seperating questions) 
    do the apropriate action for each
    stop when idx % 4 != 3 BUT the line is empty (or whenever getline returns null)
    to get total # entries divide idx by 4 (ceiling division)
    OR have nother var: entry_idx
    this keeps track of idx in arr array --> starts at 0, +1 every time we reach a non empty idx % 4 == 0 line
    #questions read = entry_idx+1
    entry_idx used to change values in arr*/

    //create file pointer to filename
    FILE* file = fopen(filename, "r");
    if (file==NULL) {fprintf(stderr, "Error: Unable to open file %s\n", filename);exit(EXIT_FAILURE);}
    //file exists
    int idx=0; int entry_idx=0;
    char *line = NULL; size_t len = 0; ssize_t read;
    //reac each line from file
    while((read = getline(&line, &len, file)) != -1) {
        line[strcspn(line, "\n")] = 0; // replace first '\n' with 0
        if (idx % 4 == 0) {//line = question
            strcpy(arr[entry_idx].prompt, line);
        }
        else if (idx % 4 == 1) {//line = choices
            const char delimeter[] = " ";
            //tokenize line 3 times
            strcpy(arr[entry_idx].options[0], strtok(line, delimeter));
            strcpy(arr[entry_idx].options[1], strtok(NULL, delimeter));
            strcpy(arr[entry_idx].options[2], strtok(NULL, delimeter));
        }
        else if (idx % 4 == 2) {//line = correct choicse
            //compare line to each options value
            if (strcmp(arr[entry_idx].options[0], line)==0) {arr[entry_idx].answer_idx=0;}
            else if (strcmp(arr[entry_idx].options[1], line)==0) {arr[entry_idx].answer_idx=1;}
            else {arr[entry_idx].answer_idx=2;}
        }
        else {//line = empty line
            entry_idx++;
        }
        idx++;
    }

    //free line
    free(line);
    fclose(file);
    return entry_idx+1;
}

//function that returns the maximal file descriptor
//AND prepares file desc set
int prep_fds(fd_set* active_fds, int server_fd, int* client_fds) {
    FD_ZERO(active_fds);
    FD_SET(server_fd, active_fds);

    int max_fd = server_fd;

    for (int i=0; i<MAX_CONN; i++) {
        if (client_fds[i] > -1) {FD_SET(client_fds[i], active_fds);}
        if (client_fds[i] > max_fd) {max_fd = client_fds[i];}
    }
    return max_fd;
}

int main(int argc, char* argv[]) {
    int opt;
    char* question_file = "qshort.txt";
    char* ip_address = "127.0.0.1";
    int port_number = 25555;
    char* message;//contains messages sent to clients
    char buffer[1024];//contains messages received from clients
    int recvbytes = 0;// # bytes received from clients
    int gameStarted = 0;//=1 once game has started
    int currentQuestionIndex = 0;//keeps track of index of current question in question_entries
    int awaitingResponse = 0;//=1 once we are awaiting a response from a player/client


    while ((opt = getopt(argc, argv, ":f:i:p:h")) != -1) {
        switch(opt) {
            case 'f':
                //set question_file to optarg
                question_file = optarg;
                break;
            case 'i':
                //set ip_address to optarg
                ip_address = optarg;
                break;
            case 'p':
                //set port_number to int cast of optarg
                port_number = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("%-4s %-20s %s\n", "-f", "question_file", "Default to \"qshort.txt\";");
                printf("%-4s %-20s %s\n", "-i", "IP_address", "Default to \"127.0.0.1\";");
                printf("%-4s %-20s %s\n", "-p", "port_number", "Default to 25555;");
                printf("%-4s %-20s %s\n", "-h", "", "Display this help info.");
                exit(EXIT_SUCCESS);
                break;
            case ':':
                fprintf(stderr, "Error: option needs a value\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                fprintf(stderr, "Error: unknown option '-%c' received\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    //WBA: create server socket
    int server_fd;
    int in_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);
    //Step 1: create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {fprintf(stderr, "Error: Failed to create socket"); exit(EXIT_FAILURE);}
    //Step 2: clear up sockadd_in memory space
    memset(&server_addr, 0, sizeof(server_addr));
    //Step 3: initialize values in object
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    //Step 4: bind socket to server
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {fprintf(stderr, "Error: Failed to bind socket"); exit(EXIT_FAILURE);}
    //Step 5: Listen to socket
    if (listen(server_fd,2) == 0) {printf("Welcome to 392 Trivia!\n");}
    else {fprintf(stderr, "Error: Failed to listen"); exit(EXIT_FAILURE);}

    //Task 2 --> Works correctly
    struct Entry question_entries[50];
    int num_questions = read_questions(question_entries,question_file);

    //create array of players
    struct Player player_infos[MAX_CONN];//currently 2 since MAX_CONN = 2

    //accept connections --> max is 2
    fd_set active_fds;//monitors all file descriptors from client
    int client_fds[MAX_CONN];//stores all file descriptors from client
    int nconn = 0;
    for (size_t i = 0; i < MAX_CONN; i++) {client_fds[i] = -1;}

    while (1) {
        memset(buffer, 0, 1024);//clear buffer
        //Part 1: Prep of file desc set
        int max_fd = prep_fds(&active_fds, server_fd, client_fds);

        //Part 2: Monitor file desc set
        select(max_fd+1, &active_fds, NULL, NULL, NULL);

        //Part 3: Accept new connections
        if (FD_ISSET(server_fd, &active_fds)) {//check if server_fd is ready
            //if so, accept new connections if possible
            in_fd = accept(server_fd, (struct sockaddr*) &in_addr, &addr_size);
            if (nconn == MAX_CONN) {close(in_fd); fprintf(stderr, "Max connection reached!\n");}
            else {
                for (int i = 0; i < MAX_CONN; i++) {
                    if (client_fds[i] == -1) {
                        printf("New connection detected!\n");
                        client_fds[i] = in_fd;
                        message = "Please type your name:\n";
                        send(in_fd, message, strlen(message), 0);
                        memset(buffer, 0, sizeof(buffer));//clear buffer
                        recvbytes = recv(in_fd, buffer, 1024, 0);
                        if (recvbytes == 0) {break;}
                        else {
                            buffer[recvbytes] = 0;//this contains the name
                            player_infos[i].fd = in_fd;
                            player_infos[i].score = 0;
                            strcpy(player_infos[i].name, buffer);
                            printf("Hi %s!\n", player_infos[i].name);
                        }
                        break;
                    }
                }
                nconn++;
            }
        }

        //Part 4: Close connections OR get client responses
        for (int i = 0; i < MAX_CONN; i++) {
            if (client_fds[i] > -1 && \
            FD_ISSET(client_fds[i], &active_fds)) {
                recvbytes = recv(client_fds[i], buffer, 1024, 0);
                if (recvbytes == 0) {//Close connection
                    close(client_fds[i]);
                    FD_CLR(client_fds[i], &active_fds);
                    client_fds[i] = -1;
                    printf("Lost connection!\n");
                    nconn--;
                    //Now we wanna set all other players points to 0, set gameStarted to 0 as well
                    printf("Resetting all points to 0 and restarting game!\n");
                    gameStarted = 0;
                    //iterate over all player_infos != i
                    for (int j=0; j<MAX_CONN; j++) {
                        if (j!=i) {//reset player points to 0 and send message saying game has reset bcuz someone left
                            player_infos[j].score = 0;
                            char* message = "A player has left the game. Your points have been reset until we can restart the game. Hang tight!\n\n";
                            send(player_infos[j].fd, message, strlen(message), 0);
                        }
                    }

                }
                else {//client acc sent a message
                    buffer[recvbytes] = 0;
                    //now ensure the game has alr started AND i acc want an answer
                    if (gameStarted==1 && awaitingResponse==1) {
                        awaitingResponse = 0;//pause listening to other clients
                        //print correct answer to screen
                        int correctIndex = question_entries[currentQuestionIndex].answer_idx;
                        printf("Correct Answer: %s\n\n", question_entries[currentQuestionIndex].options[correctIndex]);
                        //convert the buffer to number
                        char *endptr;
                        long choice = strtol(buffer, &endptr, 10);
                        //check if they acc entered a number
                        if (endptr == buffer) {
                            printf("%s doesn't know how to follow directions BUT their answer is still wrong so -1 points!\n\n", player_infos[i].name);
                            player_infos[i].score = player_infos[i].score - 1;
                        }
                        else if ((int)choice > 3 || (int)choice < 1) {
                            printf("%s doesn't know how to follow directions BUT their answer is still wrong so -1 points!\n\n", player_infos[i].name);
                            player_infos[i].score = player_infos[i].score - 1;
                        }
                        else {
                            //now we compare it to question_entries[currentQuestionIndex].answeridx+1
                            int res = (int) choice;
                            if (question_entries[currentQuestionIndex].answer_idx+1 == res) {
                                //increase score of player
                                player_infos[i].score = player_infos[i].score + 1;
                                printf("%s has answered. Their answer is correct!\n\n", player_infos[i].name);
                            }
                            else {
                                //decrease score of player
                                player_infos[i].score = player_infos[i].score - 1;
                                printf("%s has answered. Their answer is incorrect! HAHA!\n\n", player_infos[i].name);
                            }
                        }
                        //irregardless of what happened above, we send correct answer to all the players' screens
                        char client_message[1024];
                        char points_message[1024];
                        //send questions
                        int written = snprintf(client_message, 1024, "Correct Answer: %s\n\n", question_entries[currentQuestionIndex].options[correctIndex]);
                        if (written < 0) {fprintf(stderr, "Error: snprintf failed"); continue;}
                        else {
                            for (int j=0; j<MAX_CONN; j++) {
                                send(player_infos[j].fd, client_message, strlen(client_message), 0);
                                snprintf(points_message, 1024, "You currently have %d points!\n\n", player_infos[j].score);
                                send(player_infos[j].fd, points_message, strlen(points_message), 0);
                            }
                        }
                        //now increment currentQuestionIndex
                        currentQuestionIndex++;
                    }
                }
            }
        }
    
        //now start the game here
        if (nconn == MAX_CONN && gameStarted == 0) {
            gameStarted = 1;
            currentQuestionIndex = 0;//starts at question_entries[0].prompt
            awaitingResponse = 0; //since we havent printed the acc question yet
            printf("The game starts now!\n");
        }

        //this is where questions are sent to each client
        if (gameStarted==1 && awaitingResponse==0 && currentQuestionIndex<num_questions) {
            //first print on server's screen
            int qNo = currentQuestionIndex+1;
            printf("Question %d: %s\n", qNo, question_entries[currentQuestionIndex].prompt);
            printf("1: %s\n", question_entries[currentQuestionIndex].options[0]);
            printf("2: %s\n", question_entries[currentQuestionIndex].options[1]);
            printf("3: %s\n", question_entries[currentQuestionIndex].options[2]);

            //now send messages to each client_fd
            for (int i=0; i < MAX_CONN; i++) {
                if (client_fds[i] > -1) {
                    char client_message[1024];
                    int qNo = currentQuestionIndex+1;
                    //send questions
                    int written = snprintf(client_message, 1024, "Question %d: %s\n", qNo, question_entries[currentQuestionIndex].prompt);
                    if (written < 0) {fprintf(stderr, "Error: snprintf failed"); continue;}
                    else {send(client_fds[i], client_message, strlen(client_message), 0);}
                    //send option 1
                    written = snprintf(client_message, 1024, "Press 1: %s\n", question_entries[currentQuestionIndex].options[0]);
                    if (written < 0) {fprintf(stderr, "Error: snprintf failed"); continue;}
                    else {send(client_fds[i], client_message, strlen(client_message), 0);}
                    //send option 2
                    written = snprintf(client_message, 1024, "Press 2: %s\n", question_entries[currentQuestionIndex].options[1]);
                    if (written < 0) {fprintf(stderr, "Error: snprintf failed"); continue;}
                    else {send(client_fds[i], client_message, strlen(client_message), 0);}
                    //send option 3
                    written = snprintf(client_message, 1024, "Press 3: %s\n", question_entries[currentQuestionIndex].options[2]);
                    if (written < 0) {fprintf(stderr, "Error: snprintf failed"); continue;}
                    else {send(client_fds[i], client_message, strlen(client_message), 0);}

                }
            }
            //and make awaitingResponse 1
            awaitingResponse=1;
        }

        //and this is where le game ends and we tally up points
        if (gameStarted==1 && currentQuestionIndex==num_questions) {
            //figure out le winner
            int max_score = player_infos[0].score;
            char* winner_name = player_infos[0].name;
            for (int i=1; i<MAX_CONN; i++) {
                if (player_infos[i].score > max_score) {
                    max_score = player_infos[i].score;
                    winner_name = player_infos[i].name;
                }
            }
            printf("Congrats, %s! You've won with %d points!\n", winner_name, max_score);
            //now close all player fds
            for (int i=0; i<MAX_CONN; i++) {
                close(player_infos[i].fd);
            }
            //and exit while loop
            break;
        }
    }
    
    //below is for closing program
    close(server_fd);
    exit(EXIT_SUCCESS);
}