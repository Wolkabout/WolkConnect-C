

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

static int send_buffer_wifi(unsigned char* buffer, unsigned int len)
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
    int n = read(sockfd, buffer, max_bytes);
    if (n < 0) 
      return RESP_FALSE;

    return n;
}

int main(int argc, char *argv[])
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

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR connecting");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc = 0;
    int mysock = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int msgid = 1;
    MQTTString topicString = MQTTString_initializer;
    int req_qos = 0;

    int payloadlen = strlen(message);
    int len = 0;
    MQTTTransport mytransport;
    int state = READING;
    transport_iofunctions_t iof = {send_buffer_wifi, receive_buffer_wifi};

    mysock = transport_open(&iof);
    if(mysock < 0)
        return mysock;

    mytransport.sck = &mysock;
    mytransport.getfn = transport_getdatanb;
    mytransport.state = 0;
    data.clientID.cstring = serial_mqtt;
    data.keepAliveInterval = KEEPALIVE_INTERVAL;
    data.cleansession = 1;
    data.username.cstring = serial_mqtt;
    data.password.cstring = password_mqtt;


    len = MQTTSerialize_connect(buf, buflen, &data);
    /* This one blocks until it finishes sending, you will probably not want this in real life,
    in such a case replace this call by a scheme similar to the one you'll see in the main loop */
    rc = transport_sendPacketBuffer(mysock, buf, len);


    printf("MQTT connected\n");
    /* subscribe */
    topicString.cstring = topic_sub;
    len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);

    /* This is equivalent to the one above, but using the non-blocking functions. You will probably not want this in real life,
    in such a case replace this call by a scheme similar to the one you'll see in the main loop */
    transport_sendPacketBuffernb_start(mysock, buf, len);
    while((rc=transport_sendPacketBuffernb(mysock)) != TRANSPORT_DONE);


    /* loop getting msgs on subscribed topic */
    //topicString.cstring = topic_sub;
    topicString.cstring = topic;
    state = READING;
    while (!toStop)	{
        /* do other stuff here */
        switch(state){
        case READING:
            if((rc=MQTTPacket_readnb(buf, buflen, &mytransport)) == PUBLISH){
                unsigned char dup;
                int qos;
                unsigned char retained;
                unsigned short msgid;
                int payloadlen_in;
                unsigned char* payload_in;
                int rc;
                MQTTString receivedTopic;

                rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                        &payload_in, &payloadlen_in, buf, buflen);
                printf("message arrived %.*s\n", payloadlen_in, payload_in);
                printf("publishing reading\n");
                state = PUBLISHING;
                len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)message, strlen(message));
                transport_sendPacketBuffernb_start(mysock, buf, len);
            } else if(rc == -1){
                /* handle I/O errors here */
                goto exit;
            }	/* handle timeouts here */
            break;
        case PUBLISHING:
            //Check readings , ...
            switch(transport_sendPacketBuffernb(mysock)){
            case TRANSPORT_DONE:
                printf("Published\n");
                state = READING;
                break;
            case TRANSPORT_ERROR:
                /* handle any I/O errors here */
                goto exit;
                break;
            case TRANSPORT_AGAIN:
            default:
                /* handle timeouts here, not probable unless there is a hardware problem */
                break;
            }
            break;
        }
    }

    printf("disconnecting\n");
    len = MQTTSerialize_disconnect(buf, buflen);
    /* Same blocking related stuff here */
    rc = transport_sendPacketBuffer(mysock, buf, len);

exit:
    transport_close(mysock);

    return 0;
}


