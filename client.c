//Name: Samara Vassell
//Pledge: I pledge my honor that I have abided by the Stevens Honor System - Samara Vassell

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void parse_connect(int argc, char** argv, int* server_fd) {
    //below parses input
    int opt;
    char* ip_address = "127.0.0.1";
    int port_number = 25555;
    while ((opt = getopt(argc, argv, ":i:p:h")) != -1) {
        switch(opt) {
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
                printf("%-4s %-20s %s\n", "-i", "IP_address", "Default to \"127.0.0.1\";");
                printf("%-4s %-20s %s\n", "-p", "port_number", "Default to 25555;");
                printf("%-4s %-20s %s\n", "-h", "", "Display this help info.");
                return ;
            case ':':
                fprintf(stderr, "Error: option needs a value\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                fprintf(stderr, "Error: unknown option '-%c' received\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    //and below acc creates client socket
    struct sockaddr_in server_addr;
    //socklen_t addr_size;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);

    //this connects to server
    if (connect(*server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Connect");
        //fprintf(stderr, "Error: Failed to connect");
        exit(EXIT_FAILURE);
    }
    
}


int main(int argc, char* argv[]) {
    //char* message;//contains messages sent to server
    char buffer[1024];//contains messages received from server
    int recvbytes = 0;// # bytes received from server
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {fprintf(stderr, "Error: Failed to create socket"); exit(EXIT_FAILURE);}
    parse_connect(argc, argv, &server_fd);
    fd_set active_fds;//monitors all file descriptors from client
    int max_fd;
    while (1) {
        memset(buffer, 0, 1024);//clear buffer
        //Part 1: Prep of file desc set
        FD_ZERO(&active_fds);
        FD_SET(0, &active_fds);//for stdin
        FD_SET(server_fd, &active_fds); //for connecting to server
        if (server_fd > 0) {max_fd = server_fd;}
        else {max_fd = 0;}

        //Part 2: monitor file desc set
        select(max_fd+1, &active_fds, NULL, NULL, NULL);

        //Part 3: Check if client vs server sent smthn
        if (FD_ISSET(0, &active_fds)) {//client types smthn into stdin
            if (fgets(buffer, 1024, stdin) != NULL) {//aka we receive smthn
                buffer[strcspn(buffer, "\n")] = 0;//remove newline
                //send input to server
                send(server_fd, buffer, strlen(buffer), 0);
            }
        }

        if (FD_ISSET(server_fd, &active_fds)) {//server sent smthn
            recvbytes = recv(server_fd, buffer, 1024, 0);
            if (recvbytes <= 0) {printf("Server closed connection.\n"); break;}
            //otherwise null terminate the message
            buffer[recvbytes] = 0;
            printf("%s", buffer);

        }

    }
    
    //below is for closing program
    close(server_fd);
    exit(EXIT_SUCCESS);
}