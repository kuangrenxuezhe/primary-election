
ifneq ($(strip $(enable_debug)),)
		ENABLE_DEBUG=1
endif
ENABLE_DEBUG?=0

INCLUDES=-I./src -I./deps/include -L./deps/lib
ARFLAGS = rcs
ifneq ($(strip $(ENABLE_DEBUG)), 1)
		CFLAGS+= -g -w -O2
else
		CFLAGS+=-g -w -O0 -DDEBUG
endif

LIBS=-lpthread -luuid -lglog -lprotobuf -lconfig++ -lcrypto
sources=util/status.cpp \
				util/crc32c.cpp \
				util/wal.cpp \
				util/file.cpp \
				util/level_file.cpp \
				util/ahead_log.cpp \
				core/options.cpp \
				core/item_table.cpp \
				core/user_table.cpp \
				core/candidate_db.cpp \
				proto/record.pb.cc \
				proto/service.pb.cc

unittests=unittest.cpp \
					util/file_test.cpp \
					util/ahead_log_test.cpp \
					util/table_file_test.cpp \
					util/level_table_test.cpp \
					util/table_base_test.cpp \
					core/user_table_test.cpp \
					core/item_table_test.cpp

temp=util/status.cpp \
		 util/crc32c.cpp \
		 util/file.cpp \
		 util/table_file.cpp \
		 util/wal.cpp \
		 util/ahead_log.cpp \
		 util/table_base.cpp \
		 core/options.cpp \
	   core/user_table.cpp \
		 core/item_table.cpp \
		 core/candidate_db.cpp \
		 proto/record.pb.cc \
		 proto/message.pb.cc \
		 proto/service.pb.cc \
		 proto/service.grpc.pb.cc

#SOURCES=$(addprefix ./src/, $(sources))
SOURCES=$(addprefix ./src/, $(temp))
UNITTESTS=$(addprefix ./src/, $(unittests))

all: unittest 

#main: libcandset
	#protoc -I./deps/include -I./docs --cpp_out=./src/proto ./docs/record.proto ./docs/news_rsys.proto 
#	g++ $(CFLAGS) -o bin/primary_election main.cpp $(INCLUDES) $(LIBS) -lcandset

unittest: 
	g++ $(CFLAGS) -o bin/unittest $(SOURCES) $(UNITTESTS) $(INCLUDES) $(LIBS)
	#g++ $(CFLAGS) -o bin/unittest $(SOURCES) ./src/unittest.cpp $(INCLUDES) $(LIBS)

clean:
	rm -f bin/primary_election 
