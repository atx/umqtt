/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014-2015 Josef Gajdusek
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * */

#include <alloca.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "umqtt.h"

struct option long_options[] = {
	{"topic",	required_argument,	0, 't'},
	{"host",	required_argument,	0, 'h'},
	{"port",	required_argument,	0, 'p'},
	{"message",	required_argument,	0, 'm'},
	{"id",		required_argument,	0, 'i'},
	{"verbose",	no_argument,		0, 'v'},
	{"help",	no_argument,		0, 'u'},
};

const char *usage =
	"usage: umqtt -t <topic> [-h hostname] [-p port] [-m message] [-i client-id] [-v]\n";

bool verbose = false;

uint8_t u_rxbuff[2048];
uint8_t u_txbuff[2048];

void message_callback(struct umqtt_connection *uc, char *topic, uint8_t *data,
		int len)
{
	printf("%s %s\n", topic, data);
}

struct umqtt_connection u_conn = {
	.kalive = 30,
	.txbuff = {
		.start = u_txbuff,
		.length = sizeof(u_txbuff),
	},
	.rxbuff = {
		.start = u_rxbuff,
		.length = sizeof(u_rxbuff),
	},
	.message_callback = message_callback,
};

int main(int argc, char *argv[])
{
	struct sockaddr_in serveraddr;
	struct hostent *server;
	struct timeval timeout;
	char *host = "localhost";
	char *msg = NULL;
	char *topic = NULL;
	char *clid = NULL;
	uint8_t buff[1024];
	int port = 1883;
	int sockfd;
	uint16_t kalive = 30;
	int digit_optind = 0;
	int ret;
	int c;

	/* Parse command line options */
	while (true) {
		int option_index = 0;
		c = getopt_long(argc, argv, "h:p:m:t:i:v", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 'h':
				host = optarg;
				break;
			case 'p':
				sscanf(optarg, "%d", &port);
				break;
			case 'm':
				msg = optarg;
				break;
			case 't':
				topic = optarg;
				break;
			case 'v':
				verbose = true;
				break;
			case 'i':
				if (strlen(clid) > 23) {
					fprintf(stderr, "Client id length cannot exceed 23 characters\n");
					exit(EXIT_FAILURE);
				}
				clid = optarg;
				break;
			case 0:
				puts(usage);
				exit(EXIT_SUCCESS);
		}
	}

	if (topic == NULL) {
		fprintf(stderr, "No topic specified!\n");
		exit(EXIT_FAILURE);
	}

	server = gethostbyname(host);

	bzero((char *) &serveraddr, sizeof(serveraddr));
	memcpy((char *) server->h_addr, (char *)&serveraddr.sin_addr.s_addr,
			server->h_length);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
		perror("Failed to connect");

	if (msg == NULL) {
		timeout.tv_sec = kalive;
		timeout.tv_usec = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	}

	/* Initialize umqtt */
	if (clid == NULL) {
		srand(time(NULL));
		clid = malloc(20);
		snprintf(clid, 20, "umqtt-%d", rand());
	}
	u_conn.clientid = clid;

	if (verbose)
		printf("Connecting to %s:%d with client id \"%s\"\n", host, port, clid);


	umqtt_connect(&u_conn);

	/* Enter main loop */
	while (true) {
		if (!umqtt_circ_is_empty(&u_conn.txbuff)) {
			ret = umqtt_circ_pop(&u_conn.txbuff, buff, 1024);
			send(sockfd, buff, ret, 0);	
		}

		if (umqtt_circ_is_empty(&u_conn.txbuff) && topic == NULL && msg != NULL)
			break;

		ret = recv(sockfd, buff, sizeof(buff), 0);
		if (ret < 0) {
			if (errno == EAGAIN) {
				umqtt_ping(&u_conn);
			} else {
				perror("Failed receiving");
				exit(EXIT_FAILURE);
			}
		}
		umqtt_circ_push(&u_conn.rxbuff, buff, ret);
		umqtt_process(&u_conn);
	
		if (topic != NULL && u_conn.state == UMQTT_STATE_CONNECTED) {
			if (verbose)
				printf("Connected\n");

			if (msg != NULL) {
				umqtt_publish(&u_conn, topic, msg, strlen(msg));
			} else {
				umqtt_subscribe(&u_conn, topic);
				if (verbose)
					printf("Subscribing to %s\n", topic);
			}
			topic = NULL;
		}
	}

	close(sockfd);
}
