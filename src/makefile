MAIN_DIR=../src
# INCLUDE_DIR=../include
# BIN_DIR used as output directory for executables
BIN_DIR=../bin
# DEPENDENCY_FILE=./dependencies.d

# SHELL:=/bin/bash

DEBUG=ON

# vpath %.c $(MAIN_DIR) $(TEST_DIR)
# vpath %.h $(INCLUDE_DIR)

# If last run was debugging run and this isn't force clean
# and vica versa
# ifneq ($(MAKECMDGOALS),clean)
# ifdef DEBUG
# TMP:=$(shell [[ ! -f .debug ]] && make clean && touch .debug)
# DEBUG=-DDEBUG -ggdb
# CFLAGS+=-fstack-protector-all -ftrapv
# else
# TMP:=$(shell [[ -f .debug ]] && make clean && rm .debug)
# CFLAGS+=-O2
# endif
# # If debugging messages then output messages and also use -O2
# ifdef DEBUG_MSG
# TMP:=$(shell [[ ! -f .debug ]] && make clean && touch .debug)
# DEBUG=-DDEBUG
# endif
# endif # MAKECMDGOALS

# ifdef GCC
# CC:=$(GCC)
# else
# CC:=gcc
# endif

# CFLAGS+=-Wall -Wextra -Wundef -Wpointer-arith -Wstrict-prototypes \
# 	   -Wmissing-prototypes -Wmissing-declarations -I$(INCLUDE_DIR) $(DEBUG) -pthread
# LIBS=-lzmq -lm

CPPFLAGS += -Wall -pthread -std=c++11


# -include $(DEPENDENCY_FILE)

# include $(MAIN_DIR)/makefile
# include $(TEST_DIR)/makefile

# .PHONY: dependencies

# dependencies:
# 	$(CC) -MM -I$(INCLUDE_DIR) $(wildcard ../src/main/*.c) > $(DEPENDENCY_FILE)
# 	$(CC) -MM -I$(INCLUDE_DIR) $(wildcard ../src/test/*.c) >> $(DEPENDENCY_FILE)

maparray: maparray.o
	$(CXX) $(CPPFLAGS) $^ -o $(BIN_DIR)/$@ $(LIBS)

clean:
	rm -rf *.o && rm -rf ../bin/*