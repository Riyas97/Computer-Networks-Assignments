/*
UDP socket for transferring big file in small data units
*/
#include "headsock.h"


//Responsible for calculating the time interval between out and in
void tv_sub(struct  timeval *out, struct timeval *in) {
	if ((out->tv_usec -= in->tv_usec) <0) {
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}


//Responsible for transmission
float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len) {

    //buffer containing whole file
	char *buf;
    //size of file and current index
	long lsize, ci = 0;   
    //packet to be sent
	char sends[DATALEN];    
	struct ack_so ack;
    //length of current packet
	int n, slen;  
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	
    int window = 1;
	int currentSize = 0;
    

	fseek (fp , 0 , SEEK_END);
	//total size of file
    lsize = ftell (fp);

	rewind (fp);
    printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

    // allocate memory to contain the whole file.
	buf = (char *) malloc (lsize+1);
	if (buf == NULL) exit (2);

    // copy the file into the buffer.
	fread (buf,1,lsize,fp);

    /*** the whole file is loaded in the buffer. ***/
    //append the end byte (extra byte sent to server)
	buf[lsize] ='\0';		
    //get the current time				  
	gettimeofday(&sendt, NULL);		
	
    while(ci <= lsize) {
		printf("===========\n");

        if ((lsize+1-ci) <= DATALEN) {
            slen = lsize+1-ci;
        } else {
            slen = DATALEN;
        }
        
        printf("slen: %d\n", slen);

        memcpy(sends, (buf+ci), slen);
        
		//send the data
		n = sendto(sockfd, &sends, slen, 0, addr, addrlen);
		if(n == -1) {
			printf("send error!");								
			exit(1);
		}
		currentSize += 1;
		printf("send a packet\n");

		if (currentSize == window) {
			//receive the ack
			if ((n = recvfrom(sockfd, &ack, 2, 0, addr, (socklen_t*)&addrlen))== -1) { 
				//no ack received
				printf("error when receiving ack\n");
				exit(1);
			}
			if (ack.num == 1 && ack.len == 0) {
				//ACK
				ci += slen;
				printf("receive an ack\n");
			} else {
				printf("error in ack transmission");
			}
			currentSize = 0;
			if (window == 1) {
				window = 2;
			} else if (window == 2) {
				window = 3;
			} else if (window == 3) {
				window = 1;
			}
		} else {
			ci += slen;
		}
        
        
	}

    //get current time
    gettimeofday(&recvt, NULL);       
    *len= ci;    
    // get the whole trans time
    tv_sub(&recvt, &sendt);              
    time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
    return(time_inv);
}

    
int main(int argc, char **argv)
{

    //socket file descriptor
	int sockfd;
    //ti: time, rt: transmission rate
	float ti, rt, th;
    //data size sent to server
	long len;
    //socket address information
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc != 2) {
		printf("parameters not match");
        exit(0);
	}

    //get host's information
    if ((sh=gethostbyname(argv[1]))==NULL) {            
		printf("error when gethostbyname");
		exit(0);
	}

    //create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);            
	if (sockfd<0) {
		printf("error in socket");
		exit(1);
	}

    //print the remote host's information
    printf("canonical name: %s\n", sh->h_name); //127.0.0.1 or localhost					
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++){
        printf("the aliases name is: %s\n", *pptr); //socket information
    }
	switch(sh->h_addrtype) {
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
    addrs = (struct in_addr **)sh->h_addr_list;
	
  
	ser_addr.sin_family = AF_INET; //IPv4                                                
	ser_addr.sin_port = htons(MYUDP_PORT); //port number
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr)); //IP address of running server
	bzero(&(ser_addr.sin_zero), 8); 
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL) { //file to read data
		printf("File doesn't exit\n");
		exit(0);
	}

    //transmission and receiving
	ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
	if (ti != -1)	{
		rt = (len/(float)ti);           //caculate the average transmission rate
		th = 8*rt/(float)1000;
		printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s), Throughput: %f (Mbps)\n", ti, (int)len, rt, th);
	}

	close(sockfd);
	fclose(fp);
	exit(0);
}
