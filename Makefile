BUILD_PATH:=.build

INCLUDE_PATH:=
SOURCE_PATH:=src

TARGET_MAIN=candb
TARGET_UNITTEST=unittest

INCLUDES=-Isrc -Ideps/include -Ideps/include/db
LDFLAGS=-Ldeps/lib -L$(BUILD_PATH)/lib
LIBS=-ldb -lrdkafka++ -lrdkafka -ljson -lutils -lpthread -luuid \
		 -lglog -lprotobuf -lconfig++ -lcrypto -lgrpc -lgpr \
		 -lgrpc_unsecure -lgrpc++_unsecure -lgflags -lz

ifneq ($(strip $(debug)),)
	DEBUG=1
endif
DEBUG?=0

ifneq ($(strip $(prefix)),)
	PREFIX=$(prefix)
endif
PREFIX?=/usr/include

CXXFLAGS:=-g -w
ifneq ($(strip $(DEBUG)), 1)
	CXXFLAGS+= -O2 -DNDEBUG
else
	CXXFLAGS+= -O0 -DDEBUG -DTRACE
endif

ifeq ($(shell g++ -dumpversion), 4.8)
	CXXFLAGS+= -std=c++11 -DCPP11
else
	CXXFLAGS+= -std=c++0x
endif

all: dirs main unittest 
.PHONY: all

# 包含cpp文件列表
include ./src/Makefile.dep
OBJS=$(addprefix $(BUILD_PATH)/build/, $(sources:.cpp=.o))
OBJS_MAIN=$(addprefix $(BUILD_PATH)/build/, $(main:.cpp=.o))
OBJS_UNITTEST=$(addprefix $(BUILD_PATH)/build/, $(unittests:.cpp=.o))

.PHONY: main
main: $(OBJS) $(OBJS_MAIN)
	@echo $(CXX) $(CXXFLAGS) -o $(BUILD_PATH)/bin/$(TARGET_MAIN)
	@$(CXX) -o $(BUILD_PATH)/bin/$(TARGET_MAIN) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $^ $(LIBS) 

.PHONY: unittest
unittest: $(OBJS) $(OBJS_UNITTEST)
	@echo $(CXX) $(CXXFLAGS) -o $(BUILD_PATH)/bin/$(TARGET_UNITTEST)
	@$(CXX) -o $(BUILD_PATH)/bin/$(TARGET_UNITTEST) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $^ $(LIBS) 

.PHONY: proto
proto:
	protoc -I./docs -I../../deps/src/db/docs --cpp_out=./src/proto ./docs/service.proto
	@sed "s/message.pb.h/proto\/message.pb.h/" ./src/proto/service.pb.h  > ./src/proto/service.pb.h.tmp
	@mv ./src/proto/service.pb.h.tmp ./src/proto/service.pb.h
	@mv ./src/proto/service.pb.cc ./src/proto/service.pb.cpp
	protoc -I./docs -I../../deps/src/db/docs --grpc_out=./src/proto --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin ./docs/service.proto
	@mv ./src/proto/service.grpc.pb.cc ./src/proto/service.grpc.pb.cpp

$(BUILD_PATH)/build/%.o: $(SOURCE_PATH)/%.cpp $(BUILD_PATH)/build/%.d
	@echo $(CXX) $(CFLAGS) $(CXXFLAGS) $<
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -MT $@ -MMD -MP -MF $(BUILD_PATH)/build/$*.Td -c -o $@ $<
	@mv $(BUILD_PATH)/build/$*.Td $(BUILD_PATH)/build/$*.d

$(BUILD_PATH)/build/%.d: ;

-include $(patsubst %,$(BUILD_PATH)/build/%.d,$(basename $(sources)))

.PHONY: check
check: all
	@$(BUILD_PATH)/bin/$(TARGET_UNITTEST)

.PHONY: install
install: all
	@mkdir -p $(PREFIX)
	@cp ./conf/candb.conf $(PREFIX)
	@cp $(BUILD_PATH)/bin/$(TARGET_MAIN) $(PREFIX)
	
.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/candb.conf
	rm -f $(PREFIX)/$(TARGET_MAIN)

.PHONY: clean
clean:
	@rm -rf $(BUILD_PATH)

.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_PATH)
	@mkdir -p $(BUILD_PATH)/bin
	@mkdir -p $(BUILD_PATH)/lib
	@mkdir -p $(BUILD_PATH)/build

.PHONY: help
help:
	@echo "Usage: make [options] [target]"
	@echo
	@echo "Options:"
	@echo "  debug[=FLAG]   : flag: 0 ndebug, 1 debug"
	@echo "  prefix[=PATH]  : install path, default: /usr/include"
	@echo 
	@echo "Target:"
	@echo "  build          : Build target"
	@echo "  install        : Install target to prefix path"
	@echo "  check          : Run unittest"
	@echo "  rebuild        : Rebuild target"
	@echo "  uninstall      : Uninstall from prefix path"
	@echo "  clean          : Clean target and objects"
	@echo "  help           : Print help"
