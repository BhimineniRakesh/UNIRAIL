
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <wiringPi.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <arpa/inet.h>

#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include "marvelmind.h"

/// pour compilation
/// gcc -o Surveillance_arret surveillance_arret.c marvelmind.c marvelmind.h -lpthread -lwiringPi

#define PIN_LED2 13
#define PIN_LED3 5
#define LEGACY
#ifdef LEGACY
	#define PIN_BUMPER 17
#else
	#define PIN_BUMPER 4
#endif

static sem_t *sem;
struct timespec ts;
void semCallback()
{
	sem_post(sem);
}


int main(void)
{   
    const char * ttyFileName = "/dev/ttyACM0";
	// struct PositionValue MMpos;
	// char logLine [64] = "";
    // Init
    struct MarvelmindHedge * hedge=createMarvelmindHedge ();
    struct PositionValue MMlocality;
    struct can_frame  positionframe;
   if (hedge==NULL)
    {
        puts ("Error: Unable to create MarvelmindHedge");
        return -1;
    }
    hedge->ttyFileName=ttyFileName;
    hedge->anyInputPacketCallback= semCallback;
    hedge->verbose=true; // show errors and warnings
    startMarvelmindHedge (hedge);

	sem = sem_open(DATA_INPUT_SEMAPHORE, O_CREAT, 0777, 0);

	// FILE * logFile = fopen("logBalCoords.txt", "a");
	// if (logFile==NULL)
		// printf(" \n Error opening log file\n");
	// fprintf(logFile,"#balise; X; Y\n");
	// fclose(logFile);

		
	int socketCan;
	int nbytes;
	struct sockaddr_can addr;
	struct can_frame frameRx, frameTx;
	int fd;
	char bufAddr[20];
	struct ifreq ifr;
	struct can_filter rfilter[3]; // attention a bien adapter la taille du tableau 
	unsigned int ip3, ip2, ip1, ip0;

	wiringPiSetupGpio () ;
	pinMode (PIN_LED2, OUTPUT) ;
	pinMode (PIN_LED3, OUTPUT) ;
	pinMode (PIN_BUMPER, INPUT) ;
	digitalWrite(PIN_LED2, HIGH);
	const char *ifname = "can0";
	int count;

	if((socketCan = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
		perror("Error while opening socket");
		return -1;
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(socketCan, SIOCGIFINDEX, &ifr);

	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

	if(bind(socketCan, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}
	 
	rfilter[0].can_id   = 0x015;
	rfilter[0].can_mask = CAN_SFF_MASK;
	rfilter[1].can_id   = 0x024;
	rfilter[1].can_mask = CAN_SFF_MASK;
	rfilter[2].can_id   = 0x030;
	rfilter[2].can_mask = CAN_SFF_MASK;

	setsockopt(socketCan, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)); 
	// MCP2515
	//Receive buffers, masks and filters:
	//-  Two receive buffers with prioritized message storage
	//-  Six 29-bit filters
	//- Two 29-bit mask

	//To disable the reception of CAN frames on the selected CAN_RAW socket:
	//setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	/* I want to get an IPv4 IP address */
	fcntl(socketCan, F_SETFL, fcntl(socketCan, F_GETFL) | O_NONBLOCK);
	positionframe.can_id = 0x123; // set the CAN identifier
	positionframe.can_dlc = 8; // set the data length to 8 bytes

	while(1)
	{
		if (!digitalRead(PIN_BUMPER))
		{
			digitalWrite(PIN_LED2, LOW);
			digitalWrite(PIN_LED3, LOW);
			frameRx.can_id  = 0x16;	// consigneVistesseUrgence
			frameRx.can_dlc = 1;	
			frameRx.data[0] = 0x00;

			nbytes = write(socketCan, &frameRx, sizeof(struct can_frame));
			close(socketCan);
			printf("Arret suite a accident\n");	
			system ("sudo shutdown -h now");
		}
		char buf[20];
		count = recv(socketCan, buf, sizeof(buf), (MSG_DONTWAIT | MSG_PEEK) );
		if (count>0)
		{
			read(socketCan,&frameRx,sizeof(struct can_frame));
			if (frameRx.can_id==0x015)
			{  
				digitalWrite(PIN_LED2, LOW);
				digitalWrite(PIN_LED3, LOW);
				stopMarvelmindHedge (hedge);
				destroyMarvelmindHedge (hedge);
				printf("Arret hardware\n");
				system ("sudo shutdown -h now");
			}
			if (frameRx.can_id==0x024)
			{
				if (!(frameRx.data[4]&0x02))
				{
					ifr.ifr_addr.sa_family = AF_INET;
					/* I want IP address attached to "wlan0" */
					strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
					ioctl(fd, SIOCGIFADDR, &ifr);

					sprintf(bufAddr,"%s",inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
					sscanf(bufAddr, "%u.%u.%u.%u", &ip3, &ip2, &ip1, &ip0); 
					if (ip3!=0)
					{
						frameTx.can_id = 0xA1;
						frameTx.can_dlc = 5;
						frameTx.data[0] = 0x20;
						frameTx.data[1] = ip3;
						frameTx.data[2] = ip2;
						frameTx.data[3] = ip1;
						frameTx.data[4] = ip0;
						nbytes = write(socketCan, &frameTx, sizeof(struct can_frame));
					}
				}
				else
					digitalWrite(PIN_LED3, HIGH);
			}
			if (frameRx.can_id==0x030)
			{
				printf("%X\n", count);
				if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
				{
					printf("clock_gettime error");
					return -1;
				}
				ts.tv_sec += 1;
				sem_timedwait(sem,&ts);
				frameRx.can_id = 0;
			}	
	}

     //        printPositionFromMarvelmindHedge(hedge, true);

    // Create a CAN socket
    // int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    // if (s < 0) {
        // perror("socket");
        // return 1;
    // }

    // Set up the CAN interface
    // struct ifreq ifr;
    // strcpy(ifr.ifr_name, "can0");
    // ioctl(s, SIOCGIFINDEX, &ifr);

    // Bind the socket to the CAN interface
    // struct sockaddr_can addr;
    // addr.can_family = AF_CAN;
    // addr.can_ifindex = ifr.ifr_ifindex;
    // bind(s, (struct sockaddr *)&addr, sizeof(addr));
	// if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	// {
    // perror("bind");
    // return 1;
    // }


    // Set up the CAN frame
    // Get the position data
    if (hedge->haveNewValues_) 
	{
	 printPositionFromMarvelmindHedge(hedge, true);
	 printRawIMUFromMarvelmindHedge(hedge, true);

    getPositionFromMarvelmindHedge(hedge, &MMlocality);
	hedge->haveNewValues_ = false;	
        // perror("getPositionFromMarvelmindHedge");
        // close(s);
        // return 1;
	 // x = (int)(MMlocality.x*1000);
    // y = (int)(MMlocality.y*1000);
    // z = (int)(MMlocality.z*1000);
    // Validate the position data
    // if (MMlocality.x > INT_MAX || MMlocality.x < INT_MIN ||
        // MMlocality.y > INT_MAX || MMlocality.y < INT_MIN ||
        // MMlocality.z > INT_MAX || MMlocality.z < INT_MIN)
	// {
        // fprintf(stderr, "Invalid position data\n");
         // close(s);
        // return 1;
    // }

    // Set the CAN frame data to the position data
     positionframe.data[0] = (MMlocality.x >> 24) & 0x00FF;
     positionframe.data[1] = (MMlocality.x >> 16) & 0x00FF;
     positionframe.data[2] = (MMlocality.x >> 8) & 0x00FF;
     positionframe.data[3] = MMlocality.x & 0x00FF;
     positionframe.data[4] = (MMlocality.y >> 24) & 0x00FF;
     positionframe.data[5] = (MMlocality.y >> 16) & 0x00FF;
     positionframe.data[6] = (MMlocality.y >> 8) & 0x00FF;
     positionframe.data[7] = MMlocality.y & 0x00FF;

    // Send the CAN frame
	nbytes = write(socketCan, &positionframe, sizeof(struct can_frame));
    if (nbytes < 0) 
	{
		perror("write");
		return 1;
    }
	else if (nbytes != sizeof(struct can_frame)) 
	{
		fprintf(stderr, "Incomplete write.\n");
		return 1;
    }
    }
     
    	
	usleep(10000);	
	}
	return 0;
}

