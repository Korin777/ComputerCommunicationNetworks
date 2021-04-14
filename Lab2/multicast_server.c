/* Send Multicast Datagram code example. */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct in_addr localInterface;
struct sockaddr_in groupSock;
int sd;
unsigned char databuf[1024];
int datalen = sizeof(databuf);
 
int main (int argc, char *argv[ ])
{
	
	FILE *fp = fopen(argv[1],"rb");
	fseek(fp,0,SEEK_END);
    int size = ftell(fp);
    fseek(fp,0,SEEK_SET);


/* Create a datagram socket on which to send. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
	  perror("Opening datagram socket error");
	  exit(1);
	}
	else
	  printf("Opening the datagram socket...OK.\n");
	 
	/* Initialize the group sockaddr structure with a */
	/* group address of 226.1.1.1 and port 4321. */
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
	groupSock.sin_port = htons(4321);
	 
	/* Set local interface for outbound multicast datagrams. */
	/* The IP address specified must be associated with a local, */
	/* multicast capable interface. */
	localInterface.s_addr = inet_addr("140.116.112.162");
	
	/* IP_MULTICAST_IF:  Sets the interface over which outgoing multicast datagrams are sent. */
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
	{
	  perror("Setting local interface error");
	  exit(1);
	}
	else
	  printf("Setting the local interface...OK\n");
	/* Send a message to the multicast group specified by the*/
	/* groupSock sockaddr structure. */
	/*int datalen = 1024;*/
	// if(sendto(sd, databuf, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
	// {
	// 	perror("Sending datagram message error");
	// }
	// else
	//   printf("Sending datagram message...OK\n");

	int readlen = 0;
	while(1) {
		readlen = fread(databuf, sizeof(unsigned char), 1023, fp);
		if(readlen == 0) {
			if(sendto(sd, databuf, readlen, 0, (struct sockaddr *)&groupSock,sizeof(groupSock)) < 0)
				perror("Sending datagram message error");
			break;
		}
		if(sendto(sd, databuf, readlen, 0, (struct sockaddr *)&groupSock,sizeof(groupSock)) < 0)
			perror("Sending datagram message error");
		// transSize += 255;
		// if(transSize >= size/4*count) {
		// 	printf("%d%% ",count*25);
		// 	count++;
		// 	gettime();
		// }
		bzero(databuf,1024);
	}
	printf("Sending datagram message...OK\n");
	printf("file size : %d(Byte)\n",size);

	/* Try the re-read from the socket if the loopback is not disable
	if(read(sd, databuf, datalen) < 0)
	{
	perror("Reading datagram message error\n");
	close(sd);
	exit(1);
	}
	else
	{
	printf("Reading datagram message from client...OK\n");
	printf("The message is: %s\n", databuf);
	}
	*/
	return 0;
}
