#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

/* base code found at  
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

int main(int argc, char **argv){
    int portNum = 0;
    char ipAddr[13];
    int clientSocket, nBytes;
    char fileBuf[1024];
    char* filename;
    char recvBuf[1024];
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
	if(fileBuf[strlen(fileBuf)-1]=='\n') fileBuf[strlen(fileBuf)-1] = '\0';
        if(strncmp(fileBuf,"/exit",5)==0){
            close(clientSocket);
            exit(0);
        } 
	
        nBytes = strlen(fileBuf) + 1;
    
        /*Send filepath to server, wait for ack packet*/
	while(strncmp("ack", recvBuf, 3)!=0) {
	  sendto(clientSocket, fileBuf, nBytes, 0, sockAddrPtr, addr_size);
	  recvfrom(clientSocket,recvBuf, sizeof(recvBuf), 0, NULL, NULL);
	}
	recvfrom(clientSocket, recvBuf, sizeof(recvBuf), 0, NULL, NULL);
	if(strncmp("N", recvBuf, 1) == 0){
	  printf("File not found...\n");
	  exit(0);
	}
	printf("File was found!\n");
	filename = basename(fileBuf);
	
	//Open file using filename variable
	//Begin to receive packets from server, while sending ack packets

        /*Receive message from server*/
        nBytes = recvfrom(clientSocket,recvBuf,1024,0,NULL, NULL);

        printf("Received from server: %s\n",recvBuf);
    }

    return 0;
}
