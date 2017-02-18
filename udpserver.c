#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

/*base code for this comes from 
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

int main(int argc, char** argv){
    int udpSocket, nBytes;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addr_size;
    int portNum = 0;
    FILE* file;
    char window[5][1024];

    
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
	sendto(udpSocket,"ack",3,0,sockAddrPtr ,addr_size); //Received filepath
        
	/*print message*/
	printf("Received from client: %s \n",buffer);
	if((file = fopen(buffer, "r")) != NULL) {
	  printf("Found file\n");
	  sendto(udpSocket,"Y",1,0,sockAddrPtr ,addr_size); //File exists
	  
	  //Transfer file here using sliding window
	  setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
	  fclose(file);
	  timeout.tv_sec=0;
	  setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
	} else {
	  printf("Could not find file\n");
	  sendto(udpSocket,"N",1,0,sockAddrPtr ,addr_size); //File not exist
	}
	    
    }
    return 0;
}
