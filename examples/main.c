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
#include <pthread.h>

#include "WolkConn.h"

#include "MQTTPacket.h"
#include "transport.h"

#include <signal.h>

#include <sys/time.h>

#define BUFFER_SIZE 256
#define STR_16 16

static unsigned char buffer[BUFFER_SIZE];
static int buffer_length = sizeof(buffer);
int sockfd;


//const char *device_key = "222111";
//const char *password = "dshuojw0a932ui5j";
const char *device_key = "123567";
const char *password = "4c098907-0a23-4246-8e3d-fdf4e73409a1";
const char *hostname = "integration.wolksense.com";
int portno = 1883;
const char *numeric_slider_reference = "SL";
const char *bool_switch_refernece = "SW";
wolk_ctx_t wolk;

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

/* this function is run by the second thread */
void *input_thread(void *arg)
{
    char selection[STR_16];
    char reference[STR_16];
    char value[STR_64];
    while (1)
    {
        memset (selection, 0, STR_16);
        printf ("Wolk client - Control interface\n");
        printf ("Wolk client - Available commands:\n ");
        printf ("\tWolk client - A - Add reading\n");
        printf ("\tWolk client - PS - Publish single reading\n");
        printf ("\tWolk client - CL - Clear readings\n");
        printf ("\tWolk client - P - Publish acumulated readings\n");
        printf ("\tWolk client - Q - Quit\n");
        printf("Wolk client - Command:");
        scanf("%s", selection);
        if (strcmp(selection, "A")==0)
        {
            printf ("Wolk client - Add reading\n");
            printf("\tWolk client - Enter reading type (string/bool/numeric):");
            memset (selection, 0, STR_16);
            memset (reference, 0, STR_16);
            memset (value, 0, STR_64);
            scanf("%s", selection);
            printf ("\tWolk client - Enter reference:");
            scanf("%s", reference);
            printf ("\tWolk client - Enter value:");
            scanf("%s", value);
            if (strcmp(selection,"string")==0)
            {
                if ( wolk_add_string_reading(&wolk, reference, value, 0) != W_FALSE)
                {
                    printf ("Wolk client - Adding string reading error\n");
                }
            }else if (strcmp(selection,"bool")==0)
            {
                if (strcmp(value,"true")==0)
                {
                    if ( wolk_add_bool_reading(&wolk, reference, true, 0)!= W_FALSE)
                    {
                        printf ("Wolk client - Adding bool reading error\n");
                    }
                } else if (strcmp(value,"false")==0)
                {
                    if ( wolk_add_bool_reading(&wolk, reference, false, 0)!= W_FALSE)
                    {
                        printf ("Wolk client - Adding bool reading error\n");
                    }
                }
            }else if (strcmp(selection,"numeric")==0)
            {
                int num_value = atof(value);
                if ( wolk_add_numeric_reading(&wolk, reference, num_value, 0)!= W_FALSE)
                {
                    printf ("Wolk client - Adding numeric reading error\n");
                }
            }
        } else if (strcmp(selection, "PS")==0)
        {
            printf ("Wolk client - Publish single reading\n");
            printf("\tWolk client - Enter reading type (string/bool/numeric):");
            memset (selection, 0, STR_16);
            memset (reference, 0, STR_16);
            memset (value, 0, STR_64);
            scanf("%s", selection);
            printf ("\tWolk client - Enter reference:");
            scanf("%s", reference);
            printf ("\tWolk client - Enter value:");
            scanf("%s", value);
            if (strcmp(selection,"string")==0)
            {
                if (wolk_publish_single (&wolk, reference, value, DATA_TYPE_STRING, 0)!= W_FALSE)
                {
                    printf ("Wolk client - Publishing single reading error\n");
                }
            }else if (strcmp(selection,"bool")==0)
            {
                if (wolk_publish_single (&wolk, reference, value, DATA_TYPE_BOOLEAN, 0)!= W_FALSE)
                {
                    printf ("Wolk client - Publishing single reading error\n");
                }
            }else if (strcmp(selection,"numeric")==0)
            {
                if (wolk_publish_single (&wolk, reference, value, DATA_TYPE_NUMERIC, 0)!= W_FALSE)
                {
                    printf ("Wolk client - Publishing single reading error\n");
                }
            }
        } else if (strcmp(selection, "CL")==0)
        {
            if (wolk_clear_readings(&wolk) != W_FALSE)
            {
                 printf ("Wolk client - Clear readings error\n");
            }
        } else if (strcmp(selection, "P")==0)
        {
            if ( wolk_publish (&wolk)!= W_FALSE)
            {
                printf ("Wolk client - Publish readings error\n");
            }
        } else if (strcmp(selection, "Q")==0)
        {
            toStop = 1;
            break;
        }

    }
    return NULL;

}


int main(int argc, char *argv[])
{
    char reference[32];
    char command [32];
    char value[64];
    int counter = 0;
    unsigned current_time = (unsigned)time(NULL);

    printf ("Wolk client - Establishing tcp connection\n");
    setup_network();

    wolk_set_protocol(&wolk, PROTOCOL_TYPE_JSON);
    printf ("Wolk client - Connecting to server\n");
    wolk_connect(&wolk, &send_buffer, &receive_buffer, device_key, password);

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

    /* this variable is our reference to the second thread */
    pthread_t wolk_thread;

    /* create a second thread which executes inc_x(&x) */
    if(pthread_create(&wolk_thread, NULL, input_thread, NULL))
    {
        fprintf(stderr, "Error creating thread\n");
        return 1;
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


    if(pthread_join(wolk_thread, NULL))
    {
        fprintf(stderr, "Error joining thread\n");
        return 1;
    }
    printf("Wolk client - Diconnecting\n");

    wolk_disconnect(&wolk);

    close(sockfd);

    return 0;
}


