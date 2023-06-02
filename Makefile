CC := gcc
CXX := g++
.DEFAULT_GOAL := all
DEST := .

########## for ctp-query BEGIN ###########################

module_list := ctp-query-stock-option
ctp-query-stock-option: ctp-query.o main.o
	$(CXX) -o $@ $^ -L $(DEST)/lib -lthosttraderapi_se -pthread

########## for ctp-query END #############################

CFLAGS := -Wall -I $(DEST)/include -O2 -g -pthread
CXXFLAGS := $(CFLAGS) --std=c++11

all: $(module_list)

install: $(module_list)
	install -d $(DEST)/bin
	install -p $(module_list) $(DEST)/bin

clean:
	$(RM) $(module_list) *.o

.PHONY: all install clean