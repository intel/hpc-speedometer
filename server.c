/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#include "metrics.h"

static int sock_fd;
static int accept_fd = -1;
#define DEFAULT_PORT 12345

void
setup_server()
{
    int ret;
    struct addrinfo hint;
    struct addrinfo *res;
    char hostname[128];
    char port[128];

    if (gbl.port <= 0)
        sprintf(port, "%d", DEFAULT_PORT);
    else
        sprintf(port, "%d", gbl.port);

    sock_fd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    if (sock_fd == -1)
        err(1, "create socket");
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    ret = getaddrinfo(NULL, port, &hint, &res);
    if (ret != 0)
        err(1, "getaddrinfo returns: %s", gai_strerror(ret));
    ret = bind(sock_fd, res->ai_addr, res->ai_addrlen);
    if (ret != 0)
        err(1, "bind socket");
    ret = listen(sock_fd, 1);
    if (ret != 0)
        err(1, "listen socket");
    signal(SIGPIPE, SIG_IGN);
    fprintf(stderr, "Listening on port %s\n", port);
}

static void
sock_write(const void *data, ssize_t len)
{
    if (accept_fd == -1)
        return;
    ssize_t ret = write(accept_fd, data, len);
    if (ret != len)
    {
        warn("write socket");
        close(accept_fd);
        accept_fd = -1;
    }
}

void
send_server(const char *format, ...)
{
    int ret;
    char buffer[4+1024];
    int l;

    if (accept_fd == -1)
    {
        struct sockaddr client;
        socklen_t len = sizeof(client);


        ret = accept(sock_fd, &client, &len);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            err(1, "accept socket");
        }
        accept_fd = ret;
        char *q = gbl.hdr;
        char *p = strchr(q, '\n');
        while (p != 0)
        {
            int l = p - q + 1;
            *((int *)buffer) = l;
            memcpy(&buffer[4], q, l);
            sock_write(buffer, l+4);
            q = p+1;
            p = strchr(q, '\n');
        }
    }
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer+4, sizeof(buffer), format, ap);
    va_end(ap);
    l = strlen(buffer+4);
    *((int *)buffer) = l;
    sock_write(buffer, l+4);
}
