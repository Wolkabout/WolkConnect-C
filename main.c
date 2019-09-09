/*
 * Copyright 2018 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wolk_connector.h"
#include "wolk_utils.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

#include <openssl/ssl.h>
#include <assert.h>
#include <unistd.h>
//#include <fcntl.h>


#define BUFSIZE 64 

static void error_fatal(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}



char ip2[BUFSIZE];
char command[256] = "sensors | grep -i \"Core [0-9]\" | cut -d \"+\" -f2 | cut -d ' ' -f1 | grep -o '[0-9][0-9].[0-9]' >REZULTAT";
char command1[256] = "ifconfig| grep -i  \"inet\"|sed -n '3p'| cut -d 't' -f2| cut -d ' ' -f2>ipadresa";
char command2[256] = "ifconfig| grep -i  \"inet\"|sed -n '3p'| cut -d 't' -f2| cut -d ' ' -f2>ipadresa1";
static SSL_CTX* ctx;
static BIO* sockfd;

static const char *device_key = "95gq1jxn7m61hm5b";
static const char *device_password = "3148e635-b390-47c9-93f4-e67f004beedc";
static const char *hostname = "api-demo.wolkabout.com";
static int portno = 8883;
static char certs[] = "../ca.crt";

/* Sample in-memory persistence storage - size 1MB */
static uint8_t persistence_storage[1024*1024];

/* WolkConnect-C Connector context */
static wolk_ctx_t wolk;

static volatile bool keep_running = true;


static void int_handler(int dummy)
{
	WOLK_UNUSED(dummy);

	keep_running = false;
}



double get_cpu_temperature()

{	FILE *fp;
	char s[BUFSIZE];

	double d, sum=0;
	int cnt = 0;
	double t;
	if(system(command) != 0)
		error_fatal("system");

	if(( fp = fopen("REZULTAT", "r")) == NULL )
		error_fatal("fopen");	
	while(fgets(s, BUFSIZE, fp) != NULL)
	{
		d = atof(s);
		sum += d;

		++cnt;
	}

	if(cnt != 0){
		t = (double)(sum/cnt);
		if(fclose(fp) != 0)
			error_fatal("fclose");
		return t;
	}
	else 
		error_fatal("temperature");



}


void update_secondip(void)
{
	puts("Update second ip2 with update_secondip() call...");
	FILE *fp2;

	if(system(command2) != 0)
		error_fatal("system");
	if(( fp2 = fopen("ipadresa1", "r")) == NULL )
		error_fatal("fopen");
	memset(ip2,'\0',BUFSIZE);
	fscanf(fp2, "%s", ip2);
	if(fclose(fp2) != 0)
		error_fatal("fclose");
	printf("ip2 was updated on %s\n", ip2);

}


static int send_buffer(unsigned char* buffer, unsigned int len)
{
	int n;

	n = (int)BIO_write(sockfd, buffer, len);
	if (n < 0) {
	}

	return n;
}

static int receive_buffer(unsigned char* buffer, unsigned int max_bytes)
{
	bzero(buffer, max_bytes);
	int n;

	n = (int)BIO_read(sockfd, buffer, max_bytes);
	if (n < 0) {
		return -1;
	}

	return n;
}

static void open_socket(BIO** bio, SSL_CTX** ssl_ctx, const char* addr, const int portno, const char* ca_file, const char* ca_path) {
	SSL* ssl;
	const char port[5];
	snprintf(port, 5, "%d", portno);

	/* Load OpenSSL */
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	SSL_library_init();

	*ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	/* Load Certificate */
	if (!SSL_CTX_load_verify_locations(*ssl_ctx, ca_file, ca_path)) {
		printf("Wolk client - Error, failed to load certificate\n");
		exit(1);
	}

	/* Open BIO Socket */
	*bio = BIO_new_ssl_connect(*ssl_ctx);
	BIO_get_ssl(*bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(*bio, addr);
	BIO_set_nbio(*bio, 1);
	BIO_set_conn_port(*bio, port);

	/* Wait for connection in 15 second timeout */
	int start_time = time(NULL);
	while(BIO_do_connect(*bio) <= 0 && (int)time(NULL) - start_time < 15);
	if (BIO_do_connect(*bio) <= 0) {
		printf("Wolk client - Error, do connect\n");
		BIO_free_all(*bio);
		SSL_CTX_free(*ssl_ctx);
		*bio = NULL;
		*ssl_ctx=NULL;
		return;
	}

	/* Verify Certificate */
	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		printf("Wolk client - Error, x509 certificate verification failed\n");
		exit(1);
	}
}
static char device_configuration_references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE] = {"LOG_LEVEL"};
static char device_configuration_values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE] = {"Init"};

static void configuration_handler(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                  char (*value)[CONFIGURATION_VALUE_SIZE],
                                  size_t num_configuration_items)
{
    for (size_t i = 0; i < num_configuration_items; ++i) {
        printf("Configuration handler - Reference: %s | Value: %s\n", reference[i], value[i]);
        
        strcpy(device_configuration_references[i], reference[i]);
        strcpy(device_configuration_values[i], value[i]);
	
    }
}

static size_t configuration_provider(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                     char (*value)[CONFIGURATION_VALUE_SIZE],
                                     size_t max_num_configuration_items)
{
    WOLK_UNUSED(max_num_configuration_items);
    WOLK_ASSERT(max_num_configuration_items >= NUMBER_OF_CONFIGURATION);
    
    for (size_t i = 0; i < CONFIGURATION_ITEMS_SIZE; ++i) {
	 printf("Configuration handler - Reference: %s | Value: %s\n", reference[i], value[i]);
        strcpy(reference[i], device_configuration_references[i]);
        strcpy(value[i], device_configuration_values[i]);

	
    }
   
    return CONFIGURATION_ITEMS_SIZE;//change to 1.
}

int main(int argc, char *argv[])
{ 
	FILE *fp1,*fplog;
	char ip1[BUFSIZE];
	char s[BUFSIZE];
	int lip1,lip2;
	double max=0,temp=0;
	int i,tp=0;
	WOLK_UNUSED(argc);
	WOLK_UNUSED(argv);
	assert(argc == 1);





	

	signal(SIGINT, int_handler);

	if (strcmp(device_key, "device_key") == 0) {
		printf ("Wolk client - Error, device key not provided\n");
		return 1;
	}

	if (strcmp(device_password, "password") == 0) {
		printf ("Wolk client - Error, password key not provided\n");
		return 1;
	}

	printf ("Wolk client - Establishing connection to WolkAbout IoT platform\n");
	open_socket(&sockfd, &ctx, hostname, portno, certs, NULL);
	if (sockfd == NULL) {
		printf ("Wolk client - Error establishing connection to WolkAbout IoT platform\n");
		return 1;
	}

	if (wolk_init(&wolk,
				send_buffer, receive_buffer,
				NULL, NULL/*ACTUATOR_STATE_READY*/
				,configuration_handler, configuration_provider,	/*NULL,NULL,*/
				device_key, device_password,
				PROTOCOL_JSON_SINGLE,
				NULL, NULL/*actuator_references, num_actuator_references*/) != W_FALSE) {
		printf("Error initializing WolkConnect-C\n");
		return 1;
	}

	if (wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false) != W_FALSE) {
		printf("Error initializing in-memory persistence\n");
		return 1;
	}

	printf ("Wolk client - Connecting to server\n");
	if (wolk_connect(&wolk) != W_FALSE) {
		printf ("Wolk client - Error connecting to server\n");
		return -1;
	}
	printf ("Wolk client - Connnected to server\n");
	
	if(system(command1) != 0)
		error_fatal("system");
	
	if(( fp1 = fopen("ipadresa", "r")) == NULL )
		error_fatal("fopen");
	memset(ip1, '\0', BUFSIZE);
	fscanf(fp1, "%s", ip1);
	printf("ip1 = %s\n", ip1);
	

	if(fclose(fp1) != 0)
		error_fatal("fclose");
	if(( fplog = fopen("logs", "a")) == NULL )
		error_fatal("fopen");
	fputs(ip1,fplog);fputc('\n',fplog);
	
	
strcpy(device_configuration_values,ip1);
wolk_publish_configuration_status(&wolk, device_configuration_references,device_configuration_values,CONFIGURATION_ITEMS_SIZE);

	while (keep_running) {
		//MANDATORY: sleep(currently 200us) and number of tick(currently 5) when are multiplied needs to give 1ms.
		// you can change this parameters, but keep it's multiplication
		
		for(i=0;i<5;i++)
		{
		temp=get_cpu_temperature();
		/*sprintf(s,"\nCPU_TEMP:%f\n",temp);*/gcvt(temp,5,s);puts(s);
		fputs(s,fplog);fputc('\n',fplog);
		//fflush(stdin);
		//strcpy(ip1,s);
		
		if(temp>max)
			max=temp;
		
		
		sleep(60);
strcpy(device_configuration_values,s);
wolk_publish_configuration_status(&wolk, device_configuration_references,device_configuration_values,CONFIGURATION_ITEMS_SIZE);

}
		wolk_add_numeric_sensor_reading(&wolk, "CPU_T", max, 0);
		fputs("\nMAX_CPU_TEMP:",fplog);gcvt(max,5,s);puts(s);
		fputs(s,fplog);fputc('\n',fplog);
		
		update_secondip();
		printf("After update_secondip(): ip2 = %s\n", ip2);
		
		if(strcmp(ip1,ip2)!=0)
		{fputs("\nAddress has changed.\n",fplog);
			memset(ip1, '\0', BUFSIZE);
			strcpy(ip1,ip2);
		fputs(ip1,fplog);fputc('\n',fplog);			
		wolk_add_string_sensor_reading(&wolk, "IP_ADD", ip1, 0);
strcpy(device_configuration_values,ip1);
wolk_publish_configuration_status(&wolk, device_configuration_references,device_configuration_values,CONFIGURATION_ITEMS_SIZE);
		}
		
		
		
			
		


		
		wolk_publish(&wolk);
		temp=0;
		max=0;

		wolk_process(&wolk, 5);
	}
	if(fclose(fplog) != 0)
		error_fatal("fclose");
	printf("Wolk client - Diconnecting\n");

	wolk_disconnect(&wolk);

	close(sockfd);

	return 0;
}
