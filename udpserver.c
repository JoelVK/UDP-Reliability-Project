#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

/*base code for this comes from 
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

int main(){
    int udpSocket, nBytes;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    struct timeval timeout;
    timeout.tv_sec=5;
    timeout.tv_usec=0;

    /*Create UDP socket*/
    udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*get the port number*/
    printf("enter a port number: \n");
    int portNum;
    scanf("%d",&portNum);
    while(getchar() != '\n');

    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

    /*Bind socket with address struct*/
    bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(serverAddr));
    /*Initialize size variable to be used later on*/ 
    addr_size = sizeof serverStorage;

    while(1){
        /* Try to receive any incoming UDP datagram. Address and port of 
        requesting client will be stored on serverStorage variable */
        nBytes = recvfrom(udpSocket,buffer,1024,0,(struct sockaddr *)&serverStorage, &addr_size);

        if(nBytes >= 0){
        
            /*print message*/
            printf("Received from client: %s \n",buffer);
    
            /*Send uppercase message back to client, using serverStorage as the address*/
            sendto(udpSocket,buffer,nBytes,0,(struct sockaddr *)&serverStorage,addr_size);

        }
        else{
            printf("no message received in time\n");
        }
    }


    return 0;
}
