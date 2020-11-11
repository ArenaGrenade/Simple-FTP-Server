#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#ifndef UTILS_H_
#include "utils.h"
#endif

// In all of the following functions that can be used to make the transfer
// request handling easier - a return of -1 or other negative numbers are
// erroraneous executions (could be that the tranfer had failed). Further
// it would return a 0 if there were no errors.


// This is used for sending a string over a connection and then
// a confirmation + error check acknowledgement
//
// Returns -1 if the size transfer itself fails
// Returns -2 if the actual message transfer fails
// Returns 0 if no errors occur
int send_n_ack(int sock_fd, char* msg) {
    int error_code; // This error code is updated and then returned at the end.

    // First send the other side the size of the transfer
    int size_msg = strlen(msg) + 1;
    int converted_size_msg = htonl(size_msg);
    send(sock_fd, &converted_size_msg, sizeof(converted_size_msg), 0);

    // Wait for the other side to repeat and check if the values are correctly transferred
    int size_msg_check;
    recv(sock_fd, &size_msg_check, sizeof(size_msg_check), 0);
    size_msg_check = ntohl(size_msg_check);

    // Return an error code as this first tranfer has failed
    if (size_msg_check != size_msg) error_code = -1;

    // This means that the size message has been transferred successfully
    // So, send the actual message and based on that return an error code
    send(sock_fd, msg, size_msg, 0);

    // Wait for the reieve and then set the error code accordingly
    char msg_check[size_msg];
    recv(sock_fd, &msg_check, sizeof(msg_check), 0);
    if (strcmp(msg, msg_check) == 0) error_code = 0;
    else error_code = -2;

    return error_code;
}

// A function that is used to transfer a number only instead of a string
// and is an overloaded function to the above one
//
// Returns -1 on the error in transmission
// Returns 0 on no errors
int send_n_ack(int sock_fd, int msg) {
    // Convert the message and send it
    int converted_msg = htonl(msg);
    send(sock_fd, &converted_msg, sizeof(converted_msg), 0);

    // Wait for a correctness confirmation ack from the other end
    int msg_chk;
    recv(sock_fd, &msg_chk, sizeof(msg_chk), 0);
    msg_chk = ntohl(msg_chk);
    if (msg_chk != msg) return -1;
    return 0;
}

// A function that is used for recieving any string message
// It is also responsible for letting the other end know that the message
// transfer was succesful or not. It write the given message to the msg
// array that is provided
int recv_msg(int sock_fd, char** msg) {
    // Wait for the other side to send the size of the message to be transferred
    int recv_size;
    recv(sock_fd, &recv_size, sizeof(recv_size), 0);

    // Send the same size over to let the other side handle any possible errors
    send(sock_fd, &recv_size, sizeof(recv_size), 0);
    recv_size = ntohl(recv_size);

    // Change the size of the given message pointer and then read in the message
    char msg_interm[recv_size];
    recv(sock_fd, msg_interm, recv_size, 0);

    // Now send this message over to the other side for ack and check
    send(sock_fd, msg, recv_size, 0);

    *msg = (char*) malloc(recv_size * sizeof(char));
    memset(*msg, 0, recv_size);
    strcpy(*msg, msg_interm);

    return 0;
}

// Another function for recieving number data transfers
// Overrides the above function
int recv_msg(int sock_fd, int* msg) {
    // Wait for a message from the other side
    int converted_msg;
    recv(sock_fd, &converted_msg, sizeof(converted_msg), 0);

    // Send this message over to the other side for ack and check
    send(sock_fd, &converted_msg, sizeof(converted_msg), 0);

    *msg = ntohl(converted_msg);
    return 0;
}

// This funcntion is used when a turn-around of data flow is needed
// preferrable it would be used when the A is performing 2 sends to B and for
// this B would recieve twice. After this if we want B to send message back 
// to A then B and A would both perform a handover that would immediately switch
// the data flow. Role for a previous sender should be 1 and a previous
// reciever should pass 0 as the role.
//
// Role is 1 for a previous sender
// Role is 0 for a previous reciever
int hand_over(int sock_fd, int role) {
    int number = 1;
    if (role == 1) {
        send(sock_fd, &number, sizeof(number), 0);
        return 0;
    } else if (role == 0) {
        recv(sock_fd, &number, sizeof(number), 0);
        return 0;
    } else {
        return -1;
    }
}