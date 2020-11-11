#ifndef UTILS_H_
#define UTILS_H_
int send_n_ack(int sock_fd, char* msg);
int send_n_ack(int sock_fd, int msg);
int recv_msg(int sock_fd, char** msg);
int recv_msg(int sock_fd, int* msg);
int hand_over(int sock_fd, int role);
#endif
