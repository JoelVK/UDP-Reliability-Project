#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <math.h>

/* base code found at  
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

#define WIN_SIZE 5
#define PACK_SIZE 1024

int main(int argc, char **argv){
    int portNum = 0;
    char ipAddr[13];
    int clientSocket, nBytes;
    char fileBuf[PACK_SIZE];
    char* filename;
    char recvBuf[PACK_SIZE];
    char window[WIN_SIZE][PACK_SIZE];
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


    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
    //memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

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
	while(strncmp("ack", recvBuf, 3)!=0) {
	  printf("Sending filepath packet to server\n");
	  sendto(clientSocket, fileBuf, nBytes, 0, sockAddrPtr, addr_size);
	  recvfrom(clientSocket,recvBuf, sizeof(recvBuf), 0, NULL, NULL);
	  printf("Received ack packet from server\n");
	}
	recvfrom(clientSocket, recvBuf, sizeof(recvBuf), 0, NULL, NULL);
	printf("Received packet for file existence\n");
	if(strncmp("N", recvBuf, 1) == 0){
	  printf("Server could not find file...\n");
	  exit(0);
	}
	printf("Server found file!\n");

	//Open file using filename variable
	filename = basename(fileBuf);
        FILE * fp;
        fp = fopen(filename, "w");
	
        //receive size of incoming file
        int size, numPackets;
        uint32_t sendSize;
        recvfrom(clientSocket,&sendSize,sizeof(sendSize),0,NULL,NULL);
	printf("Received packet containing file size\n");

	//Calculate number of packets that will be sent
        size = ntohl(sendSize);
        numPackets = ceil((double)size/ (double)1024);
        
        //Begin to receive packets from server, while sending ack packets
        /*Receive message from server*/
        while(numPackets > 0){
            nBytes = recvfrom(clientSocket,recvBuf,1024,0,NULL, NULL);
	    printf("Received file packet. Size: %d\n", nBytes);
	    char seqNum;
	    strncpy(&seqNum,recvBuf, 1); 
	    printf("Sending ack packet (Seq Num: %c)\n", seqNum);
	    sendto(clientSocket, recvBuf, 1, 0, sockAddrPtr, addr_size);
            fwrite(recvBuf+1,nBytes-1,1,fp); //1023 isnt right FIXIT
            //printf("Received from server: %s\n",recvBuf);
            numPackets--;
        }
        fclose(fp);
    }
    return 0;
}
