#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>

#include "WolkConn.h"

#include "MQTTPacket.h"
#include "transport.h"

#include <signal.h>

#include <sys/time.h>

#define BUFFER_SIZE 256

static unsigned char buffer[BUFFER_SIZE];
static int buffer_length = sizeof(buffer);

const char *serial_mqtt = "123567";
const char *topic = "sensors/123567";
const char *password_mqtt = "4c098907-0a23-4246-8e3d-fdf4e73409a1";
const char *message = "STATUS S:true:READY;";
const char *topic_sub = "config/123567";
const char *hostname = "integration.wolksense.com";
int portno = 1883;
int sockfd;
const char *numeric_slider_reference = "SL";
const char *bool_switch_refernece = "SW";

static volatile int toStop = 0;

enum states { READING, PUBLISHING };

int send_buffer(unsigned char* buffer, unsigned int len)
{
    int n = write(sockfd, buffer, len);
    if (n < 0)
        return -1;

    return n;
}

static int receive_buffer(unsigned char* buffer, unsigned int max_bytes)
{
    bzero(buffer, max_bytes);
    int n = read(sockfd, buffer, max_bytes);
    if (n < 0)
        return -1;

    return n;
}

void setup_network ()
{
    int n;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ERROR opening socket\n");
        return;
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));


    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
        printf("ERROR connecting\n");
}


int main(int argc, char *argv[])
{
    char reference[32];
    char command [32];
    char value[64];
    int counter = 0;
    wolk_ctx_t wolk;
    unsigned current_time = (unsigned)time(NULL);

    printf ("Wolk client - Establishing tcp connection\n");
    setup_network();

    wolk_set_protocol(&wolk, PROTOCOL_TYPE_JSON);
    printf ("Wolk client - Connecting to server\n");
    wolk_connect(&wolk, &send_buffer, &receive_buffer, serial_mqtt, password_mqtt);

    printf ("Wolk client - Seting actuator references\n");
    wolk_set_actuator_references (&wolk, 2, numeric_slider_reference, bool_switch_refernece);

    printf ("Wolk client - Seting numeric slider actuator with reference %s\n", numeric_slider_reference);
    if ( wolk_publish_num_actuator_status (&wolk, numeric_slider_reference, 0, ACTUATOR_STATUS_READY, current_time) != W_FALSE)
    {
        printf ("Wolk client - Numeric actuator status error\n");
    }
    printf ("Wolk client - Seting bool switch actuator with reference %s\n", bool_switch_refernece);
    if ( wolk_publish_bool_actuator_status (&wolk,bool_switch_refernece, false, ACTUATOR_STATUS_READY, current_time) != W_FALSE)
    {
        printf ("Wolk client - Bool actuator status error\n");
    }


    while (!toStop)	{
        memset (reference, 0, 32);
        memset (command, 0, 32);
        memset (value, 0, 64);
        wolk_receive (&wolk, 1);

        while (wolk_read_actuator (&wolk, command, reference, value)!= W_TRUE)
        {
            printf ("Wolk client - Received: \n Command %s \n Actuator refernece %s \n Value received %s\n", command, reference, value);

            if (strcmp(reference,"SW")==0)
            {
                if (strcmp(value,"true")==0)
                {
                    if ( wolk_publish_bool_actuator_status (&wolk,"SW", true, ACTUATOR_STATUS_READY, 0) != W_FALSE)
                    {
                        printf ("Wolk client - Bool actuator status error\n");
                    }

                } else if (strcmp(value,"false")==0)
                {
                    if ( wolk_publish_bool_actuator_status (&wolk,"SW", false, ACTUATOR_STATUS_READY, 0) != W_FALSE)
                    {
                        printf ("Wolk client - Bool actuator status error\n");
                    }

                }
            } else if (strcmp(reference,"SL")==0)
            {
                int num_value = atof(value);
                if ( wolk_publish_num_actuator_status (&wolk,"SL", num_value, ACTUATOR_STATUS_READY, 0) != W_FALSE)
                {
                    printf ("Wolk client - Numeric actuator status error\n");
                }

            }

        }

        if (counter==0)
        {
            if (wolk_publish_single (&wolk, "TS","Mark 1", DATA_TYPE_STRING, 0))
            {
                printf ("Wolk client - Publishing single reading error\n");
            }


            if (wolk_publish_single (&wolk, "TN","22", DATA_TYPE_NUMERIC, 0) != W_FALSE)
            {
                printf ("Wolk client - Publishing single reading error\n");
            }

            if (wolk_publish_single (&wolk, "TB","false", DATA_TYPE_BOOLEAN, 0))
            {
                printf ("Wolk client - Publishing single reading error\n");
            }


            if ( wolk_add_string_reading(&wolk, "TS", "Periodic", 0) != W_FALSE)
            {
                printf ("Wolk client - Adding string reading error\n");
            }
            if ( wolk_add_numeric_reading(&wolk, "TN", 11, 0)!= W_FALSE)
            {
                printf ("Wolk client - Adding numeric reading error\n");
            }
            if ( wolk_add_bool_reading(&wolk, "TB", false, 0)!= W_FALSE)
            {
                printf ("Wolk client - Adding bool reading error\n");
            }

            if ( wolk_publish (&wolk)!= W_FALSE)
            {
                printf ("Wolk client - Publish readings error\n");
            }

            //Every 60 seconds send keep alive message
            if (wolk_keep_alive (&wolk)!= W_FALSE)
            {
                printf("Wolk client -Keep alive error\n");
            }
        }

        counter ++;
        if (counter == 10)
        {
            counter = 0;
        }

    }

    printf("Wolk client - Diconnecting\n");

    wolk_disconnect(&wolk);

    close(sockfd);

    return 0;
}


