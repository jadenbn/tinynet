CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Icommon/include -Icommon -Iclient/include -Iserver/include -Ishared/include

CLIENT_FLAGS = -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

.PHONY: all server client clean run-server run-client test

COMMON_SRC = common/src/*.cpp
SHARED_SRC = shared/src/*.cpp
SERVER_SRC = server/src/Server.cpp
CLIENT_SRC = client/src/Client.cpp

TEST_DEPS = $(COMMON_SRC) $(SERVER_SRC) $(CLIENT_SRC) $(SHARED_SRC)

all: server client

client:
	@echo "Building client"
	@mkdir -p client/bin
	$(CXX) $(CXXFLAGS) client/src/*.cpp $(COMMON_SRC) $(SHARED_SRC) $(CLIENT_FLAGS) -o client/bin/client

server:
	@echo "Building server"
	@mkdir -p server/bin
	$(CXX) $(CXXFLAGS) server/src/*.cpp $(COMMON_SRC) $(SHARED_SRC) -o server/bin/server

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

