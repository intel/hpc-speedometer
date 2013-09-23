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
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "data.h"

int
sockStream::open()
{
    int ret;
    struct addrinfo hint;
    struct addrinfo *res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        err(1, "create socket");
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = 0;
    ret = getaddrinfo(hostname, port, &hint, &res);
    if (ret != 0)
        err(1, "getaddrinfo returns: %s", gai_strerror(ret));
    for (;;)
    {
        ret = connect(fd, res->ai_addr, res->ai_addrlen);
        if (ret == 0)
            break;
        if (errno == EINPROGRESS || errno == EALREADY)
        {
            continue;
        }
        if (ret == -1)
            return -1;
    }
    return 0;
}

int
sockStream::close()
{
    if (fd >= 0)
        ::close(fd);
    fd = -1;
    return 0;
}

int
sockStream::getline(void *data, int len)
{
    int len1;
    if (fd == -1)
        return 0;
    ssize_t ret = read(fd, &len1, sizeof(len1));
    if (ret == 0)
        return -1;
    if (ret != sizeof(len1))
    {
        if (errno == EAGAIN)
            return 0;
        warn("read socket 1 %d", errno);
        fd = -1;
        return -1;
    }
    if (len1 > len) len1 = len;
    ret = 0;
    while (ret < len1)
    {
        int l = read(fd, &((char *)data)[ret], len1 - ret);
        if (l < 0)
        {
            warn("read socket 2 %d", errno);
            fd = -1;
            return -1;
        }
        ret += l;
    }
    ((char *)data)[len1] = 0;
    return len1+4;
}

int
fileStream::open()
{
    fp = fopen(filename, "r");
    if (fp == 0)
        err(1, "open file");
    return 0;
}

int
fileStream::close()
{
    if (fp)
        fclose(fp);
    fp = 0;
    return 0;
}

int
fileStream::getline(void *data, int len)
{
    char *p;

    p = fgets((char *)data, len, fp);
    if (p == 0)
        return 0;
    return strlen(p);
}

int
metricStream::open(lineStream *st)
{
    label s;

    stream = st;
    s.name = "Time";
    s.units = "Seconds";
    s.max = 0;
    s.factor = 1;
    labels.push_back(s);

    for (;;)
    {
        int ret = stream->getline(line, sizeof(line));
        if (ret == 0)
            return 0;
        char *p = strchr(line, '=');
        if (p == 0)
            break;
        *p = 0;
        //Mem=1.75e+11,GB/Sec,1e-09
        char *q;
        s.name = line;
        s.max = strtod(p+1, &q);
        assert(*q == ',');
        ++q;
        p = strchr(q, ',');
        assert(p != 0);
        *p = 0;
        s.units = q;
        ++p;
        s.factor = strtod(p, &q);
        assert(*q == '\n');
        labels.push_back(s);
    }
    // point at the header
    unsigned i = 0;
    char *p = line;
    bool done = false;
    do
    {
        char *q = strchr(p, ',');
        if (q != 0)
            *q = 0;
        else
        {
            q = strchr(p, '\n');
            if (q != 0)
                *q = 0;
            done = true;
        }
        assert(strcmp(p, labels[i].name.c_str()) == 0);
        p = q+1;
        ++i;
    } while (!done);
    assert(i == labels.size());
    // need to advance to the first data line
    return 1;
}

int
metricStream::advance()
{
    int ret = stream->getline(line, sizeof(line));
    if (ret <= 0)
        return ret;
    char *p = line;
    int i = 0;
    for (;;)
    {
        char *q = strchr(p, ',');
        if (q)
            *q = 0;
        row[i] = strtod(p, 0);
        ++i;
        if (q == 0)
            break;
        p = q + 1;
    }
    return ret;
}

int metricStream::close()
{
    if (stream)
        stream->close();
    stream = 0;
    labels.clear();
    return 0;
}
