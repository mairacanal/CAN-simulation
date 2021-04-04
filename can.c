#include "can.h"

int socket_initiation (char * interface_name, int * file_descriptor) {

    int s; 
	struct sockaddr_can addr;
	struct ifreq ifr;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket initialization error");
		return 1;
	}

	strcpy(ifr.ifr_name, interface_name);
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Binding error");
		return 1;
	}

    *file_descriptor = s;

    return 0;

}

int socket_write (int s, struct can_frame *frame) {

    if (write(s, frame, sizeof(struct can_frame)) < 0) {
        perror("Write");
        return 1;
	}

    return 0;

}

int socket_read (int s, struct can_frame *frame) {

    int nbytes;

    nbytes = read(s, frame, sizeof(struct can_frame));

    if (nbytes < 0) {
        perror("Read error");
    	return 1;
	}	

    if (nbytes < sizeof(struct can_frame)) {
        fprintf(stderr, "read: incomplete CAN frame\n");
        return 1;
    }

    return 0;

}

void printCANframe (struct can_frame frame) {

    printf("0x%03X [%d] ", frame.can_id, frame.can_dlc);

    for (int i = 0; i < frame.can_dlc; i++) {
        printf("%02X ", frame.data[i]);
    }

	printf("\n");

}

int socket_close (int file_descriptor) {

    if (close(file_descriptor) < 0) {
        perror("Closing error");
        return 1;
    }

    return 0;

}