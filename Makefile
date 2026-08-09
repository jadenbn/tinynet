CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Icommon/include -Icommon

CLIENT_FLAGS = -Iclient/include -I/opt/homebrew/include -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

.PHONY: all server client clean run-server

SERVER_SRC = server/src/main.cpp
CLIENT_SRC = client/src/main.cpp

all: server client

client:
	@echo "Building client"
	@mkdir -p client/bin
	$(CXX) $(CXXFLAGS) $(CLIENT_FLAGS) $(CLIENT_SRC) -o client/bin/client


server:
	@echo "Building server"
	@mkdir -p server/bin
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o server/bin/server

run-server:
	./server/bin/server

run-client:
	./client/bin/client

clean:
	rm -rf server/bin client/bin

