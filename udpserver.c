#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

/*
 * Joel Vanderklip, Michael Brecker
 * CIS 457
 * Project 2 - Reliable File Transfer over UDP
 * 2/27/17
 * 
 * Description: This is the server side of the udp file transfer.
 * It implements a sliding window to avoid file transfer problems. 
 * base code for this comes from 
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

#define WIN_SIZE 5
#define PACK_SIZE 1024

void transferfile(FILE* f, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size);
void recvMsg(char* dst, int dstLen, char expSeqNum, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size);
void sendMsg(char* msg, char seqNum, int msgLen, int udpSock,struct sockaddr* sockAddrPtr, socklen_t addr_size);
int filesize(FILE* file);

struct sockaddr_in clientAddr;

int main(int argc, char** argv){
    char buffer[PACK_SIZE];
    struct sockaddr_in serverAddr;
    int udpSocket;
    socklen_t addr_size;
    int portNum = 0;
    FILE* file;
    
    /*Create UDP socket*/
    udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
    
    /* Check number of cmd line args */
    if(argc != 2) {
	//Print and set default
	fprintf(stderr, "Usage: %s portNum\n", argv[0]);
	portNum = 5555;
    } else {
	//Set port number from cmd line arg
	portNum = atoi(argv[1]);
    }
    printf("Port Number: %d\n", portNum);
    if(portNum < 1024 || portNum > 49151){
	printf("Invalid port number\n");
	exit(1);
    }
    
    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    /*Bind socket with address struct*/
    if(bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){ 
	printf("Failed to bind socket with address structure\n");
	exit(1);
    }
    
    /*Initialize size variable to be used later on*/ 
    addr_size = sizeof(clientAddr);
    struct sockaddr* sockAddrPtr = (struct sockaddr*)&clientAddr;
    
    //timeout for ack packet
    struct timeval timeout;
    timeout.tv_sec=1;
    timeout.tv_usec=0;
    
    while(1){
	//Recieve filepath packet
	recvMsg(buffer, sizeof(buffer), '0', udpSocket, sockAddrPtr, addr_size); 
	//Open file if exists
	if((file = fopen(buffer+1, "r")) != NULL) {	    
	    //Set timeout
	    setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
	    //Send packet notifying that file exists
	    sendMsg("Y", '1', 1, udpSocket, sockAddrPtr, addr_size);

	    //Transfer file
	    transferfile(file, udpSocket, sockAddrPtr, addr_size);
	    
	    //Disable timeout and close file
	    fclose(file);
	    timeout.tv_sec=0;
	    setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
	} else {
	    //Send packet notifying that file could not be found
	    sendMsg("N", '1', 1, udpSocket, sockAddrPtr, addr_size);
	}
	
    }
    return 0;
}

//Set timeout before use of this function
void sendMsg(char* msg, char seqNum, int msgLen, int udpSock,struct sockaddr* sockAddrPtr, socklen_t addr_size ) {
    char recvBuf[1] = {'z'};
    char sendBuf[1024];
    
    //Format msg with seq. number
    sprintf(sendBuf,"%c", seqNum);
    memcpy(sendBuf+1,msg,msgLen);
    
    //Guarentee message was recieved
    while(strncmp(&seqNum, recvBuf, 1) != 0) {
	sendto(udpSock,sendBuf,msgLen+1,0,sockAddrPtr, addr_size); 
	printf("Sent packet to client. Seq. Number: %c\n", seqNum);
	if(recvfrom(udpSock, recvBuf,1,0,sockAddrPtr,&addr_size) > 0) { 
	    printf("Recieved acknowledgement packet. Seq. Number: %c\n", recvBuf[0]);
	}
    }
}

//Does not need timeout
void recvMsg(char* dst, int dstLen, char expSeqNum, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size ) {
    char seqNum = 'z';
    
    //Recieve packet and send acknowledgment
    while(expSeqNum != seqNum) {
	recvfrom(udpSock, dst, dstLen, 0, sockAddrPtr, &addr_size);
	strncpy(&seqNum, dst, 1);
	printf("Recieved pakcet from client. Seq. Number: %c\n", seqNum);
	//if(strncmp(&expSeqNum, &seqNum, 1) == 0) {
	sendto(udpSock, &seqNum, 1,0, sockAddrPtr, addr_size);
	printf("Sent acknowledgement packet. Seq. Number: %c\n", seqNum);
	//break;
	//}
    }
}

void transferfile(FILE* f, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size) {
    char buf[1023];
    char window[5][1024];
    int acks[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int win_start = 0;
    int win_end = 0;
    
    
    //get size of file in bytes
    int size = filesize(f);
    uint32_t sendSize;
    sendSize = htonl(size);
    printf("File Size: %d\n", size);
  
    //Send packet containing file size
    sendMsg((char*)&sendSize, '2', sizeof(sendSize), udpSock, sockAddrPtr, addr_size);
    int totalpacks = ceil((double)size / (double) (PACK_SIZE-1));
    int numpacks = totalpacks;
    int numpacksread = 0;
    int lastpacksize = 0;
    int lastpackpos = -1;
    int win_count = 0;
    
    while(numpacks > 0) {
	int i, nbytes;
	char ack_buf[1];
	
	//Update window buf
	i = win_end % 5;
	for(; i<5; i++) {
	    if(win_count == 5) break;           //Window is full
	    nbytes = fread(buf,1,sizeof(buf),f);//Read next data from file
	    if(nbytes <= 0) break;              //Nothing more to read
	    numpacksread++;
	    win_count++;
	    int seqNum = (win_end) % 10;        //Calculate sequence num
	    
	    //Add data to window with sequence number added in first byte
	    sprintf(window[i],"%d", seqNum);
	    memcpy(window[i]+1,buf,1023);
	    
	    sendto(udpSock,window[i],nbytes+1,0,sockAddrPtr, addr_size);
	    printf("Sent packet of size %d and seq. num %c\n", nbytes+1, window[i][0]);
	    if(numpacksread == totalpacks){
		lastpacksize = nbytes;
		lastpackpos = i;
	    }
	    win_end = (win_end + 1) % 10;
	}
	
	//Try to receive one acknowledgement
	nbytes = recvfrom(udpSock,ack_buf,1,0,sockAddrPtr,&addr_size);
	if(nbytes == -1) {
	    //No acks recieved, resend unacknowledged packets in window
	    for(i=0; i<5; i++) {
		int ack = acks[(win_start + i)%10];
		if(ack == 0) {
		    int psize = 1024;
		    if(i == lastpackpos){
			psize = lastpacksize + 1;
		    }
		    else{
			psize = 1024;
		    }
		    sendto(udpSock,window[i],psize,0,sockAddrPtr, addr_size);
		    printf("Re-sent packet of size %d and seq. num %c\n", psize, window[i][0]);
		    if(i == lastpackpos) break;
		}
	    }
	} else {
	    //Ack recieved, record ack for seq. num
	    int ackseqNum = atoi(ack_buf);
	    printf("Received ack for seq. num %d\n", ackseqNum);
	    acks[ackseqNum] = 1;
	    numpacks--;
	    
	    //Shift window as far as possible
	    for(i=0; i<5; i++) {
		int ack = acks[win_start];
		if(ack == 1) {
		    acks[win_start] = 0; //reset ack for seq. num outside window
		    win_start = (win_start+1) % 10; //advance window start
		    win_count--;        //Count how many packets are in window
		} else {
		    break;
		}
	    }
	}
    }
    printf("File Transfer Completed\n");
}

int filesize(FILE* file) {
  int size;
  fseek(file,0,SEEK_END);
  size = ftell(file);
  fseek(file,0,SEEK_SET);
  return size;
}
