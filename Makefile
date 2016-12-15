# makefile pour HTTP Server

CC =gcc
LDFLAGS =-lpthread -lrt
CFLAGS =-W -Wall -ansi -pedantic -Iinclude

DIR=.
BIN=$(DIR)/bin/
OBJ=$(DIR)/obj/
INCLUDE=$(DIR)/include/
LIB=$(DIR)/lib/
SRC=$(DIR)/src/

HC=

.PHONY: all clean main curl
all: $(BIN)main

prog: $(BIN)main
	-$$PWD/bin/main

$(BIN)%: $(OBJ)%.o
	@if [ -d $(BIN) ]; then : ; else mkdir $(BIN); fi
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ)%.o: $(SRC)%.c $(HC)
	@if [ -d $(OBJ) ]; then : ; else mkdir $(OBJ); fi
	$(CC) $(CFLAGS) -o $@ -c $<

$(INCLUDE)%.h:
	@if [ -d $(INCLUDE) ]; then : ; else mkdir $(INCLUDE); fi

clean:
	rm -rf $(OBJ)*.o $(BIN)*

curl:
	curl --header "X-Forwarded-For: 192.168.0.1" 127.0.0.1:7100/samples/test.txt
	curl --header "X-Forwarded-For: 192.168.0.2" 127.0.0.1:7100/samples/test.txt
	curl --header "X-Forwarded-For: 192.168.0.3" 127.0.0.1:7100/samples/test.txt
	curl --header "X-Forwarded-For: 192.168.0.4" 127.0.0.1:7100/samples/test.txt
