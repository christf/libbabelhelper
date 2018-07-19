/*
   Copyright (c) 2017,2018 Christof Schulze <christof.schulze@gmx.net>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */


#include <libbabelhelper/babelhelper.h>
#include <sys/time.h>
#include <ctype.h>

static const char *BABEL_TOKEN_STRING[] = {
	FOREACH_BABEL_TOKEN(GENERATE_STRING)
};

bool babelhelper_generateip(char *result, const unsigned char *mac, const char *prefix){
	unsigned char buffer[8];
	struct in6_addr dst = {};

	if (! inet_pton(AF_INET6, prefix, &(dst.s6_addr))) {
		fprintf(stderr, "inet_pton failed in babelhelper_generateip on address %s.\n",prefix);
		return false;
	}

	memcpy(buffer,mac,3);
	buffer[3]=0xff;
	buffer[4]=0xfe;
	memcpy(&(buffer[5]),&(mac[3]),3);
	buffer[0] ^= 1 << 1;

	memcpy(&(dst.s6_addr[8]), buffer, 8);
	inet_ntop(AF_INET6, &(dst.s6_addr), result, INET6_ADDRSTRLEN);

	return true;
}

bool babelhelper_generateip_str(char *result,const char *stringmac, const char *prefix) {
	unsigned char mac[6];
	sscanf(stringmac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &(mac[0]), &(mac[1]), &(mac[2]), &(mac[3]), &(mac[4]), &(mac[5]));
	return babelhelper_generateip(result, mac, prefix);
}

char *tolower_s(char *str) {
	for(int i = 0; str[i]; i++){
		str[i] = tolower(str[i]);
	}
	return str;
}

int gettoken(char* token) {

	// TODO: how can we do this more efficiently?
	for (int i=0;i<num_different_tokens;i++) {
		char *lowerstring = tolower_s(strdup(BABEL_TOKEN_STRING[i])); 
		if (!strncmp(token, lowerstring, strlen(BABEL_TOKEN_STRING[i])) ) {
			free(lowerstring);
			return i;
		}
		free(lowerstring);
	}
	return UNKNOWN;
}

void printifdefined(int token, char **babeldata) {
	if (babeldata[token] && *babeldata[token])
		printf("%s: %s ", BABEL_TOKEN_STRING[token], babeldata[token]);
}

void printrecognized(char** babeldata){
	printf("parsed babeld-line contained the following known tokens: ");
	for (int i=0;i<num_different_tokens;i++)
		printifdefined(i, babeldata);
	printf("\n");
}

/* reallocates the buffer an shifts all the pointers addresses that are used
 * for the line parser
 * */
void realloc_and_compensate_for_move(char **buffer, size_t newsize, char **babeldata, char **token ) {

	char *oldpointer = *buffer;

	*buffer = realloc(*buffer, newsize);
	if (buffer == NULL) {
		perror("Cannot allocate buffer");
		exit(1);
	}

	for (int i=0;i<num_different_tokens;i++) {// compensate already found addresses for words for new position due to buffer realloc 
		if (babeldata[i])
			babeldata[i] += (*buffer - oldpointer);
	}
	if (*token)
		*token += (*buffer - oldpointer);
}

/* this will read data from a nonblocking socket, and parse this line-wise and
 * for babel tokens in one single loop
 * It will return an array of char* to each token
 */
bool babelhelper_input_pump(struct babelhelper_ctx *ctx, int fd,  void* obj, bool (*lineprocessor)(char** babeldata, void* object)) {
	char *buffer = NULL;
	size_t buffer_used = 0;
	ssize_t len = 0;
	bool waitingforclosingquote=false;
	bool foundverb = false;
	char *token = NULL;
	int lasti = 0;
	char *parseddata[num_different_tokens];
	memset(parseddata, 0 , sizeof(parseddata));
	bool exit_success = false;
//	fd_set rfds;
//	FD_ZERO(&rfds);
//	FD_SET(fd, &rfds);
	do {
		realloc_and_compensate_for_move(&buffer, buffer_used + LINEBUFFER_SIZE + 1, (void*)&parseddata, &token);
		len = read(fd, buffer + buffer_used, LINEBUFFER_SIZE);

		if ( (len == -1 && errno == EAGAIN) ) {
			exit_success = true; // no more data, 
			break;
		}
		else if ( len == 0 ) {
			break; // end of file - we should re-connect
		} else if (len < 0 && errno > 0 ) {
			perror("error when reading from babel socket");
		} else if (len > 0 ) {
			buffer[buffer_used + len] = 0; // terminate string appropriately. This will be overwritten if more data is read.
			buffer_used = buffer_used + len;
			int i=0;
			while ( buffer_used > 0 ) {
				switch (buffer[i]) {
					case '"':
						waitingforclosingquote = !waitingforclosingquote;
						break;
					case '\n':
					case '\r':
						buffer[i]='\0';
						if (!strncmp(buffer, "ok", 2)) {
							exit_success = true;
							goto out;
						}
						if (token) {
							parseddata[gettoken(token)] = &buffer[lasti];
						}

						lineprocessor(parseddata, obj);
						buffer_used-=i;
						memmove(buffer, &buffer[i+1], buffer_used);
						buffer_used--; // when copying we omitted 1 byte.

						memset(parseddata, 0 , sizeof(parseddata));
						i = lasti = 0;
						token=NULL;
						waitingforclosingquote = foundverb = false;
						break;
					case '\t':
					case ' ':
						if (!foundverb) { // first word on line
							buffer[i]='\0';
							foundverb = true;
							(parseddata[VERB]) = &buffer[0];
						}

						else if (!waitingforclosingquote) { // after the first word, everything else forms pairs of token, value.
							buffer[i]='\0';
							if (!token) // no token known yet, this word must be a token.
								token=&buffer[lasti];
							else { // we already know a token so this word must be a value
								parseddata[gettoken(token)] = &buffer[lasti];
								token = NULL;
								}

							}
							lasti = i + 1;
							break;
					}
					if (i >= buffer_used-1) {
						break; // incomplete line due to INPUT_BUFFER_SIZE_LIMITATION - read some more data, then repeat parsing.
					}
					i++;
				}
			} else if (len == 0 ) {
				fprintf(stderr, "didn't read anything but no error %i\n", errno);
			}
	} while ( buffer_used > 0 || len > 0 );

out:
	free(buffer);
	return exit_success;
}

int babelhelper_babel_connect(int port) {
	int sockfd ;
	fd_set rfds;
	FD_ZERO(&rfds);

	struct sockaddr_in6 serv_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(port)
	};

	sockfd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		return -1;
	}
	FD_SET(sockfd, &rfds);
	if (inet_pton(AF_INET6, "::1", &serv_addr.sin6_addr.s6_addr) != 1)
	{
		perror("Cannot parse hostname");
		return -1;
	}
	if (connect(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		if (errno != EINPROGRESS) {
			perror("Can not connect to babeld");
			goto errorout;
		} else {
			struct timeval timeout =  {
				.tv_sec=5,
				.tv_usec=0,
			};
			if (select(sockfd +1, NULL, &rfds, NULL, &timeout) < 0) {
				perror("error on select when connecting to babel socket");
				goto errorout;
			} else {
				socklen_t len = sizeof(errno);

				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &errno, &len) < 0)
					goto errorout;

				if (errno == 0 )
					return sockfd;

				perror("Could not connect");
				goto errorout;
			}
		}
	}
	return sockfd;
errorout:
	close(sockfd);
	return -1;
}

int babelhelper_sendcommand(struct babelhelper_ctx *ctx, int fd, char *command) {
	int cmdlen = strlen(command);
	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	struct timeval timeout =  {
		.tv_sec=5,
		.tv_usec=0,
	};

	int retval = select(fd+1, NULL, &wfds, NULL, &timeout);

	if (retval == -1) {
		perror("select()");
		return 0;
	}
	else if (retval) {
		while (send(fd, command, cmdlen, 0) != cmdlen) {
			perror("Select said the babel socket is ready for writing but we received an error while sending command %s to babel. This should not happen. Retrying.");
			usleep(500000);
		}
	}
	else {
		if (ctx->debug)
			fprintf(stderr, "could not write command to babel socket within 5 seconds.\n");
		return 0;
	}

	return cmdlen;
}


bool babelhelper_discard_response(char **data, void *object) {
	return (!!data[OK]);
}

void babelhelper_readbabeldata(struct babelhelper_ctx *ctx,void *object, bool (*lineprocessor)(char**, void* object)) {
	int sockfd;
	fd_set rfds;
	FD_ZERO(&rfds);
	do {
		sockfd = babelhelper_babel_connect(BABEL_PORT);
		if (sockfd < 0) {
			fprintf(stderr, "Connecting to babel socket failed. Retrying.\n");
			usleep(1000000);
		}
	} while (sockfd < 0);

	FD_SET(sockfd, &rfds);

	// receive and ignore babel header
	while (true) {
		if ( babelhelper_input_pump(ctx, sockfd, NULL, babelhelper_discard_response))
			break;

		if (select(sockfd +1, &rfds, NULL, NULL, NULL) < 0) {
			perror("select:");
		};
	}

	// query babel data
	if ( babelhelper_sendcommand(ctx, sockfd, "dump\n") != 5 ) {
		fprintf(stderr, "Retrying to send dump-command to babel socket.\n");
		goto cleanup;
	}

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	// receive result
	while (true) {
		if ( babelhelper_input_pump(ctx, sockfd, object, lineprocessor))
			break;

		if (select(sockfd +1, &rfds, NULL, NULL, NULL) < 0) {
			perror("select:");
			goto cleanup;
		};
	}

cleanup:
	close(sockfd);
	return;
}

/**
 * convert a ipv6 link local address to mac address
 * @dest: buffer to store the resulting mac as string (18 bytes including the terminating \0)
 * @linklocal_ip6: \0 terminated string of the ipv6 address
 *
 * Return: true on success
 */
bool babelhelper_ll_to_mac(char *dest, const char* linklocal_ip6) {
	if (!linklocal_ip6)
		return false;

	struct in6_addr ll_addr = {};
	unsigned char mac[6];

	// parse the ip6
	if (!inet_pton(AF_INET6, linklocal_ip6, &ll_addr))
		return false;

	mac[0] = ll_addr.s6_addr[ 8] ^ (1 << 1);
	mac[1] = ll_addr.s6_addr[ 9];
	mac[2] = ll_addr.s6_addr[10];
	mac[3] = ll_addr.s6_addr[13];
	mac[4] = ll_addr.s6_addr[14];
	mac[5] = ll_addr.s6_addr[15];

	snprintf(dest, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return true;
}

