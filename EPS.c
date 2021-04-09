/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * EPS (Energy Power System)                                                   *
 * Author: Maíra Canal (@mairacanal)                                               *
 * São Carlos School of Engineering - University of São Paulo                      *
 * Abril/2021                                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <pthread.h>
#include "can.h"
#include "CAN_ID_map.h"

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

    if (frame.can_id == CDH_HC_REQ) {

        frame.can_id = EPS_HC_RES;
        frame.can_dlc = 1;
        frame.data[0] = 0x001; // Here, the system makes tests and return if the EPS is OK

        socket_write(fd, &frame);

        frame.can_id = SOLAR_HC_RES;
        frame.can_dlc = 1;
        frame.data[0] = 0x001; // Here, the system makes tests and return if the SOLAR is OK    

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
        
        if (frame.can_id == CDH_SYS_REQ) {

            frame.can_id = EPS_SYS_RES;   
            frame.can_dlc = 4;
            frame.data[0] = 0x4B;
            frame.data[1] = 0x01;
            frame.data[2] = 0x7B;
            frame.data[3] = 0x1F;

            socket_write(systemArgs->fd, &frame);

            frame.can_id = SOLAR_SYS_RES;   
            frame.can_dlc = 4;
            frame.data[0] = 0xFB;
            frame.data[1] = 0xFA;
            frame.data[2] = 0x1F;
            frame.data[3] = 0xA5;
            
            socket_write(systemArgs->fd, &frame);
        
        }

        pthread_mutex_unlock(systemArgs->mutex);

    }

}