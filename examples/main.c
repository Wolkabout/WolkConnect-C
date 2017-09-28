

/*******************************************************************************
 * Copyright (c) 2012, 2016 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Ian Craggs - change delimiter option from char to string
 *    Al Stockdill-Mander - Version using the embedded C client
 *    Ian Craggs - update MQTTClient function names
 *******************************************************************************/

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

#include "WolkConn.h"

#include "MQTTPacket.h"
#include "transport.h"

#include <signal.h>

#include <sys/time.h>

#define BUFSIZE 1024

#define BUFFER_SIZE 256
#define CLIENT_ID "mikroe"
#define KEEPALIVE_INTERVAL 20

#define RESP_FALSE -1
#define RESP_TRUE 0

static int socket_mqtt = -1;
static unsigned char buffer[BUFFER_SIZE];
static int buffer_length = sizeof(buffer);


const char *serial_mqtt = "3332223";
const char *topic = "sensors/3332223";
const char *password_mqtt = "6099b9e6-f968-47a7-ad74-5ce863606d8b";
//const char *message = "RTC 1463400060;READINGS R:1463400000,C:+100;";
const char *message = "STATUS S:true:READY;";
const char *topic_sub = "config/3332223";

int sockfd;




static volatile int toStop = 0;

enum states { READING, PUBLISHING };

////////////////////////
/// \brief send_buffer_wifi
/// \param buffer
/// \param len
/// \return


//typedef uint32_t key_t;


#define list_size 8
#define mem_noown 0x0
#define mem_using 0x1
#define failed (~0x0)
#ifndef nullptr
#define nullptr NULL
#endif // !nullptr




int send_buffer_wifi(unsigned char* buffer, unsigned int len)
{
    /* send the message line to the server */
    int n = write(sockfd, buffer, len);
    if (n < 0) 
      return RESP_FALSE;

    return n;
}

static int receive_buffer_wifi(unsigned char* buffer, unsigned int max_bytes)
{
    /*if (receive_message_wifi(buffer, (signed int)max_bytes) == RESP_TRUE)
    {
        return strlen(buffer);
    }*/
    bzero(buffer, max_bytes);
    //   printf ("Receive buffer read\n");
    int n = read(sockfd, buffer, max_bytes);
    if (n < 0) 
      return RESP_FALSE;

    return n;
}

void setup_network ()
{
    int portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;


    hostname = "integration.wolksense.com";
    portno = 1883;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

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
        error("ERROR connecting");
}

int main(int argc, char *argv[])
{

    wolk_ctx_t wolk;

    setup_network();

    wolk_connect(&wolk, &send_buffer_wifi, &receive_buffer_wifi, serial_mqtt, password_mqtt);



    char reference[32];
    char command [32];
    char value[64];




    //topicString.cstring = topic_sub;
    //topicString.cstring = topic;
    //state = READING;
    while (!toStop)	{
        //sleep(1);
        wolk_receive (&wolk,3);

        memset (reference, 0, 32);
        memset (command, 0, 32);
        memset (value, 0, 64);

        if (wolk_read_actuator (&wolk, command, reference, value)!= W_TRUE)
        {
            printf ("Command from queue: %s\n", command);
            printf ("Reference from queue: %s\n", reference);
            printf ("Value from queue: %s\n", value);
        }


//        /* do other stuff here */
//        switch(state){
//        case READING:
//            sleep(1);
//            if((rc=MQTTPacket_readnb(buf, buflen, &(wolk.mytransport))) == PUBLISH){
//                unsigned char dup;
//                int qos;
//                unsigned char retained;
//                unsigned short msgid;
//                int payloadlen_in;
//                unsigned char* payload_in;
//                int rc;
//                MQTTString receivedTopic;

//                rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
//                        &payload_in, &payloadlen_in, buf, buflen);
//                printf("message arrived %.*s\n", payloadlen_in, payload_in);
//                printf("publishing reading\n");
//                state = PUBLISHING;
//                len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)message, strlen(message));
//                transport_sendPacketBuffernb_start(wolk.sock, buf, len);
//            } else if(rc == -1){
//                /* handle I/O errors here */
//            }	/* handle timeouts here */
//            break;
//        case PUBLISHING:
//            //Check readings , ...
//            switch(transport_sendPacketBuffernb(wolk.sock)){
//            case TRANSPORT_DONE:
//                printf("Published\n");
//                state = READING;
//                toStop =1;
//                break;
//            case TRANSPORT_ERROR:
//                /* handle any I/O errors here */
//                break;
//            case TRANSPORT_AGAIN:
//            default:
//                /* handle timeouts here, not probable unless there is a hardware problem */
//                break;
//            }
//            break;
//        }
    }

    printf("disconnecting\n");

    wolk_disconnect(&wolk);

    close(sockfd);

    return 0;
}


