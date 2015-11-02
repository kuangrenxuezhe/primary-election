
ifneq ($(strip $(enable_debug)),)
		ENABLE_DEBUG=1
endif
ENABLE_DEBUG?=0

ifneq ($(strip $(ENABLE_DEBUG)), 1)
		CFLAGS+= -g -O2
else
		CFLAGS+=-g -O0 -DDEBUG
endif

all: 
	g++ $(CFLAGS) -o bin/primary_election ./src/*.cpp -I./deps/include -lpthread -luuid

clean:
	rm -f bin/primary_election
