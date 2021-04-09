#ifndef LINUX_CAN_H
#define LINUX_CAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int socket_initiation(char *, int *);

int socket_read(int, struct can_frame *);

int socket_write (int, struct can_frame *);

void printCANframe (struct can_frame);

int socket_close(int);

#endif 