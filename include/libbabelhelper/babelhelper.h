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

void babelhelper_babelroute_free(struct babelroute *br);
void babelhelper_babelneighbour_free(struct babelneighbour *bn);
bool babelhelper_get_neighbour(struct babelneighbour *dest, char *line);
bool babelhelper_get_route(struct babelroute *dest, char *line);
void babelhelper_readbabeldata(void* object, void (*lineprocessor)(char*, void* object));
int babelhelper_babel_connect(int port);
int babelhelper_sendcommand(int fd, char *command);  
bool babelhelper_input_pump(int fd, void *object, void (*lineprocessor)(char*, void* object));

bool babelhelper_generateip(char *result,const unsigned char *mac, const char *prefix);
bool babelhelper_generateip_str(char *result,const char *strmac, const char *prefix);
bool babelhelper_ll_to_mac(char *dest, const char* linklocal_ip6);
