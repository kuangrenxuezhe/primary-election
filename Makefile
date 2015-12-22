BUILD=.build
TARGET=candb
UNITTESTS=unittest

SRCPATH=./src/
OBJPATH=$(BUILD)/objs/

ifneq ($(strip $(debug)),)
	DEBUG=1
endif
DEBUG?=0

ifneq ($(strip $(prefix)),)
	PREFIX=$(prefix)
endif
PREFIX?=/usr

GCC_VERSION=$(shell g++ -dumpversion)

INCLUDES=-I./src -I./deps/include
LDFLAGS=-L./deps/lib -L$(BUILD)/lib

ifneq ($(strip $(DEBUG)), 1)
	CFLAGS+= -g -w -O2 -DNDEBUG
else
	CFLAGS+=-g -w -O0 -DDEBUG -DTRACE
endif

ifeq ($(GCC_VERSION), 4.8)
	CFLAGS+= -std=c++11 -DCPP11
else
	CFLAGS+= -std=c++0x
endif

.SUFFIXES:
.PHONY: build install check rebuild uninstall clean help

LIBS=-lpthread -luuid -lglog -lprotobuf -lconfig++ -lcrypto -lgrpc -lgpr -lgrpc++_unsecure -lgflags

sources=core/core_type.cpp \
		    core/model_options.cpp \
	      core/user_table.cpp \
		    core/item_table.cpp \
		    core/candidate_db.cpp \
				service/service_glue.cpp \
				service/service_grpc.cpp \
		    proto/message.pb.cpp \
				proto/service.pb.cpp \
				proto/service.grpc.pb.cpp

unittests=unittest.cpp \
					core/user_table_test.cpp \
					core/item_table_test.cpp \
					core/candidate_db_test.cpp
				
OBJECTS=$(addprefix $(OBJPATH), $(sources:.cpp=.o))
OBJTESTS=$(addprefix $(OBJPATH), $(unittests:.cpp=.o))

build: mkdir $(TARGET) $(UNITTESTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(BUILD)/bin/$@ $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ $(LIBS)

$(UNITTESTS): $(OBJTESTS)
	$(CXX) -o $(BUILD)/bin/$@ $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ $(LIBS)

sinclude $(addprefix $(OBJPATH), $(sources:.cpp=.d))
sinclude $(addprefix $(OBJPATH), $(unittests:.cpp=.d))
$(OBJPATH)%.o: $(SRCPATH)%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) $(INCLUDES) $^ -c -o $@
	@$(CXX) -MM $(CFLAGS) $(INCLUDES) $^ > $(OBJPATH)/$*.d
	@mv -f $(OBJPATH)/$*.d $(OBJPATH)/$*.d.tmp
	@sed -e 's|.*:|$(OBJPATH)/$*.o:|' < $(OBJPATH)/$*.d.tmp > $(OBJPATH)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(OBJPATH)/$*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> $(OBJPATH)/$*.d
	@rm -f $(OBJPATH)/$*.d.tmp

check: build 
	@$(BUILD)/bin/unittest

install: build
	@mkdir -p $(PREFIX)/bin
	@cp $(BUILD)/lib/$(TARGET) $(PREFIX)/lib/
	
rebuild: clean build

uninstall:
	@rm -rf $(PREFIX)/include/db
	@rm -rf $(PREFIX)/include/proto
	@rm -f $(PREFIX)/lib/$(TARGET)

clean:
	@rm -rf $(BUILD)

mkdir:
	@mkdir -p $(BUILD)
	@mkdir -p $(BUILD)/bin
	@mkdir -p $(BUILD)/objs

help:
	@echo "Usage: make [options] [target]"
	@echo "Options:"
	@echo "  debug[=FLAG]   : flag: 0 ndebug, 1 debug"
	@echo "  prefix[=PATH]  : install path, default: /usr/include"
	@echo "Target:"
	@echo "  build          : Build target"
	@echo "  install        : Install target to prefix path"
	@echo "  check          : Run unittest"
	@echo "  rebuild        : Rebuild target"
	@echo "  uninstall      : Uninstall from prefix path"
	@echo "  clean          : Clean target and objects"
	@echo "  help           : Print help"
