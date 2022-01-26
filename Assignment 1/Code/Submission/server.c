#include "headsock.h"

// transmitting and receiving function
void str_ser(int sockfd, struct sockaddr *addr, int addrlen);             

int main(void)
{
	//socket file descriptor
	int sockfd;
	//socket address information for many types of sockets
	struct sockaddr_in my_addr;

	//create socket
	//AF_INET = IPv4, SOCK_DGRAM is datagram sockets
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);          
	if (sockfd == -1)
	{
		printf("error in socket!");
		exit(1);
	}
	
	//contains a code for the address family
	my_addr.sin_family = AF_INET;
	//port number
	//converts a port number in host byte order to a port number in network byte order
	my_addr.sin_port = htons(MYUDP_PORT);
	//s_addr contains the IP address of the host
	//INADDR_ANY which gets this address (socket will be bound to all local interfaces)
	my_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr("172.0.0.1");
	
	//zeroes out the sin_zero array
	bzero(&(my_addr.sin_zero), 8);

	//bind socket
	//ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));                
	if ((bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)))==-1) {
		printf("error in binding");
		exit(1);
	}
	
	printf("waiting for data\n");
	//receive packet and response
	while(1){
		str_ser(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));                
	}
	
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd, struct sockaddr *addr, int addrlen)
{

    // buffer to save received data
	char buf[BUFSIZE];
    // storing received data
	FILE *fp;
    // receiving data
	char recvs[DATALEN];
    // ack packet
	struct ack_so ack;
	int end = 0, n = 0;
	long lseek=0;
	int window = 1;
	int currentSize = 0;

	while(!end) {
		printf("===========\n");

        // complete packet
        if ((n= recvfrom(sockfd, &recvs, DATALEN, 0, addr, (socklen_t *)&addrlen))==-1) {
            printf("error when receiving\n");
            exit(1);
        }
        currentSize += 1;
        printf("received a packet\n");
        printf("current window is: %d\n", currentSize);
        
        //end of the file
        if (recvs[n-1] == '\0')	{
            end = 1;
            n --;
        }

        // process the received packet
        memcpy((buf+lseek), recvs, n);
        //printf("%d bytes of data received: %s\n", n, recvs);
        lseek += n;

        if (currentSize == window) {
            // send ack 
            ack.num = 1;
            ack.len = 0;
            if ((n = sendto(sockfd, &ack, 2, 0, addr, addrlen))==-1) {
                printf("send error!");								
                exit(1);
            }
            printf("sent an ack\n");
            currentSize = 0;
			window += 1;
			if (window == 4) {
				window = 1;
			}
        }
    

	    printf("the end: %d\n", end);
    }
	
  
	if ((fp = fopen ("myUDPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exist\n");
		exit(0);
	}
    //write data into file
	fwrite (buf , 1 , lseek , fp);					
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}