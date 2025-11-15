CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2
LDLIBS = -lws2_32

SERVER_SRC = chatRoom_basic.cpp
CLIENT_SRC = client_basic.cpp

SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

all: chatServer clientApp

chatServer: $(SERVER_OBJ)
	$(CXX) $(SERVER_OBJ) $(LDLIBS) -o chatServer.exe

clientApp: $(CLIENT_OBJ)
	$(CXX) $(CLIENT_OBJ) $(LDLIBS) -o clientApp.exe

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /Q *.o *.exe 2>nul || true

.PHONY: all clean
