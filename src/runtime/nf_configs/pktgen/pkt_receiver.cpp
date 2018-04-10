#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <string>
#include <iostream>
#include <signal.h>
#include<unistd.h>

#include "../../socket_util.h"


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void int_handler(int sig);


int sockfd;

int main(int argc,char **argv)
{
	signal(SIGINT, int_handler);

	if (argc != 2) {
		std::cerr << "./receiver portno";
		return -1;
	}

	// get this server's bind port
    int bind_port = atoi(argv[1]);
    printf("Listening on port %s\n", argv[1]);

	sockfd = open_socket();

    if (bind_socket(sockfd, bind_port) < 0) {
        std::cerr << "bind failure: " << strerror(errno) << std::endl;
        return -1;
    }

    int count = 0;

    while (true) {
        printf("\nListening for packets...\n");

        sockdata *pkt_data = receive_data(sockfd);
        if (pkt_data == NULL || pkt_data->size == 0) { 
            std::cerr << "packet receive error: " << strerror(errno) << std::endl;
            continue;
        }
        count++;

        packet *p = packet_from_data(pkt_data);
        delete pkt_data;
        
        p->print_info();
        printf("Received %d packets in total\n", count);
    }


	return 0;
}

void int_handler(int sig)
{
    close(sockfd);
    exit(0);
}

#pragma clang diagnostic pop