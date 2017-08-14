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

char* babelhelper_generateip(const char *stringmac, const char *prefix) {
	char *address = malloc(INET6_ADDRSTRLEN);
	unsigned char mac[8];
	void *dst = malloc(sizeof(struct in6_addr));

	inet_pton(AF_INET6, prefix, dst);

	sscanf(stringmac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[5], &mac[6], &mac[7]);
	mac[3]=0xff;
	mac[4]=0xfe;
	mac[0] ^= 1 << 1;

	memcpy(dst + 8, mac, 8); 
	inet_ntop(AF_INET6, dst, address, 60);
	free(dst);
	return address;
}


int babelhelper_get_neighbour(struct babelneighbour *dest, char *line) {
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

	return 0;

free:
	free(action);
	free(address_str);
	free(ifname);

	return 1;
}

void babelhelper_babelneighbour_free(struct babelneighbour *bn) {
	if (bn->action) free(bn->action);
	if (bn->address_str) free(bn->address_str);
	if (bn->ifname) free(bn->ifname);
}

void babelhelper_babelroute_free(struct babelroute *br) {
	free(br->action);
	free(br->route);
	free(br->prefix);
	free(br->from);
	free(br->id);
	free(br->via);
	free(br->ifname);
}

int babelhelper_get_route(struct babelroute *dest, char *line) {
	struct babelroute ret;
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

	struct in6_addr in6_via;
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

	return 0;
free:
	babelhelper_babelroute_free(&ret);
	return 1;
}

int readchunk(int fd, char *target) {
	ssize_t len = 0;

	while (len <=0) {
		len = read(fd, (void*)target, CHUNKSIZE);
		if (len == 0) {
			// fprintf(stderr, "Connection closed.\n"); // this may or may not be an error => donot print a message
			break;
		}
		else if (len < 0 && errno == EAGAIN) {
			usleep(1000);
		}
		else if (len < 0) {
			perror("Connection error while reading from babel socket.");
			return -1;
		}
	}

	target[len] = 0;
	return len;
}

void input_pump(int fd, struct json_object* obj, void (*lineprocessor)(char*, struct json_object*)) {
	char *inbuf;
	char *stringp = inbuf;
	const char *endstring = "ok";
	size_t new_len, old_len = 0;
	inbuf[0] = 0;

	while  ( 1 ) {
		old_len=strlen(inbuf);
		new_len = old_len + CHUNKSIZE + 1;
		inbuf = realloc(inbuf, new_len);
		if ( readchunk(fd, &inbuf[old_len]) < 1 )
			break;

		//scan for newlines, have whole lines processed by lineprocessor
		while ( strsep(&stringp, "\n") ) {
			if (lineprocessor)
				lineprocessor(inbuf, obj);

			if ( !strncmp(inbuf, endstring, strlen(endstring) ) ) {
				free(inbuf);
				return;
			}

			size_t buffer_used  = strlen(stringp) +1;
			memmove(inbuf, stringp, buffer_used);
			inbuf = realloc(inbuf, buffer_used);
		}

	}
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


void readbabeldata(struct json_object *obj, void (*lineprocessor)(char*, struct json_object*))
{
	int sockfd = babelhelper_babel_connect(BABEL_PORT);

	// receive and ignore babel header
	input_pump(sockfd, NULL, NULL);
	int i =  write(sockfd, "dump\n", 5);
	if ( i < 5 ) {
		// TODO should we handle this?
		fprintf(stderr, "could not complete write \"dump\" on the babel socket\n");
	}

	shutdown(sockfd, SHUT_WR);
	// receive result
	input_pump(sockfd, obj, lineprocessor);

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
int babelhelper_ll_to_mac(char *dest, const char* linklocal_ip6) {
	struct in6_addr ll_addr;
	unsigned char mac[6];

	// parse the ip6
	if (!inet_pton(AF_INET6, linklocal_ip6, &ll_addr))
		return 1;

	mac[0] = ll_addr.s6_addr[ 8] ^ (1 << 1); 
	mac[1] = ll_addr.s6_addr[ 9]; 
	mac[2] = ll_addr.s6_addr[10];
	mac[3] = ll_addr.s6_addr[13];
	mac[4] = ll_addr.s6_addr[14];
	mac[5] = ll_addr.s6_addr[15];

	snprintf(dest, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return 0;
}

