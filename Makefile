#!/bin/make

TARGET=cloudconnector
OBJECTS=main.o ssl.o log.o network.o cfg_files.o array.o array_int.o array_dump.o

PKG_LIST=gnutls

CC=gcc
CFLAGS=-Wall -g -ggdb -O0 -pipe --std=gnu99
LIBS=


CFLAGS+=$(shell pkg-config --cflags $(PKG_LIST))
LIBS+=$(shell pkg-config --libs $(PKG_LIST))

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJECTS) $(TARGET)

