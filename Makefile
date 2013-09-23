CC=icc
CXX=icpc
CFLAGS=-std=gnu99 -g -I.  -O
CXXFLAGS=-g -I. -O
LDFLAGS=
LIBS=-lpthread -lrt
TARGET ?= intel64
DESTDIR ?= .

SRCS=main.c server.c

PROG=speedometer.bin

ifeq ($(TARGET),mic)
    CFLAGS += -mmic -DKNC
    CXXFLAGS += -mmic -DKNC
    LDFLAGS += -mmic
    SRCS += mc.c
endif
ifeq ($(TARGET),intel64)
    CFLAGS += -DINTEL64 -Iintel64
    CXXFLAGS += -DINTEL64 -Iintel64
endif

TARGETDIR=$(TARGET)

TARGETSRCS=events.cpp output.cpp
ifeq ($(TARGET),intel64)
    TARGETSRCS += mc.cpp pci.cpp
endif

OBJS=$(SRCS:%.c=$(TARGETDIR)/%.o) $(TARGETSRCS:%.cpp=$(TARGETDIR)/%.o)

default: $(TARGETDIR)/$(PROG)

# Build rules
$(TARGETDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $(DEFINES) $(INCLUDES) $<

$(TARGETDIR)/%.o: $(TARGETDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $(DEFINES) $(INCLUDES) $<

$(TARGETDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(DEFINES) $(INCLUDES) $<

$(TARGETDIR)/%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $(DEFINES) $(INCLUDES) $<

$(TARGETDIR)/%.s: %.S
	$(CC) $(CFLAGS) -E -dD $(DEFINES) $(INCLUDES) $< >  $@ 

$(OBJS): Makefile $(wildcard *.h)

.PRECIOUS: $(TARGETDIR)/%.o


$(TARGETDIR)/$(PROG): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

#install: $(TARGETDIR)/$(PROG)
#	install -D -o root -m 4755 $(TARGETDIR)/$(PROG) $(DESTDIR)/bin/$(TARGETDIR)/$(PROG)
#	install scripts/speedometer $(DESTDIR)/bin/$(TARGETDIR)/speedometer

clean:
	rm -rf $(TARGETDIR)/$(PROG) $(OBJS)

