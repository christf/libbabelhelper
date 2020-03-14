/*
   Copyright (c) 2017, Christof Schulze <christof.schulze@gmx.net>
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

#include <stddef.h>


#define BABEL_PORT 33123


typedef enum babel_event_type {
	BABEL_EVENT_UNKNOWN = -1,
	BABEL_EVENT_FLUSH,
	BABEL_EVENT_ADD,
	BABEL_EVENT_CHANGE,
} babel_event_type_t;

typedef enum babel_object_type {
	BABEL_OBJECT_UNKNOWN = -1,
	BABEL_OBJECT_INTERFACE,
	BABEL_OBJECT_NEIGHBOUR,
	BABEL_OBJECT_XROUTE,
	BABEL_OBJECT_ROUTE,
} babel_object_type_t;

typedef enum babel_param_type {
	BABEL_PARAM_UP,
	BABEL_PARAM_IPV4,
	BABEL_PARAM_IPV6,
	BABEL_PARAM_ADDRESS,
	BABEL_PARAM_IF,
	BABEL_PARAM_REACH,
	BABEL_PARAM_UREACH,
	BABEL_PARAM_RXCOST,
	BABEL_PARAM_TXCOST,
	BABEL_PARAM_COST,
	BABEL_PARAM_PREFIX,
	BABEL_PARAM_FROM,
	BABEL_PARAM_METRIC,
	BABEL_PARAM_INSTALLED,
	BABEL_PARAM_ID,
	BABEL_PARAM_REFMETRIC,
	BABEL_PARAM_VIA,
	/* _COUNT must always be orderd last */
	BABEL_PARAM__COUNT,
} babel_param_type_t;

typedef struct babel_event {
	babel_event_type_t type;
	babel_object_type_t object_type;
	const char *object;
	const char *params[BABEL_PARAM__COUNT];
} babel_event_t;


enum { BABEL_ERR = -1,
       BABEL_CONTINUE = 0,
       BABEL_OK = 1,
};


/* Definition of babel_ctx is hidden, so it doesn't become part of the public
 * interface of libbabelhelper */
typedef struct babel_ctx babel_ctx_t;


babel_ctx_t *babel_open(int port);
void babel_close(babel_ctx_t *babel);

int babel_write_line(babel_ctx_t *babel, const char *str);
const char *babel_read_line(babel_ctx_t *babel);

/* Pass _COUNT to allow future extensions without breaking ABI */
int babel_read_event_(babel_ctx_t *babel, babel_event_t *event, size_t n_params);

static inline int babel_read_event(babel_ctx_t *babel, babel_event_t *event) {
	return babel_read_event_(babel, event, BABEL_PARAM__COUNT);
}
