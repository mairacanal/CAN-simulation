/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ADS (Attitude Determination System)                                             *
 * Author: Maíra Canal (@mairacanal)                                               *
 * São Carlos School of Engineering - University of São Paulo                      *
 * Abril/2021                                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <pthread.h>
#include "can.h"

struct systemArgs {
    int fd;
    pthread_mutex_t *mutex;
};

void healthCheck(int fd, pthread_mutex_t *mutex);
void *systemReceive(void *args);

int main () {

    int s, rc;
    pthread_t systemThread;
    pthread_mutex_t systemMutex = PTHREAD_MUTEX_INITIALIZER;
    struct systemArgs *receiveArgs = (struct systemArgs *)malloc(sizeof(struct systemArgs));

    socket_initiation("vcan0", &s);
    healthCheck(s, &systemMutex);

    receiveArgs->fd = s;
    receiveArgs->mutex = &systemMutex;

    if ( (rc = pthread_create(&systemThread, NULL, systemReceive, (void *) receiveArgs) ) ) {

        printf("Thread creation failed: %d\n", rc);

    }

    pthread_join(systemThread, NULL);

    socket_close(s);

}

void healthCheck(int fd, pthread_mutex_t *mutex) {

    struct can_frame frame;
    
    pthread_mutex_lock(mutex);

    socket_read(fd, &frame);
    printCANframe(frame);

    if (frame.can_id == 0x010) {

        frame.can_id = 0x171;
        frame.can_dlc = 1;
        frame.data[0] = 0x001; // Here, the system makes tests and return if the ADS is OK

        socket_write(fd, &frame);

    }

    pthread_mutex_unlock(mutex);

}

void *systemReceive(void *args) {

    struct systemArgs *systemArgs = (struct systemArgs*)args;
    struct can_frame frame;
    
    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        socket_read(systemArgs->fd, &frame);
        printCANframe(frame);
        
        if (frame.can_id == 0x020) {

            frame.can_id = 0x271;   
            frame.can_dlc = 8;
            frame.data[0] = 0x6B;
            frame.data[1] = 0x01;
            frame.data[2] = 0x7B;
            frame.data[3] = 0xAF;
            frame.data[4] = 0x74;
            frame.data[5] = 0x9A;
            frame.data[6] = 0x05;
            frame.data[7] = 0x2B;
            
            socket_write(systemArgs->fd, &frame);

            frame.can_id = 0x272;   
            socket_write(systemArgs->fd, &frame);

            frame.can_id = 0x273;   
            socket_write(systemArgs->fd, &frame);

        }

        pthread_mutex_unlock(systemArgs->mutex);

    }

}