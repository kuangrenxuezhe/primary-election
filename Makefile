
ifneq ($(strip $(enable_debug)),)
		ENABLE_DEBUG=1
endif
ENABLE_DEBUG?=0

GCC_VERSION=$(shell g++ -dumpversion)

INCLUDES+=-I./src -I./deps/include  -I./rsys-util/src
CFLAGS+=-L./deps/lib
ifneq ($(strip $(ENABLE_DEBUG)), 1)
		CFLAGS+= -g -w -O2
else
		CFLAGS+= -g -w -O0 -DDEBUG
endif

ifeq ($(GCC_VERSION), 4.8)
	CFLAGS+= -std=c++11 -DCPP11
else
	CFLAGS+= -std=c++0x
endif

LIBS=-lpthread -luuid -lglog -lprotobuf -lconfig++ -lcrypto -lgrpc -lgpr -lgrpc++_unsecure

utils=status.cpp \
			crc32c.cpp \
			wal.cpp \
			file.cpp \
			table_file.cpp \
			ahead_log.cpp \
			table_base.cpp

sources=core/core_type.cpp \
		    core/options.cpp \
	      core/user_table.cpp \
		    core/item_table.cpp \
		    core/candidate_db.cpp \
				service/service_glue.cpp \
				service/service_grpc.cpp \
		    proto/record.pb.cc \
		    proto/message.pb.cc \
				proto/service.pb.cc \
				proto/service.grpc.pb.cc

unittests=unittest.cpp \
					core/user_table_test.cpp \
					core/item_table_test.cpp \
					core/candidate_db_test.cpp

UTILS=$(addprefix ./rsys-util/src/, $(utils))
SOURCES=$(addprefix ./src/, $(sources))
UNITTESTS=$(addprefix ./src/, $(unittests))

all: main unittest 

main:
	g++ $(CFLAGS) -o bin/candb ./src/main.cpp $(SOURCES) $(UTILS) $(INCLUDES) $(LIBS)

unittest: 
	g++ $(CFLAGS) -o bin/unittest $(SOURCES) $(UTILS) $(UNITTESTS) $(INCLUDES) $(LIBS)

clean:
	rm -f bin/candb bin/unittest
