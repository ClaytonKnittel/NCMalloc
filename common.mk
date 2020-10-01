CC=g++
AR=ar

BASE_DIR=$(shell pwd)

BDIR=$(BASE_DIR)/bin

LFLAGS=-L$(BASE_DIR)/lib/lib/
IFLAGS=-I$(BASE_DIR)/include -I$(BASE_DIR)/lib

# set when in debug mode
DEBUG=1

ifeq ($(DEBUG), 0)
CFLAGS=-O3 -std=c++17 -Wall -Wno-unused-function -MMD -MP $(BFLAGS)
else
CFLAGS=-O0 -std=c++17 -Wall -Wno-unused-function -MMD -MP -g3 -DDEBUG $(BFLAGS)
endif


# -flto allows link-time optimization (like function inlining)
LDFLAGS=-flto $(LFLAGS)

