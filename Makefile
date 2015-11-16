
ifneq ($(strip $(enable_debug)),)
		ENABLE_DEBUG=1
endif
ENABLE_DEBUG?=0

ifneq ($(strip $(ENABLE_DEBUG)), 1)
		CFLAGS+= -g -w -O2
else
		CFLAGS+=-g -w -O0 -DDEBUG
endif

LIBS=-lpthread -luuid -lglog -lprotobuf -lconfig++
sources=main.cpp \
				util/status.cpp \
				util/crc32c.cpp \
				util/wal.cpp \
				util/file.cpp \
				util/level_file.cpp \
				core/options.cpp \
				core/item_table.cpp \
				core/user_table.cpp \
				core/candidate_db.cpp \
				service/service_glue.cpp \
				proto/record.pb.cc \
				proto/service.pb.cc

SOURCES=$(addprefix ./src/, $(sources))
all: 
	#protoc -I./deps/include -I./docs --cpp_out=./src/proto ./docs/record.proto ./docs/news_rsys.proto 
	g++ $(CFLAGS) -o bin/primary_election $(SOURCES) -I./src -I./deps/include -L./deps/lib $(LIBS)

clean:
	rm -f bin/primary_election
