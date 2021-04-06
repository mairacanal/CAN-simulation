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
void *systemTransmit(void *args);
void *systemReceive(void *args);

int main () {

    int s, rc;
    struct can_frame transmitFrame;
    pthread_mutex_t systemCommunicationMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t systemThread;
    struct systemArgs *transmitArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));
    struct systemArgs *receiveArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));

    socket_initiation("vcan0", &s);
    healthCheck(s, &systemCommunicationMutex);

    transmitArgs->file_descriptor = s;
    transmitArgs->mutex = &systemCommunicationMutex;
    transmitArgs->frame = &transmitFrame;

    if ( (rc = pthread_create(&systemThread, NULL, systemReceive, (void *) transmitArgs) ) ) {

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

        frame.can_id = 0xFF;
        frame.can_dlc = 1;
        frame.data[0] = 0x01; // Here, the system makes tests and return if the ACS is OK

        socket_write(file_descriptor, &frame);

    }

    pthread_mutex_unlock(mutex); 

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

    }

}

void *systemReceive(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    
    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        socket_read(systemArgs->file_descriptor, systemArgs->frame);
        printCANframe(*(systemArgs->frame));
        
        if (systemArgs->frame->can_id == 0x02) {

            systemArgs->frame->can_id = 0x03;   
            systemArgs->frame->can_dlc = 1;
            systemArgs->frame->data[0] = 0x7B;
            
            socket_write(systemArgs->file_descriptor, systemArgs->frame);
        
        }

        pthread_mutex_unlock(systemArgs->mutex);

    }

}