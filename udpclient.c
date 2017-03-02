#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <math.h>

/* 
 * Joel Vanderklip, Michael Brecker
 * CIS 457
 * Project 2 - Reliable File Transfer over UDP
 * 2/27/17
 * 
 * Description: This is ther client side fo the udp file transfer. 
 * It implements a sliding window to avoid file transfer problems.
 * 
 * base code found at  
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

#define WIN_SIZE 5
#define PACK_SIZE 1024

void receiveFile(FILE* f, int clientSocket, struct sockaddr* sockAddrPtr, socklen_t addr_size);
void sendMsg(char* msg, int msgLen, char seqNum, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size);
void recvMsg(char* dst, int dstLen, char expSeqNum, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size);

int main(int argc, char **argv){
    int portNum = 0;
    char ipAddr[13];
    int clientSocket, nBytes;
    char fileBuf[PACK_SIZE];
    char* filename;
    char recvBuf[PACK_SIZE];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    
    /*Create UDP socket*/
    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
    
    if(argc != 3) {
	/* Print error message and set defaults */
	fprintf(stderr, "Usage: %s port IP\n", argv[0]);
	portNum = 5555;
	strcpy(ipAddr, "127.0.0.1");
    } else {
	/* Store cmd line args */
	portNum = atoi(argv[1]);
	strcpy(ipAddr, argv[2]);
    }
    printf("Port Number: %d\n", portNum);
    printf("IP Address:  %s\n", ipAddr);
    if(portNum  < 1024 || portNum  > 49151) {
	printf("Invalid port number\n");
	exit(1);
    }
    
    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddr);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
    
    /*Initialize size variable to be used later on*/
    addr_size = sizeof(serverAddr);
    struct sockaddr* sockAddrPtr = (struct sockaddr*)&serverAddr;
    while(1){
        printf("Enter file: ");
	fflush(stdout);
	fgets(fileBuf,sizeof(fileBuf),stdin);
	if(fileBuf[strlen(fileBuf)-1]=='\n'){ 
	    fileBuf[strlen(fileBuf)-1] = '\0';
        }
        if(strncmp(fileBuf,"/exit",5)==0){
            close(clientSocket);
            exit(0);
        } 
	
        nBytes = strlen(fileBuf) + 1;
	
        /*Send filepath to server, wait for ack packet*/
	sendMsg(fileBuf, nBytes, '0', clientSocket, sockAddrPtr, addr_size);
	
	//Recieve file existance packet
	recvMsg(recvBuf, sizeof(recvBuf), '1', clientSocket, sockAddrPtr, addr_size);
       
	if(strncmp("N", recvBuf, 1) == 0){
	    printf("Server could not find file...\n");
	    exit(0);
	}
	printf("Server found file!\n");
	
	//Open file using filename variable
	filename = basename(fileBuf);
        FILE * fp;
        fp = fopen(filename, "w");
	
	//Disable timeout for file transfer
	timeout.tv_sec = 0;
	setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));

	receiveFile(fp,clientSocket,sockAddrPtr,addr_size);
        fclose(fp);
    }
    return 0;
}

void sendMsg(char* msg, int msgLen, char seqNum, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size) {
    char recvBuf[1] = {'z'};
    char sendBuf[1024];

    sprintf(sendBuf, "%c", seqNum);
    memcpy(sendBuf+1, msg, msgLen);

    while(seqNum != recvBuf[0]){
	sendto(udpSock, sendBuf, msgLen+1, 0, sockAddrPtr, addr_size);
	printf("Send packet to server. SeqNum: %c\n", seqNum);
	if(recvfrom(udpSock, recvBuf, 1, 0, sockAddrPtr, &addr_size) > 0) {
	    printf("Recieved acknowledgement packet. SeqNum: %c\n", recvBuf[0]);
	}
    }
    printf("message sent successfully\n");
}

void recvMsg(char* dst, int dstLen, char expSeqNum, int udpSock, struct sockaddr* sockAddrPtr, socklen_t addr_size) {
    char seqNum = 'z';
    char recvBuf[1024];
    while(seqNum != expSeqNum) {
	if(recvfrom(udpSock, recvBuf, 1024, 0, sockAddrPtr, &addr_size) > 0){
	    strncpy(&seqNum, recvBuf, 1);
	    printf("Recieved message from server. SeqNum: %c\n", seqNum);
	    //if(seqNum == expSeqNum) {
	    memcpy(dst, recvBuf+1, dstLen);
	    sendto(udpSock, &seqNum, 1, 0, sockAddrPtr, addr_size);
	    printf("Sent acknowledgement packet. SeqNum: %c\n", seqNum);	
	    //  break;
	    //}
	} 
    }
}

void receiveFile(FILE* f, int clientSocket, struct sockaddr* sockAddrPtr, socklen_t addr_size){
    //receive size of incoming file
    int size, numPackets, nBytes;
    char window[5][1024], recvBuf[1024];
    uint32_t sendSize;
    int new_data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int last_seqNum = 0;
    int num_written = 0;
    int num_recieved = 0;
    int win_start = 0;
    int lastPacketSize = -1;
    int lastPackNum = 0; //sequence number of last packet

    //recvfrom(clientSocket,&sendSize,sizeof(sendSize),0,NULL,NULL);
    //printf("Received packet containing file size\n");
    recvMsg((char*)&sendSize, sizeof(sendSize), '2', clientSocket, sockAddrPtr, addr_size);
    //sendSize = atoi(buf);
    
    //Calculate number of packets that will be sent
    size = ntohl(sendSize);
    printf("File size: %d\n", size);
    numPackets = ceil((double)size/ (double)1024);
    
    lastPackNum = (numPackets % 10) - 1;
    printf("Last packet sequence number: %d\n", lastPackNum);
    //Begin to receive packets from server, while sending ack packets
    
    /*Receive message from server*/
    while(num_written < numPackets){
	//if(num_recieved < numPackets) {
	    nBytes = recvfrom(clientSocket,recvBuf,1024,0,NULL, NULL);
	
	    //check sequence number
	    printf("Received file packet. Size: %d\n", nBytes);
	    char seqNumChar;
	    strncpy(&seqNumChar,recvBuf, 1); 
	    printf("Sending ack packet (Seq Num: %c)\n", seqNumChar);
	    sendto(clientSocket, &seqNumChar, 1, 0, sockAddrPtr, addr_size);
	    int seqNum = atoi(&seqNumChar);
	    if(new_data[seqNum%5] == 0) { // seqNum % 5
      		memcpy(window[seqNum % 5],recvBuf,1024);
		new_data[seqNum%5] = 1; // seqNum % 5
		num_recieved++;
		if(seqNum == lastPackNum) lastPacketSize = nBytes;
	    }
	    //} 

        int i = last_seqNum % 5;
        for(; i<5; i++) {
            if(new_data[i] == 1) {
		int size = 1023;
		if(num_written == numPackets - 1) size = lastPacketSize -1;
                last_seqNum = (last_seqNum+1) % 5;
                printf("Writing %d bytes to file\n", size);
                fwrite(window[i]+1,size,1,f); 
                new_data[i] = 0;
		num_written++;
		win_start++;
            } else {
                break;
            }
        }
    }

    struct timeval tout;
    tout.tv_sec = 2;
    setsockopt(clientSocket,SOL_SOCKET,SO_RCVTIMEO,&tout,addr_size);
    while(1) {
	nBytes = recvfrom(clientSocket,recvBuf,1024,0,NULL, NULL);
	if(nBytes >= 0) {
	    char seqNumChar;
	    strncpy(&seqNumChar,recvBuf, 1); 
	    printf("Sending ack packet (Seq Num: %c)\n", seqNumChar);
	    sendto(clientSocket, &seqNumChar, 1, 0, sockAddrPtr, addr_size);
	} else {
	    break;
	}
    }
    
    //char doneMsg[1] = { 'q' };
    //sendMsg(doneMsg,sizeof(doneMsg), 'q', clientSocket, sockAddrPtr, addr_size);
}
