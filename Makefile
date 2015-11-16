
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
				util/options.cpp \
				util/crc32c.cpp \
				util/wal.cpp \
				util/file.cpp \
				util/level_file.cpp \
				core/item_table.cpp \
				core/user_table.cpp \
				core/candidate_db.cpp \
				service/service_glue.cpp \
				proto/news_rsys.pb.cc \
				proto/log_record.pb.cc
SOURCES=$(addprefix ./src/, $(sources))
all: 
	g++ $(CFLAGS) -o bin/primary_election $(SOURCES) -I./src -I./deps/include -L./deps/lib $(LIBS)

clean:
	rm -f bin/primary_election
