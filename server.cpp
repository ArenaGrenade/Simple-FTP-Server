#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef UTILS_H_
#include "utils.h"
#endif

#define FILE_STORE "./server/"
#define READ_BUFFER_SIZE 1024

void signal_handler(int signal) {
    printf("The client has yeeted\n");
    raise(SIGKILL);
}

void server_get(int sock_fd_conn) {
    // Get the number of files to transfer
    int files_count;
    recv_msg(sock_fd_conn, &files_count);
    
    // Data structures need for the transfer
    int client_error;
    char* filename;
    char filepath[1024];

    // Data related to the file itself
    int file_fd;
    int filesize;
    int perfect_sized_blocks;
    int perfect_blocks_size;
    int imperfect_blocks_size;
    int block_count;
    int read_blocks_size;

    // Block read data structures
    char* fileblock;

    // Loop on the count of files and send them over after getting
    // the file names from the client
    for (int i = 0; i < files_count; i++) {
        // Get the number of files to download
        recv_msg(sock_fd_conn, &filename);

        // Check if the write file can be opened at the client side
        client_error = 1;
        recv_msg(sock_fd_conn, &client_error);
        if (client_error) continue;

        // Accept the hand over
        hand_over(sock_fd_conn, 0);

        // Check if the file exists and then send confirmation over
        sprintf(filepath, "%s%s", FILE_STORE, filename);
        if (access(filepath, R_OK) < 0) {
            printf("Skipping the file named %s\n", filename);
            send_n_ack(sock_fd_conn, 1);
            continue;
        }
        send_n_ack(sock_fd_conn, 0);

        // Now open the file and setup the descriptors and send over the file
        // blocks to the client.
        // We are sending a 0 over to the client if the file has been opened
        // Else we would send a 1 and continue, this needs be handled by the 
        // client.
        if((file_fd = open(filepath, O_RDONLY)) < 0) {
            printf("Failure in accesing the file named %s\n", filename);
            send_n_ack(sock_fd_conn, 1);
            continue;
        }
        send_n_ack(sock_fd_conn, 0);

        // Get the file size and calculate the number of blocks to read
        filesize = lseek(file_fd, 0, SEEK_END);

        perfect_sized_blocks = filesize / READ_BUFFER_SIZE;
        perfect_blocks_size = perfect_sized_blocks * READ_BUFFER_SIZE;
        // imperfect_blocks_size = filesize - perfect_blocks_size - 1;
        imperfect_blocks_size = filesize - perfect_blocks_size;
        block_count = ((imperfect_blocks_size > 0)?perfect_sized_blocks + 1:perfect_sized_blocks);

        // Sets the initial read block size to that of the imperfect_blocks_size or normal
        read_blocks_size = ((imperfect_blocks_size > 0)?imperfect_blocks_size:READ_BUFFER_SIZE);

        // Send the number of blocks to be transferred over to the client.
        send_n_ack(sock_fd_conn, block_count);

        // reset the fd to the start of the file
        lseek(file_fd, 0, SEEK_SET);

        // Run through the file and transfer the corresponding block
        for (int i = 0; i < block_count; i++) {
            // Send the size of the block over
            send_n_ack(sock_fd_conn, read_blocks_size);

            // read the block from file
            fileblock = (char*) malloc(read_blocks_size);
            read(file_fd, fileblock, read_blocks_size);

            // Send it over to the client
            send_n_ack(sock_fd_conn, fileblock);

            // This is done to ensure that post the first block read everything
            // else is of the right transfer block size
            read_blocks_size = READ_BUFFER_SIZE;
        }

        // Close the fd opened for reading the file
        close(file_fd);

        // Clean up all the buffers and data structures
        bzero(filepath, 1024);
    }
}

void server_ls(int sock_fd_conn) {
    // Take control of the data flow
    hand_over(sock_fd_conn, 0);

    DIR *d;
    struct dirent *dir;

    // Count the number of files in the directory
    d = opendir(FILE_STORE);
    int file_count = 0;
    while((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        file_count++;
    }
    closedir(d);

    // Send the files count over to the client
    send_n_ack(sock_fd_conn, file_count);

    d = opendir(FILE_STORE);
    char done_buf[5];
    // Loop through the directory node and send the filenames to client
    while((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        send_n_ack(sock_fd_conn, dir->d_name);
    }
    closedir(d);

    // Return data flow control to the client
    hand_over(sock_fd_conn, 1);
}

int main(int argc, char **argv) {
    signal(SIGPIPE, signal_handler);

    struct sockaddr_in server, client;

    // Create a socket using the socket system call
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket Creation has Failed");
        exit(EXIT_FAILURE);
    }
    
    // Used to skip the "Address already in use error" message
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // argv[1] or the first command line argument is the port of the server
    memset(&server, '0', sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[2]));

    // Perform a bind for the properties of the socket to the object itself.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listens on the given port
    if (listen(sock_fd, 1) < 0) {
        perror("Unable to listen of given port");
        exit(EXIT_FAILURE);
    }

    printf("Server is running at %s:%d\n", argv[1], (int) ntohs(server.sin_port));

    // Actually confirms the connection
    socklen_t addrlen = sizeof(client);
    int sock_fd_conn = accept(sock_fd, (struct sockaddr *)&client, &addrlen);

    printf("A client has connected to the server\n");

    // Run the server from here.
    // Data structures for the server
    char* command;

    // Infinite loop to run the server.
    while(true) {
        // Synchronization information sent to the client
        send_n_ack(sock_fd_conn, (char*)"sentinel");

        // recieve command request type here
        recv_msg(sock_fd_conn, &command);

        if (strcmp(command, "ls") == 0) {
            server_ls(sock_fd_conn);
        } else if (strcmp(command, "get") == 0) {
            server_get(sock_fd_conn);
        } else if (strcmp(command, "help") == 0) {
            ; // Just ignore help command
        } else if (strcmp(command, "quit") == 0) {
            raise(SIGPIPE);
        } else {
            printf("Command not found\n");
        }
    }

    return 0;
}
