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

#include "babel-test.h"

#include <stdbool.h>
#include <unistd.h>

static void monitor(void) {
	babel_event_t event;
	bool done = false;

	babel_ctx_t *babel = babel_open(BABEL_PORT);
	if (!babel) {
		perror("babel_open");
		return;
	}

	printf("connected\n");

	babel_write_line(babel, "monitor");

	while (!done) {
		switch (babel_read_event(babel, &event)) {
		case BABEL_ERR:
			perror("babel_read_event");
			done = true;
			break;

		case BABEL_OK:
			printf("synced\n");
			break;

		case BABEL_CONTINUE:
			babel_test_print_event(&event);
		}
	}

	babel_close(babel);
}

int main() {
	while (true) {
		monitor();
		sleep(5);
	}

	return 0;
}
