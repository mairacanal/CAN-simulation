#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
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

        frame.can_id = 0x141;
        frame.can_dlc = 1;
        frame.data[0] = 0x001; // Here, the system makes tests and return if the EPS is OK

        socket_write(fd, &frame);

        frame.can_id = 0x151;
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
        
        if (frame.can_id == 0x020) {

            frame.can_id = 0x241;   
            frame.can_dlc = 4;
            frame.data[0] = 0x4B;
            frame.data[1] = 0x01;
            frame.data[2] = 0x7B;
            frame.data[3] = 0x1F;

            socket_write(systemArgs->fd, &frame);

            frame.can_id = 0x251;   
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