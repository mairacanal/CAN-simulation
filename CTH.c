#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "can.h"

struct systemArgs {
    int file_descriptor;
    pthread_mutex_t *mutex;
    struct can_frame *frame;
};

void healthCheck(int file_descriptor, pthread_mutex_t *mutex);
void *systemReceive(void *args);

int main () {

    int s, rc;
    struct can_frame transmitFrame;
    pthread_mutex_t systemCommunicationMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t systemThread;
    struct systemArgs *receiveArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));

    socket_initiation("vcan0", &s);
    healthCheck(s, &systemCommunicationMutex);

    receiveArgs->file_descriptor = s;
    receiveArgs->mutex = &systemCommunicationMutex;
    receiveArgs->frame = &transmitFrame;

    if ( (rc = pthread_create(&systemThread, NULL, systemReceive, (void *) receiveArgs) ) ) {

        printf("Thread creation failed: %d\n", rc);

    }

    pthread_join(systemThread, NULL);
    socket_close(s);

}

void healthCheck(int file_descriptor, pthread_mutex_t *mutex) {

    struct can_frame frame;
    
    pthread_mutex_lock(mutex);

    socket_read(file_descriptor, &frame);
    printCANframe(frame);

    if (frame.can_id == 0x01) {

        frame.can_id = 0x80;
        frame.can_dlc = 1;
        frame.data[0] = 0x01; // Here, the system makes tests and return if the ACS is OK

        socket_write(file_descriptor, &frame);

    }

    pthread_mutex_unlock(mutex); 

}

void *systemReceive(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    
    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        socket_read(systemArgs->file_descriptor, systemArgs->frame);
        printCANframe(*(systemArgs->frame));

        pthread_mutex_unlock(systemArgs->mutex);

    }

}