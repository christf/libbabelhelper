/*
   Copyright (c) 2017, 2018 Christof Schulze <christof@christofschulze.com>
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

#include "../include/libbabelhelper/babelhelper.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#define BABEL_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


struct babel_ctx {
	FILE *stream;

	size_t n;
	char *line;
};


/* Keyword lookup tables
 *
 * Must be ordered lexicographcally (with regard to strcmp),
 * as we use binary search for lookup */

struct babel_keyword {
	ssize_t index;
	const char *keyword;
};

const struct babel_keyword event_keywords[] = {
	{ BABEL_EVENT_ADD, "add" },
	{ BABEL_EVENT_CHANGE, "change" },
	{ BABEL_EVENT_FLUSH, "flush" },
};

const struct babel_keyword object_keywords[] = {
	{ BABEL_OBJECT_INTERFACE, "interface" },
	{ BABEL_OBJECT_NEIGHBOUR, "neighbour" },
	{ BABEL_OBJECT_ROUTE, "route" },
	{ BABEL_OBJECT_XROUTE, "xroute" },
};

const struct babel_keyword param_keywords[] = {
	{ BABEL_PARAM_ADDRESS, "address" }, { BABEL_PARAM_COST, "cost" },     { BABEL_PARAM_FROM, "from" },
	{ BABEL_PARAM_ID, "id" },           { BABEL_PARAM_IF, "if" },         { BABEL_PARAM_INSTALLED, "installed" },
	{ BABEL_PARAM_IPV4, "ipv4" },       { BABEL_PARAM_IPV6, "ipv6" },     { BABEL_PARAM_METRIC, "metric" },
	{ BABEL_PARAM_PREFIX, "prefix" },   { BABEL_PARAM_REACH, "reach" },   { BABEL_PARAM_REFMETRIC, "refmetric" },
	{ BABEL_PARAM_RXCOST, "rxcost" },   { BABEL_PARAM_TXCOST, "txcost" }, { BABEL_PARAM_UP, "up" },
	{ BABEL_PARAM_UREACH, "ureach" },   { BABEL_PARAM_VIA, "via" },
};

static int babel_keyword_cmp(const void *a, const void *b) {
	const struct babel_keyword *ka = a, *kb = b;
	return strcmp(ka->keyword, kb->keyword);
}

static ssize_t babel_lookup(const char *keyword, const struct babel_keyword *entries, size_t n_entries) {
	const struct babel_keyword key = { 0, keyword };

	const struct babel_keyword *entry = bsearch(&key, entries, n_entries, sizeof(key), babel_keyword_cmp);
	if (!entry)
		return -1;

	return entry->index;
}

static inline babel_event_type_t babel_lookup_event_type(const char *keyword) {
	return babel_lookup(keyword, event_keywords, BABEL_ARRAY_SIZE(event_keywords));
}

static inline babel_object_type_t babel_lookup_object_type(const char *keyword) {
	return babel_lookup(keyword, object_keywords, BABEL_ARRAY_SIZE(object_keywords));
}

static inline babel_param_type_t babel_lookup_param_type(const char *keyword) {
	return babel_lookup(keyword, param_keywords, BABEL_ARRAY_SIZE(param_keywords));
}


static inline bool babel_is_ok(const char *str) {
	return strcmp(str, "ok") == 0;
}

babel_ctx_t *babel_open(int port) {
	const struct sockaddr_in6 addr = { .sin6_family = AF_INET6,
					   .sin6_addr = IN6ADDR_LOOPBACK_INIT,
					   .sin6_port = htons(port) };
	int fd, errno_safe;
	babel_ctx_t *babel;
	const char *line;

	babel = calloc(1, sizeof(*babel));
	if (!babel)
		return NULL;

	fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (fd < 0)
		goto err;

	if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto err;

	/* Using the same FILE for reading from and writing to the same socket
	 * is tricky to get right, so we only use it for read buffering */
	babel->stream = fdopen(fd, "r");
	if (!babel->stream)
		goto err;

	/* Closing the stream will close the fd - set fd to -1 to avoid closing it twice on errors */
	fd = -1;

	do {
		line = babel_read_line(babel);
		if (!line)
			goto err;
	} while (!babel_is_ok(line));

	return babel;

err:
	errno_safe = errno;
	if (fd >= 0)
		close(fd);
	babel_close(babel);
	errno = errno_safe;
	return NULL;
}

void babel_close(babel_ctx_t *babel) {
	if (babel->stream)
		fclose(babel->stream);
	free(babel->line);
	free(babel);
}

int babel_write_line(babel_ctx_t *babel, const char *str) {
	return dprintf(fileno(babel->stream), "%s\n", str);
}

const char *babel_read_line(babel_ctx_t *babel) {
	ssize_t len = getline(&babel->line, &babel->n, babel->stream);
	if (len < 0)
		return NULL;

	if (len > 0 && babel->line[len - 1] == '\n')
		babel->line[len - 1] = 0;

	return babel->line;
}

int babel_read_event_(babel_ctx_t *babel, babel_event_t *event, size_t n_params) {
	char *tok, *saveptr = NULL;
	size_t i;

	if (!babel_read_line(babel))
		return BABEL_ERR;

	if (babel_is_ok(babel->line))
		return BABEL_OK;

	event->type = BABEL_EVENT_UNKNOWN;
	event->object_type = BABEL_OBJECT_UNKNOWN;
	event->object = NULL;
	for (i = 0; i < n_params; i++)
		event->params[i] = NULL;

	tok = strtok_r(babel->line, " ", &saveptr);
	if (!tok)
		goto end;
	event->type = babel_lookup_event_type(tok);

	tok = strtok_r(NULL, " ", &saveptr);
	if (!tok)
		goto end;
	event->object_type = babel_lookup_object_type(tok);

	tok = strtok_r(NULL, " ", &saveptr);
	if (!tok)
		goto end;
	event->object = tok;

	while ((tok = strtok_r(NULL, " ", &saveptr))) {
		size_t index = babel_lookup_param_type(tok);

		tok = strtok_r(NULL, " ", &saveptr);
		if (!tok)
			break;

		if (index >= 0 && index < n_params)
			event->params[index] = tok;
	}

end:
	return BABEL_CONTINUE;
}
