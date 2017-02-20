#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
 * Joel Vanderklip, Michael Brecker
 * CIS 457
 * Project 2 - Reliable File Transfer over UDP
 * 2/27/17
 * 
 * Description: This is the server side of the udp file transfer.
 * base code for this comes from 
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

#define WIN_SIZE 5
#define PACK_SIZE 1024

void transferfile(FILE* f, int udpSock, struct sockaddr_in clientAddr);
int filesize(FILE* file);

int main(int argc, char** argv){
    int udpSocket, nBytes;
    char buffer[PACK_SIZE];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addr_size;
    int portNum = 0;
    FILE* file;
    char window[WIN_SIZE][PACK_SIZE];
    
    //timeout for ack packet
    struct timeval timeout;
    timeout.tv_sec=1;
    timeout.tv_usec=0;
    

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


    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    //memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

    /*Bind socket with address struct*/
    bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    /*Initialize size variable to be used later on*/ 
    addr_size = sizeof(clientAddr);
    struct sockaddr* sockAddrPtr = (struct sockaddr*)&clientAddr;

    while(1){
      recvfrom(udpSocket,buffer,sizeof(buffer),0,sockAddrPtr, &addr_size);
      printf("Received filepath packet\nSending ack packet\n");
      sendto(udpSocket,"ack",3,0,sockAddrPtr ,addr_size); //Received filepath
        
      if((file = fopen(buffer, "r")) != NULL) {
	printf("Sending file confirmation packet\n");
	sendto(udpSocket,"Y",1,0,sockAddrPtr ,addr_size); //File exists
	
	//Set timeout
	setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
        
	//get size of file in bytes
	uint32_t sendSize;
	sendSize = htonl(filesize(file));
	printf("Sending file size packet\n");
	sendto(udpSocket,&sendSize,sizeof(sendSize),0,sockAddrPtr, addr_size);

	int i = 0;
	while((nBytes = fread(window[i],1,sizeof(window[i]),file)) > 0){
	  printf("Sending file data packet. Size: %d\n", nBytes);
	  sendto(udpSocket,window[i],nBytes,0,sockAddrPtr,addr_size);
	  i = (i+1) % WIN_SIZE;
	}
	
	fclose(file);
	timeout.tv_sec=0;
	setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
      } else {
	printf("Sending file failure packet\n");
	sendto(udpSocket,"N",1,0,sockAddrPtr ,addr_size); //File not exist
      }
      
    }
    return 0;
}

void transferfile(FILE* f, int udpSock, struct sockaddr_in clientAddr) {
  char window[WIN_SIZE][PACK_SIZE];
  int win_start = 0;
  int acks[WIN_SIZE*2] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  char buf[PACK_SIZE - 1];
  char ack_buf[1];

  socklen_t addr_size = sizeof(clientAddr);
  struct sockaddr* sockAddrPtr = (struct sockaddr*)&clientAddr;
  while(1) {

    //Search window for available packet spaces
    int i, newDataIndex = 0;
    for(i=0; i<WIN_SIZE*2; i++) {
      if(i >= win_start && i < win_start+WIN_SIZE) {
	if(acks[i] == 1) newDataIndex = (i+1) - win_start; 
      }
    }

    //Gets contents from file and stores in available window packets
    int nbytes = 0;
    for(i=newDataIndex; i<WIN_SIZE; i++) {
      nbytes = fread(buf,1,sizeof(buf),f);
      if(nbytes > 0) {
	sprintf(window[i%WIN_SIZE], "%d%s", i, buf); //Not sure if this will work to copy
	printf("Msg to send: %s\n", window[i%WIN_SIZE]);
      }
      else {
	break; //file transfer is complete
      }
    }

    //Send new/unacknowledged packets
    for(i=0; i<WIN_SIZE;i++) {
      if(acks[(win_start+i)%(WIN_SIZE*2)] == 0) {
	sendto(udpSock,window[i],nbytes,0,sockAddrPtr,addr_size);
      }
    }

    //Get acknowledgements
    for(i=0; i<WIN_SIZE; i++) {
      nbytes = recvfrom(udpSock,ack_buf,sizeof(ack_buf),0,sockAddrPtr, &addr_size);
      if(nbytes == -1) {
	//timeout reached
      } else {
	int ackNum = atoi(ack_buf);
	acks[ackNum] = 1;

	//increment start of window
	if(acks[win_start] == 1) win_start = (win_start + 1) % (WIN_SIZE*2); 
      }
    }

    //Reset acks that are outside window
    for(i=0; i<WIN_SIZE*2; i++) {
      if(i < win_start || i >= win_start+WIN_SIZE) {
	acks[i] = 0;
      }
    }

  }
}

int filesize(FILE* file) {
  int size;
  fseek(file,0,SEEK_END);
  size = ftell(file);
  fseek(file,0,SEEK_SET);
  return size;
}
