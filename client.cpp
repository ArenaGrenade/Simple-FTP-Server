#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>

#ifndef UTILS_H_
#include "utils.h"
#endif

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"

#define FILE_STORE "./client/"

using namespace std;


void get(int argc, char **argv, int sock_fd) {
    if (argc < 2) {
        printf("Provide a file name to fetch\n");
        send_n_ack(sock_fd, (char*) "fail");
        return;
    }

    // Send the command type to the server
    send_n_ack(sock_fd, (char*) "get");
    
    // Send the count of files to send to the server
    send_n_ack(sock_fd, argc - 1);

    // Data structures for the transfer
    int server_error = 1;
    int file_blocks;
    char* fileblock;
    int block_read_size;

    // Time tracking structures
    std::chrono::time_point<std::chrono::system_clock> start, now, end;
    std::time_t start_time, now_time, end_time;
    std::chrono::duration<double> elapsed_seconds;

    // Write file related data structures
    int file_fd;
    char filepath[1024];

    // Loop through the files and send the names over and recieve the files itself
    for (int i = 1; i < argc; i++) {
        // Send over file name
        send_n_ack(sock_fd, argv[i]);

        // Open the file to write the data into, if not there then create the file
        sprintf(filepath, "%s%s", FILE_STORE, argv[i]);
        if((file_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
            printf("\u001b[31mFailure in accesing the file named %s.\u001b[0m\n", argv[i]);
            send_n_ack(sock_fd, 1);
            continue;
        }
        send_n_ack(sock_fd, 0);

        // Handover control
        hand_over(sock_fd, 1);

        // Check if the file exists on the server
        server_error = 1;
        recv_msg(sock_fd, &server_error);
        if (server_error) {
            // This means that the file does not exist on the server.
            // We need to delete the file that we had previously created.
            remove(filepath);
            printf("\u001b[31mFile %s does not exist on the server and is skipped.\u001b[0m\n", argv[i]);
            continue;
        }
        // Set current time_point as initial
        start = std::chrono::system_clock::now();
        start_time = std::chrono::system_clock::to_time_t(start);

        // If the file is existing then only tranfer the file
        printf("\u001b[34;1m%.24s\u001b[0m | Transfering %s: 0.00%% complete\r", std::ctime(&start_time), argv[i]);

        // Recieve the error for when the file is un-openeable at the server for
        // some reason - could be due to file being occupied
        server_error = 1;
        recv_msg(sock_fd, &server_error);
        if (server_error) continue;
        // If we get this far it means that the file has been opened at the server
        // and can be transferred now.

        // Get the total number of blocks to transfer
        recv_msg(sock_fd, &file_blocks);

        // Loop through the number of blocks and transfer them over
        for (int j = 0; j < file_blocks; j++) {
            // Read the block size from the server
            recv_msg(sock_fd, &block_read_size);

            // Recieve the file block itself from the server.
            recv_msg(sock_fd, &fileblock);

            // Write the file block to a physical file.
            write(file_fd, fileblock, block_read_size);

            // Put the current time_point
            now = std::chrono::system_clock::now();
            now_time = std::chrono::system_clock::to_time_t(now);

            // Print progress information
            printf("\u001b[34;1m%.24s\u001b[0m | Transfering %s: %.2f%% complete |-", std::ctime(&now_time), argv[i], (float) j * 100 / file_blocks);
            // Print progress bar
            for (int i = 0; i < 25; i++) {
                if (i < (j * 25 / file_blocks)) printf("\u001b[42m \u001b[0m");
                else printf("\u001b[40;1m \u001b[0m");
            }
            printf("-|\r");
        }

        // Print transfer complete information
        end = std::chrono::system_clock::now();
        end_time = std::chrono::system_clock::to_time_t(end);
        elapsed_seconds = end - start;
        printf("\u001b[34;1m%.24s\u001b[0m | \u001b[32;1mTransfering %s: has been completed.\u001b[0m Elapsed time: %.4F seconds\n", std::ctime(&end_time), argv[i], elapsed_seconds.count());

        // Close the write file fd
        close(file_fd);

        // Revert all the buffers to 0
        bzero(filepath, 1024);
    }
}


void ls(int argc, char **argv, int sock_fd) { 
    // Send the command to the server
    send_n_ack(sock_fd, (char*) "ls");

    // Hand over control to the server
    hand_over(sock_fd, 1);

    // Recive the files count from the server
    int file_count;
    recv_msg(sock_fd, &file_count);
    printf("There are %d files present at the server\n", file_count);

    // Define the storage for file name recieved from server
    char* ls_buffer;
    // Loop through the file count range and recive the filenames from server
    for (int i = 0; i < file_count; i++) {
        recv_msg(sock_fd, &ls_buffer);
        printf("%s\n", ls_buffer);
    }

    // Take over control of the dataflow
    hand_over(sock_fd, 0);
}


void help(int sock_fd) {
    send_n_ack(sock_fd, (char*) "help");
    printf("Hello, User!!\n\nThis is the client interface for the AGServer.\n");
    printf("You can run a \u001b[40;1m\u001b[35;1mget file_name\u001b[0m to get that file from the server\n");
    printf("Run a \u001b[40;1m\u001b[35;1mquit\u001b[0m or \u001b[40;1m\u001b[35;1mexit\u001b[0m to quit the client.\n\n");
    printf("To invoke this help statements, run a \u001b[40;1m\u001b[35;1mhelp\u001b[0m command\n");
}


void quit(int sock_fd) {
    send_n_ack(sock_fd, (char*) "quit");
    printf("\n\u001b[31mQuitting now\u001b[0m\n");
    exit(EXIT_SUCCESS);
}

// Command runner funtion - looks into the available functions and runs them accordingly
void run_command(int argc, char **argv, int sock_fd) {
    if (argc == 0) {
        printf("There are no commands found\n");
        return;
    }
    if (strcmp(argv[0], "help") == 0) help(sock_fd);
    else if (strcmp(argv[0], "quit") == 0) quit(sock_fd);
    else if (strcmp(argv[0], "exit") == 0) quit(sock_fd);
    else if (strcmp(argv[0], "ls") == 0) ls(argc, argv, sock_fd);
    else if (strcmp(argv[0], "get") == 0) get(argc, argv, sock_fd);
    else {
        printf("Command not found\n");
        send_n_ack(sock_fd, (char *) "fail");
    }
}


// The tokenizer - splits the command into tokens
char** tokenize(int* cmd_cnt, char* cmd) {
	char* token;
	int tok_bufsize = 64, index = 0;
	char** tokens = (char**) malloc(tok_bufsize * sizeof(char*));

	if (!tokens) {
		printf("Memory allocation error.\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(cmd, TOK_DELIM);
	while (token != NULL) {
		tokens[index++] = token;

		if (index >= tok_bufsize) {
			tok_bufsize += TOK_BUFSIZE;
			tokens = (char**) realloc(tokens, tok_bufsize * sizeof(char*));
			if (!tokens) {
				printf("Memory allocation error.\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, TOK_DELIM);
	}
	tokens[index] = NULL;
	*cmd_cnt = index;
	return tokens;
}

// Prompts the user for a response.
char** prompt(int* tok_len, int sock_fd) {
    char* line = NULL;
    size_t read_bufsize = 0;

    if(getline(&line, &read_bufsize, stdin) == -1) {
		if(feof(stdin)) {
			printf("\nEOF character detected exiting...\n");
			quit(sock_fd);
			exit(EXIT_FAILURE);
		}
		perror("Prompt Readline Error");
		exit(EXIT_FAILURE);
	}

    char** tokens;

    tokens = tokenize(tok_len, line);

    return tokens;
}

int main(int argc, char **argv) {
    struct sockaddr_in server;

    // Create a socket using the socket system call
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket Creation has Failed");
        exit(EXIT_FAILURE);
    }

    // argv[1] or the first command line argument is the port of the server to connect to
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = 0;
    if(inet_pton(AF_INET, argv[1], &server.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server at the address and port
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        exit(EXIT_FAILURE);
    }

    printf("Client connected to server %s:%d\n", inet_ntoa(server.sin_addr), (int) ntohs(server.sin_port));

    // Data structures used for the UI
    char* sync_buf;
    char** tokens;
    int token_count;

    recv_msg(sock_fd, &sync_buf);
    help(sock_fd);

    while(true) {
        recv_msg(sock_fd, &sync_buf);

        printf("Client> ");
        tokens = prompt(&token_count, sock_fd);

        run_command(token_count, tokens, sock_fd);
    }

    return 0;
}