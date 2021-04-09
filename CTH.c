/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CTH                                                                             *
 * Author: Maíra Canal (@mairacanal)                                               *
 * São Carlos School of Engineering - University of São Paulo                      *
 * Abril/2021                                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include <pthread.h>
#include <unistd.h>
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

    socket_initiation("vcan1", &s);
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

        frame.can_id = CTH_HC_RES;
        frame.can_dlc = 1;
        frame.data[0] = 0x001; // Here, the system makes tests and return if the CTH is OK

        socket_write(fd, &frame);

    }

    pthread_mutex_unlock(mutex); 

}

void *systemReceive(void *args) {

    struct can_frame frame;
    struct systemArgs *systemArgs = (struct systemArgs*)args;
    
    while (1) {

        pthread_mutex_lock(systemArgs->mutex);

        socket_read(systemArgs->fd, &frame);
        printCANframe(frame);

        if (frame.can_id == CDH_SYS_REQ) {

            frame.can_id = CTH_SYS_RES;
            frame.can_dlc = 1;
            frame.data[0] = 0x001; 

            socket_write(systemArgs->fd, &frame);

        } else if (frame.can_id == CDH_IMG_REQ) {

            __uint8_t count = 0x00;

            for (int i = 0; i < 6500; i++) { // 6500 pkg of 8 bytes = 52kb

                frame.can_id = CTH_IMG_RES;
                frame.can_dlc = 8;
                for (int j = 0; j < 8; j++) {
                    frame.data[j] = count; 
                }
                socket_write(systemArgs->fd, &frame);

                if (count < 0xFF) count++;
                else count = 0x00;
            
            }

            frame.can_dlc = 1;
            frame.data[0] = 0x001;

            socket_write(systemArgs->fd, &frame);   

        }

        pthread_mutex_unlock(systemArgs->mutex);

    }

}