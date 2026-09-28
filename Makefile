CXX ?= g++
CPPFLAGS ?=
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS ?=

# Optional local headers for machines without the system development package.
LOCAL_CURL_HEADER := $(firstword $(wildcard .deps/curl/usr/include/*/curl/curl.h .deps/curl/usr/include/curl/curl.h))
ifneq ($(LOCAL_CURL_HEADER),)
CURL_CPPFLAGS := -I$(patsubst %/curl/curl.h,%,$(LOCAL_CURL_HEADER))
CURL_LDLIBS := -l:libcurl.so.4
else
CURL_LDLIBS := -lcurl
endif
LDLIBS ?= $(CURL_LDLIBS) -pthread

SOURCES := $(wildcard src/*/*.cpp) src/main.cpp
OBJECTS := $(patsubst src/%.cpp,build/%.o,$(SOURCES))
LIB_OBJECTS := $(filter-out build/main.o,$(OBJECTS))
DEPS := $(OBJECTS:.o=.d) build/unit.d

.PHONY: all test clean local-deps
all: build/nginxwatch

local-deps:
	bash scripts/setup-local-curl.sh

build/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CURL_CPPFLAGS) -Iinclude $(CXXFLAGS) -MMD -MP -c $< -o $@

build/nginxwatch: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

build/unit.o: tests/unit.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CURL_CPPFLAGS) -Iinclude $(CXXFLAGS) -MMD -MP -c $< -o $@

build/unit_tests: build/unit.o $(LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

test: build/nginxwatch build/unit_tests
	./build/unit_tests
	python3 tests/integration.py ./build/nginxwatch

clean:
	rm -rf build

-include $(DEPS)
