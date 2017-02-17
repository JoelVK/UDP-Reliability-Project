#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

/* base code found at  
 * http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/
 */

int main(){
    int clientSocket, nBytes;
    char buffer[1024];
    char recvBuffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

    /*Create UDP socket*/
    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*Get a port number and an ip*/
    printf("enter in a port number: \n");
    int portNum;
    scanf("%d",&portNum);
    while(getchar() != '\n');  //Clear stdin
    
    printf("enter in an ip address: \n");
    char ipAddress [13];
    fgets(ipAddress,13,stdin);
    if(ipAddress[strlen(ipAddress) - 1] == '\n') {
            ipAddress[strlen(ipAddress) - 1] = '\0';       
    }
    while(getchar() != '\n');  //Clear stdin

    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress);//for some reason ipAddress doesnt work
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

    /*Initialize size variable to be used later on*/
    addr_size = sizeof serverAddr;

    while(1){
        printf("Type a sentence to send to server:\n");
        fgets(buffer,1024,stdin);
        if(strncmp(buffer,"/exit",5)==0){
            close(clientSocket);
            exit(0);
        }
        nBytes = strlen(buffer) + 1;
    
        /*Send message to server*/
        sendto(clientSocket,buffer,nBytes,0,(struct sockaddr *)&serverAddr,addr_size);
        

        /*Receive message from server*/
        nBytes = recvfrom(clientSocket,recvBuffer,1024,0,NULL, NULL);

        printf("Received from server: %s\n",recvBuffer);
    }

    return 0;
}
