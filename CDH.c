#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "can.h"

struct systemArgs {
    int file_descriptor;
    pthread_mutex_t *mutex;
    struct can_frame *frame;
};

void *systemTransmit(void *args);
void *systemReceive(void *args);

int main () {

    int s, rct, rcr;
    struct can_frame transmitFrame, receiveFrame;
    pthread_mutex_t systemCommunicationMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t systemTThread, systemRThread;
    struct systemArgs *transmitArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));
    struct systemArgs *receiveArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));

    socket_initiation("vcan0", &s);

    transmitArgs->file_descriptor = s;
    transmitArgs->mutex = &systemCommunicationMutex;
    transmitArgs->frame = &transmitFrame;

    receiveArgs->file_descriptor = s;
    receiveArgs->mutex = &systemCommunicationMutex;
    receiveArgs->frame = &receiveFrame;

    if ( (rct = pthread_create(&systemTThread, NULL, systemTransmit, (void *) transmitArgs) ) ) {

        printf("Thread creation failed: %d\n", rct);

    }

    if ( (rcr = pthread_create(&systemRThread, NULL, systemReceive, (void *) receiveArgs) ) ) {

        printf("Thread creation failed: %d\n", rcr);

    }

    pthread_join(systemTThread, NULL);
    pthread_join(systemRThread, NULL);

    socket_close(s);

}

void *systemTransmit(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    
    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        systemArgs->frame->can_id = 0x01;   
        systemArgs->frame->can_dlc = 1;
        systemArgs->frame->data[0] = 0x01;

        socket_write(systemArgs->file_descriptor, systemArgs->frame);

        pthread_mutex_unlock(systemArgs->mutex);

        sleep(5);

    }

}

void *systemReceive(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    fd_set readfd;
    int selected;
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};

    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        FD_ZERO(&readfd);
        FD_SET(systemArgs->file_descriptor, &readfd);

        selected = select(systemArgs->file_descriptor + 1, &readfd, NULL, NULL, &tv);

        if (selected == -1) {
            perror("Select error: ");
            pthread_mutex_unlock(systemArgs->mutex);
        } else if (selected > 0) {
            socket_read(systemArgs->file_descriptor, systemArgs->frame);
            printCANframe(*(systemArgs->frame));
            pthread_mutex_unlock(systemArgs->mutex);
        } else {
            pthread_mutex_unlock(systemArgs->mutex);
        }

    }

}