/*
   Copyright (c) 2017, Christof Schulze <christof.schulze@gmx.net>
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

bool babelhelper_get_neighbour(struct babelneighbour *dest, char *line) {
	char *action = NULL;
	char *address_str = NULL;
	char *ifname = NULL;
	int reach, cost, rxcost, txcost;
	int n = sscanf(line, "%ms neighbour %*x address %ms if %ms "
			"reach %x rxcost %d txcost %d cost %d",
			&action, &address_str, &ifname, &reach, &rxcost, &txcost, &cost);

	if (n != 7)
		goto free;

	if (inet_pton(AF_INET6, address_str, &(dest->address)) != 1)
	{
		fprintf(stderr, "babeld-parser error: could not convert babel data to ipv6 address: %s\n", address_str);
		goto free;
	}
	dest->action = action;
	dest->address_str = address_str;
	dest->ifname = ifname;
	dest->reach = reach;
	dest->rxcost = rxcost;
	dest->txcost = txcost;
	dest->cost = cost;

	return true;

free:
	free(action);
	free(address_str);
	free(ifname);
	return false;
}

void babelhelper_babelneighbour_free_members(struct babelneighbour *bn) {
	free(bn->action);
	free(bn->address_str);
	free(bn->ifname);
}

void babelhelper_babelroute_free_members(struct babelroute *br) {
	free(br->action);
	free(br->route);
	free(br->prefix);
	free(br->from);
	free(br->id);
	free(br->via);
	free(br->ifname);
}

bool babelhelper_get_route(struct babelroute *dest, char *line) {
	struct babelroute ret = {};
	char *action = NULL;
	char *route = NULL;
	char *prefix = NULL;
	char *from = NULL;
	char *id = NULL;
	int metric;
	int refmetric ;
	char *via = NULL;
	char *ifname = NULL;

	int n = sscanf(line, "%s route %s prefix %s from %s installed yes id %s metric %d refmetric %d via %s if %s",
			action, route, prefix, from, id, &metric, &refmetric, via, ifname );

	if (n != 9)
		goto free;

	struct in6_addr in6_via = {};
	if (inet_pton(AF_INET6, via, &in6_via) != 1)
	{
		fprintf(stderr, "babeld-parser error: could not convert babel data to ipv6 address: %s\n", via);
		goto free;
	}

	ret.action = action;
	ret.route = route;
	ret.prefix = prefix;
	ret.from = from;
	ret.id = id;
	ret.metric = metric;
	ret.refmetric = refmetric;
	ret.via = via;
	ret.ifname = ifname;
	ret.in6_via = in6_via;

	return true;
free:
	babelhelper_babelroute_free_members(&ret);
	return false;
}

bool babelhelper_input_pump(struct babelhelper_ctx *ctx, int fd,  void* obj, bool (*lineprocessor)(char* line, void* object)) {
	char *line = NULL;
	char *buffer = NULL;
	size_t buffer_used = 0;
	char *stringp = NULL;
	ssize_t len = 0;
	const char *sep = "\n\r";

	int retval;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	do {
		struct timeval timeout =  {
			.tv_sec=2,
			.tv_usec=0,
		};

		retval = select(fd+1, &rfds, NULL, NULL, &timeout);
		if (retval == -1) {
			perror("Error on select(), reading from babel socket.");
		} else if (retval) {
			buffer = realloc(buffer, buffer_used + LINEBUFFER_SIZE + 1);
			if (buffer == NULL) {
				fprintf(stderr, "Cannot allocate buffer\n");
				return false;
			}

			len = read(fd, buffer + buffer_used, LINEBUFFER_SIZE);
			if ( (len == -1 && errno == EAGAIN) || len == 0) {
				break;
			} else if (len > 0 ) {
				buffer[buffer_used + len] = 0;
				buffer_used = buffer_used + len;
				while ( buffer_used > 0 ) {
					stringp = buffer;
					if (stringp) {
						line = strsep(&stringp, sep);
					}
					if (stringp == NULL) {
						break; // incomplete line due to INPUT_BUFFER_SIZE_LIMITATION - read some more data, then repeat parsing.
					}
					buffer_used--; // when replacing \n with \0 in strsep, the buffer-usage actually shrinks because \0 are not counted by strlen

					if (line) {
						size_t linelength = strlen(line);
						if (linelength > 0) {
							if (!lineprocessor(line, obj) ) {
								goto out;
							}
							buffer_used-=linelength;
							memmove(buffer, stringp, buffer_used + 1);
						}
					}
				}
			} else {
				printf("didn't read anything but no error %i\n", errno);
			}
		} else {
			if (ctx->debug)
				fprintf(stderr, "No data on babel socket within timeout of %li seconds and %li usecs.\n", timeout.tv_sec, timeout.tv_usec);
			free(buffer);
			return false;
		}
	} while ( buffer_used > 0 || len > 0 );

out:
	free(buffer);
	return true;
}

int babelhelper_babel_connect(int port) {
	int sockfd ;

	struct sockaddr_in6 serv_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(port)
	};

	sockfd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		return -1;
	}
	if (inet_pton(AF_INET6, "::1", &serv_addr.sin6_addr.s6_addr) != 1)
	{
		perror("Cannot parse hostname");
		return -1;
	}
	if (connect(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		if (errno != EINPROGRESS) {
			perror("Can not connect to babeld");
			return -1;
		}
	}
	return sockfd;
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

	if (retval == -1)
		perror("select()");
	else if (retval) {
		while (send(fd, command, cmdlen, 0) != cmdlen) {
			perror("Select said the babel socket is ready for writing but we received an error while sending command %s to babel. This should not happen. Retrying.");
		}
	}
	else {
		if (ctx->debug)
			fprintf(stderr, "could not write command to babel socket within 5 seconds.\n");
		return 0;
	}

	return cmdlen;
}


bool babelhelper_discard_response(char *lineptr, void *object) {
	return !!strncmp(lineptr, "ok", 2);
}

void babelhelper_readbabeldata(struct babelhelper_ctx *ctx,void *object, bool (*lineprocessor)(char*, void* object)) {
	int sockfd;

	do {
		sockfd = babelhelper_babel_connect(BABEL_PORT);
		if (sockfd < 0)
			fprintf(stderr, "Connecting to babel socket failed. Retrying.\n");
	} while (sockfd < 0);

	// receive and ignore babel header
	while ( ! babelhelper_input_pump(ctx, sockfd, NULL, babelhelper_discard_response))
		fprintf(stderr, "Retrying to skip babel header. Reading from babel socket.\n");

	// query babel data
	while ( babelhelper_sendcommand(ctx, sockfd, "dump\n") != 5 )
		fprintf(stderr, "Retrying to send dump-command to babel socket.\n");

	// receive result
	while ( ! babelhelper_input_pump(ctx, sockfd, object, lineprocessor))
		fprintf(stderr, "Retrying reading from babel socket.\n");

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

