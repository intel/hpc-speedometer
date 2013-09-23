/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef DATA_H
#define DATA_H

#include <vector>
#include <string>
#include <assert.h>

class lineStream
{
public:
    virtual int open() = 0;
    virtual int getline(void *buff, int size) = 0;
    virtual int close() = 0;
};

class sockStream : public lineStream
{
    int fd;
    char hostname[256];
    char port[32];
public:
    sockStream() : fd(-1) {}
    int open();
    int getline(void *buff, int size);
    int close();
    void setHostname(const char *h) { strncpy(hostname, h, sizeof(hostname)); }
    void setPort(const char *p) { strncpy(port, p, sizeof(port)); }
};

class fileStream : public lineStream
{
    FILE *fp;
    char filename[256];
public:
    fileStream() : fp(0) {}
    int open();
    int getline(void *buff, int size);
    int close();
    void setFilename(const char *h) { strncpy(filename, h, sizeof(filename)); }
};


class metricStream
{
    struct label
    {
        double max;
        std::string name;
        std::string units;
        double factor;
    };

    // current values
    double row[10];
    char line[1024];

    // where the data comes from
    lineStream *stream;

    std::vector<label> labels;

public:
    metricStream() : stream(0) {}
    int open(lineStream *);
    int advance();
    int close();

    // name
    std::string name(unsigned ix)
    {
        assert(ix < labels.size());
        return labels[ix].name;
    }
    std::string units(unsigned ix)
    {
        assert(ix < labels.size());
        return labels[ix].units;
    }

    // raw values
    double raw(unsigned ix)
    {
        assert(ix < labels.size());
        return row[ix];
    }
    // scaled values
    double scaled(unsigned ix)
    {
        assert(ix < labels.size());
        double max = labels[ix].max;
        if (max)
            return (row[ix] / max) * 100;
        return row[ix];
    }
    // maxima
    double max(unsigned ix)
    {
        assert(ix < labels.size());
        return labels[ix].max;
    }
    // factor
    double factor(unsigned ix)
    {
        assert(ix < labels.size());
        return labels[ix].factor;
    }
    // # fields
    int nlabels() { return labels.size(); }
};
#endif
