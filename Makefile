CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Icommon/include -Icommon -Iclient/include -Iserver/include

CLIENT_FLAGS = -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

.PHONY: all server client clean run-server run-client test

COMMON_SRC = common/src/*.cpp
SERVER_SRC = server/src/Server.cpp
CLIENT_SRC = client/src/Client.cpp

TEST_DEPS = $(COMMON_SRC) $(SERVER_SRC) $(CLIENT_SRC)

all: server client

client:
	@echo "Building client"
	@mkdir -p client/bin
	$(CXX) $(CXXFLAGS) client/src/*.cpp $(COMMON_SRC) $(CLIENT_FLAGS) -o client/bin/client

server:
	@echo "Building server"
	@mkdir -p server/bin
	$(CXX) $(CXXFLAGS) server/src/*.cpp $(COMMON_SRC) -o server/bin/server

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
	./client/bin/client

clean:
	rm -rf server/bin client/bin tests/bin

