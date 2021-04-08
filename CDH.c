#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "can.h"

struct systemArgs {
    int file_descriptor;
    int CTH_file_descriptor;
    pthread_mutex_t *mutex;
    struct can_frame *frame;
};

int* healthCheck (int file_descriptor, int CTH_file_descriptor, pthread_mutex_t *mutex);
void *systemTransmit(void *args);
void *systemReceive(void *args);

int main () {

    int s, t;
    int rct, rcr;
    int *systemStatus;
    pthread_mutex_t systemCommunicationMutex = PTHREAD_MUTEX_INITIALIZER;

    struct can_frame transmitFrame, receiveFrame;
    pthread_t systemTThread, systemRThread;
    struct systemArgs *transmitArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));
    struct systemArgs *receiveArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));

    socket_initiation("vcan0", &s);
    socket_initiation("vcan1", &t);
    systemStatus = healthCheck(s, t, &systemCommunicationMutex);

    transmitArgs->file_descriptor = s;
    transmitArgs->CTH_file_descriptor = t;
    transmitArgs->mutex = &systemCommunicationMutex;
    transmitArgs->frame = &transmitFrame;

    receiveArgs->file_descriptor = s;
    receiveArgs->CTH_file_descriptor = t;
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
    socket_close(t);

}

int* healthCheck (int file_descriptor, int CTH_file_descriptor, pthread_mutex_t *mutex) {

    struct can_frame frame;
    int systemStatus[6]; // CDH_ERROR, EPS_ERROR, SOLAR_ERROR, ACS_ERROR, ADS_ERROR, CTH_ERROR

    frame.can_id = 0x010;
    frame.can_dlc = 1;
    frame.data[0] = 0x001;

    socket_write(file_descriptor, &frame);
    socket_write(CTH_file_descriptor, &frame);

    fd_set readfd;
    int selected;
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};

    for (int i = 0; i < 5; i++) {

        pthread_mutex_lock(mutex);
    
        FD_ZERO(&readfd);
        FD_SET(file_descriptor, &readfd);

        selected = select(file_descriptor + 1, &readfd, NULL, NULL, &tv);

        if (selected == -1) {
            
            pthread_mutex_unlock(mutex);
            for (int i = 0; i < 6; i++) systemStatus[i] = 0;
            return systemStatus;

        } else if (selected > 0) {

            socket_read(file_descriptor, &frame);
            printCANframe(frame);
            pthread_mutex_unlock(mutex);

            switch (frame.can_id) {
                case 0x141:
                    systemStatus[1] = frame.data[0];
                    break;                
                case 0x151:
                    systemStatus[2] = frame.data[0];
                    break;
                case 0x161:
                    systemStatus[3] = frame.data[0];
                    break;
                case 0x171:
                    systemStatus[4] = frame.data[0];
                    break;
            }

        } else {
            pthread_mutex_unlock(mutex);
        }

        pthread_mutex_lock(mutex);
    
        FD_ZERO(&readfd);
        FD_SET(CTH_file_descriptor, &readfd);

        selected = select(CTH_file_descriptor + 1, &readfd, NULL, NULL, &tv);

        if (selected == -1) {
        
            pthread_mutex_unlock(mutex);    
        
        } else if (selected > 0) {

            socket_read(CTH_file_descriptor, &frame);
            printf("");
            printCANframe(frame);
            pthread_mutex_unlock(mutex);

            if (frame.can_id == 0x181) systemStatus[5] = frame.data[0];

        } else {

            pthread_mutex_unlock(mutex);

        }

    }

    return systemStatus;
}

void *systemTransmit(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    
    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        systemArgs->frame->can_id = 0x020;   
        systemArgs->frame->can_dlc = 1;
        systemArgs->frame->data[0] = 0x001;

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
            perror("Select error");
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