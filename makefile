#!/usr/bin/env make -kf

LD       = ld
CC       = clang
CXX      = $(CC)++
CFLAGS   = -g -fPIC -fbracket-depth=512
CXXFLAGS = $(CFLAGS)
LDFLAGS  = -lm -lc -ldl -lpthread
OBJECTS  = $(addsuffix .o, device list lora shell test)
LIB      = corelora
PKG      = $(LIB).a
EXE      = lora.exe
TAR      = lora.tar
DOCS     = docs

.PHONY: clean $(DOCS)

default: $(OBJECTS)

all: $(TAR) $(LIB) $(PKG) test

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PKG): $(PKG)($(OBJECTS))

$(TAR): clean
	tar cvf "$@" makefile *.c *.h

$(EXE): $(LIB)
	$(CC) $(CLFAGS) $(LDFLAGS) -o $@ $^ main.c

$(LIB): $(OBJECTS)
	$(LD) $(LDFLAGS) -dylib -o $@ $^

$(DOCS): doxygen.conf
	doxygen $<

shell test http: $(EXE)
	./$< $@ #| ts -s "%.S"

debug: $(EXE)
	lldb $<

clean:
	rm -rf $(OBJECTS) $(PKG) $(LIB) $(EXE) $(TAR) $(addsuffix .dSYM, $(LIB) $(EXE) $(OBJECTS))

upload: $(TAR) $(DOCS)
	rsync -lrzu --delete "$$(pwd)" "ec2-52-59-80-30.eu-central-1.compute.amazonaws.com:/srv/www"

export MACOSX_DEPLOYMENT_TARGET := 10.8

# alias
lib: $(LIB)
pkg: $(PKG)
exe: $(EXE)
tar: $(TAR)
