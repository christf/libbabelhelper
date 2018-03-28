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

#pragma once

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include "errno.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>


#define BABEL_PORT 33123
#define LINEBUFFER_SIZE 256
#define TRACE {printf("%s: %d\n", __FILE__, __LINE__);};


#define FOREACH_BABEL_TOKEN(BABEL_TOKEN) \
        BABEL_TOKEN(VERB)   \
        BABEL_TOKEN(XROUTE)  \
        BABEL_TOKEN(INTERFACE)   \
        BABEL_TOKEN(ROUTE)   \
        BABEL_TOKEN(NEIGHBOUR)   \
        BABEL_TOKEN(ADDRESS)   \
        BABEL_TOKEN(IF)   \
        BABEL_TOKEN(PREFIX)   \
        BABEL_TOKEN(FROM)   \
        BABEL_TOKEN(METRIC)   \
        BABEL_TOKEN(COST)   \
        BABEL_TOKEN(RXCOST)   \
        BABEL_TOKEN(TXCOST)   \
        BABEL_TOKEN(INSTALLED)   \
        BABEL_TOKEN(VIA)   \
        BABEL_TOKEN(REFMETRIC)   \
        BABEL_TOKEN(ID)   \
        BABEL_TOKEN(IPV6)   \
        BABEL_TOKEN(IPV4)   \
        BABEL_TOKEN(UREACH)   \
        BABEL_TOKEN(REACH)   \
        BABEL_TOKEN(UP)   \
        BABEL_TOKEN(OK) \
        BABEL_TOKEN(UNKNOWN)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum BABEL_TOKEN_ENUM {
    FOREACH_BABEL_TOKEN(GENERATE_ENUM)
};
static const char *BABEL_TOKEN_STRING[] = {
    FOREACH_BABEL_TOKEN(GENERATE_STRING)
};

#define str(x) #x
#define xstr(x) str(x)

#define num_different_tokens UNKNOWN+1


struct babelneighbour {
	char *action;
	char *address_str;
	char *ifname;
	int reach;
	int rxcost;
	int txcost;
	int cost;
	struct in6_addr address;
};

struct babelroute{
	char *action;
	char *route;
	char *prefix;
	char *from;
	char *id;
	int metric;
	int refmetric;
	char *via;
	char *ifname;
	struct in6_addr in6_via;
};

struct babelhelper_ctx {
	bool debug;
};

void babelhelper_babelroute_free_members(struct babelroute *br);
void babelhelper_babelneighbour_free_members(struct babelneighbour *bn);
void babelhelper_readbabeldata(struct babelhelper_ctx *ctx, void* object, bool (*lineprocessor)(char**, void* object));
bool babelhelper_discard_response(char **data, void *object);
int babelhelper_babel_connect(int port);
int babelhelper_sendcommand(struct babelhelper_ctx *ctx, int fd, char *command); 
bool babelhelper_input_pump(struct babelhelper_ctx *ctx, int fd, void *object, bool (*lineprocessor)(char**, void* object));
void printrecognized(char **data);

bool babelhelper_generateip(char *result,const unsigned char *mac, const char *prefix);
bool babelhelper_generateip_str(char *result,const char *strmac, const char *prefix);
bool babelhelper_ll_to_mac(char *dest, const char* linklocal_ip6);
