#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "can.h"

struct systemArgs {
    int fd;
    int CTHfd;
    pthread_mutex_t *mutex;
};

int CTHconnection = 0;

int* healthCheck (int fd, int CTHfd, pthread_mutex_t *mutex);
void *CTHtransmitReceive (void *args);
void *systemTransmit(void *args);
void *systemReceive(void *args);

int main () {

    int fd0, fd1;
    int rct, rcr, rcc;
    int *systemStatus;
    pthread_mutex_t systemMutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t transmitThread, receiveThread, CTHThread;
    struct systemArgs *transmitArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));
    struct systemArgs *receiveArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));
    struct systemArgs *CTHArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));

    socket_initiation("vcan0", &fd0);
    socket_initiation("vcan1", &fd1);
    systemStatus = healthCheck(fd0, fd1, &systemMutex);

    transmitArgs->fd = fd0;
    transmitArgs->CTHfd = fd1;
    transmitArgs->mutex = &systemMutex;

    receiveArgs->fd = fd0;
    receiveArgs->CTHfd = fd1;
    receiveArgs->mutex = &systemMutex;

    CTHArgs->fd = fd0;
    CTHArgs->CTHfd = fd1;
    CTHArgs->mutex = &systemMutex;

    if ( (rct = pthread_create(&transmitThread, NULL, systemTransmit, (void *) transmitArgs)) ) printf("Thread creation failed: %d\n", rct);
    if ( (rcr = pthread_create(&receiveThread, NULL, systemReceive, (void *) receiveArgs)) ) printf("Thread creation failed: %d\n", rcr);
    if ( (rcc = pthread_create(&CTHThread, NULL, CTHtransmitReceive, (void *) CTHArgs)) ) printf("Thread creation failed: %d\n", rcc);

    pthread_join(transmitThread, NULL);
    pthread_join(receiveThread, NULL);
    pthread_join(CTHThread, NULL);

    socket_close(fd0);
    socket_close(fd1);

}

int* healthCheck (int fd, int CTHfd, pthread_mutex_t *mutex) {

    struct can_frame frame;
    int systemStatus[6]; // CDH_ERROR, EPS_ERROR, SOLAR_ERROR, ACS_ERROR, ADS_ERROR, CTH_ERROR

    frame.can_id = 0x010;
    frame.can_dlc = 1;
    frame.data[0] = 0x001;

    socket_write(fd, &frame);
    socket_write(CTHfd, &frame);

    fd_set readfd;
    int selected;
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};

    for (int i = 0; i < 5; i++) {

        pthread_mutex_lock(mutex);
    
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);

        selected = select(fd + 1, &readfd, NULL, NULL, &tv);

        if (selected == -1) {
            
            for (int i = 0; i < 6; i++) systemStatus[i] = 0;
            return systemStatus;

        } else if (selected > 0) {

            socket_read(fd, &frame);
            printCANframe(frame);

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

        }
    
        FD_ZERO(&readfd);
        FD_SET(CTHfd, &readfd);

        selected = select(CTHfd + 1, &readfd, NULL, NULL, &tv);

        if (selected == -1) {
        
            pthread_mutex_unlock(mutex);    
        
        } else if (selected > 0) {

            socket_read(CTHfd, &frame);
            printCANframe(frame);
            pthread_mutex_unlock(mutex);

            if (frame.can_id == 0x181) systemStatus[5] = frame.data[0];

        } else {

            pthread_mutex_unlock(mutex);

        }

    }

    return systemStatus;
}

void *CTHtransmitReceive (void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    struct can_frame frame;
    
    while (1) {

        if (CTHconnection == 1) {

            pthread_mutex_lock(systemArgs->mutex);

            frame.can_id = 0x381;   
            frame.can_dlc = 1;
            frame.data[0] = 0x001;

            socket_write(systemArgs->CTHfd, &frame);

            do {
                
                socket_read(systemArgs->CTHfd, &frame);
                printCANframe(frame);


            } while (frame.can_dlc != 1 || frame.data[0] != 0x001);

            CTHconnection = 0;

            pthread_mutex_unlock(systemArgs->mutex);
        }

    }
}

void *systemTransmit(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    struct can_frame frame;

    while (1) {

        for (int i = 0; i < 5; i++) {

            pthread_mutex_lock(systemArgs->mutex);

            frame.can_id = 0x020;   
            frame.can_dlc = 1;
            frame.data[0] = 0x001;

            socket_write(systemArgs->fd, &frame);

            pthread_mutex_unlock(systemArgs->mutex);

            sleep(5);

        }

        CTHconnection = 1;

    }

}

void *systemReceive(void *args) {

    fd_set readfd;
    int selected;
    struct can_frame frame;
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};
    struct systemArgs *systemArgs = (struct systemArgs*)args;

    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        FD_ZERO(&readfd);
        FD_SET(systemArgs->fd, &readfd);

        selected = select(systemArgs->fd + 1, &readfd, NULL, NULL, &tv);

        if (selected == -1) {
            perror("Select error");
            pthread_mutex_unlock(systemArgs->mutex);
        } else if (selected > 0) {
            socket_read(systemArgs->fd, &frame);
            printCANframe(frame);
            pthread_mutex_unlock(systemArgs->mutex);
        } else {
            pthread_mutex_unlock(systemArgs->mutex);
        }

    }

}