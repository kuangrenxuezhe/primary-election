
ifneq ($(strip $(enable_debug)),)
		ENABLE_DEBUG=1
endif
ENABLE_DEBUG?=0

ifneq ($(strip $(ENABLE_DEBUG)), 1)
		CFLAGS+= -g -w -O2
else
		CFLAGS+=-g -w -O0 -DDEBUG
endif

all: 
	g++ $(CFLAGS) -o bin/primary_election ./src/main.cpp ./src/core/*.cpp -I./src -I./deps/include -lpthread -luuid

clean:
	rm -f bin/primary_election
