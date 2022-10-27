/*
   Copyright (c) 2020, Matthias Schiffer <mschiffer@universe-factory.net>
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

#include "../include/libbabelhelper/babelhelper.h"

#include <stdio.h>


static const char *babel_test_event_name(babel_event_type_t event) {
	switch (event) {
	case BABEL_EVENT_FLUSH:
		return "flush";
	case BABEL_EVENT_ADD:
		return "add";
	case BABEL_EVENT_CHANGE:
		return "change";
	default:
		return "<unknown>";
	}
}

static const char *babel_test_object_name(babel_object_type_t object) {
	switch (object) {
	case BABEL_OBJECT_INTERFACE:
		return "interface";
	case BABEL_OBJECT_NEIGHBOUR:
		return "neighbour";
	case BABEL_OBJECT_XROUTE:
		return "xroute";
	case BABEL_OBJECT_ROUTE:
		return "route";
	default:
		return "<unknown>";
	}
}

static const char *babel_test_param_name(babel_param_type_t param) {
	switch (param) {
	case BABEL_PARAM_UP:
		return "up";
	case BABEL_PARAM_IPV4:
		return "ipv4";
	case BABEL_PARAM_IPV6:
		return "ipv6";
	case BABEL_PARAM_ADDRESS:
		return "address";
	case BABEL_PARAM_IF:
		return "if";
	case BABEL_PARAM_REACH:
		return "reach";
	case BABEL_PARAM_UREACH:
		return "ureach";
	case BABEL_PARAM_RXCOST:
		return "rxcost";
	case BABEL_PARAM_TXCOST:
		return "txcost";
	case BABEL_PARAM_COST:
		return "cost";
	case BABEL_PARAM_PREFIX:
		return "prefix";
	case BABEL_PARAM_FROM:
		return "from";
	case BABEL_PARAM_METRIC:
		return "metric";
	case BABEL_PARAM_INSTALLED:
		return "installed";
	case BABEL_PARAM_ID:
		return "id";
	case BABEL_PARAM_REFMETRIC:
		return "refmetric";
	case BABEL_PARAM_VIA:
		return "via";
	default:
		return "<unknown>";
	}
}


static void babel_test_print_event(const babel_event_t *event) {
	size_t i;

	printf("%s %s '%s'\n",
	       babel_test_event_name(event->type),
	       babel_test_object_name(event->object_type),
	       event->object);

	for (i = 0; i < BABEL_PARAM__COUNT; i++) {
		if (event->params[i])
			printf("\t%s '%s'\n", babel_test_param_name(i), event->params[i]);
	}
}
