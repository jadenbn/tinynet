CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Icommon/include -Icommon -Iclient/include -Iserver/include -Ishared/include

CLIENT_CPPFLAGS = -I/opt/homebrew/include
CLIENT_LDLIBS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

.PHONY: all server client clean run-server run-client test

COMMON_SRC := $(wildcard common/src/*.cpp)
SHARED_SRC := $(wildcard shared/src/*.cpp)
SERVER_SRC := server/src/Server.cpp
CLIENT_SRC := client/src/Client.cpp

# Expand these in Make rather than leaving shell globs in the compiler command.
# Besides including source subdirectories, this lets compile database generators
# associate each command with its concrete translation units.
SERVER_APP_SRC := $(wildcard server/src/*.cpp server/src/*/*.cpp)
CLIENT_APP_SRC := $(wildcard client/src/*.cpp client/src/*/*.cpp)

SERVER_OBJECTS := $(addprefix server/bin/obj/,$(SERVER_APP_SRC:.cpp=.o) $(COMMON_SRC:.cpp=.o) $(SHARED_SRC:.cpp=.o))
CLIENT_OBJECTS := $(addprefix client/bin/obj/,$(CLIENT_APP_SRC:.cpp=.o) $(COMMON_SRC:.cpp=.o) $(SHARED_SRC:.cpp=.o))

TEST_DEPS = $(COMMON_SRC) $(SERVER_SRC) $(CLIENT_SRC) $(SHARED_SRC)

all: server client

client: client/bin/client

server: server/bin/server

client/bin/client: $(CLIENT_OBJECTS)
	@echo "Building client"
	$(CXX) $^ $(CLIENT_LDLIBS) -o $@

server/bin/server: $(SERVER_OBJECTS)
	@echo "Building server"
	$(CXX) $^ -o $@

client/bin/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLIENT_CPPFLAGS) -c $< -o $@

server/bin/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test:
	@echo "Building and running tests"
	@mkdir -p tests/bin
	@for test_file in tests/*.cpp; do \
		echo "Testing $$test_file"; \
		$(CXX) $(CXXFLAGS) $$test_file $(TEST_DEPS) -o tests/bin/$$(basename $$test_file .cpp) || exit 1; \
		./tests/bin/$$(basename $$test_file .cpp) || exit 1; \
	done

run-server:
	./server/bin/server

run-client:
ifndef PORT
	$(error PORT is not set. Usage: make deploy PORT=port. 3001 is a good default)
endif
	./client/bin/client $(PORT)

clean:
	@rm -rf server/bin client/bin tests/bin
