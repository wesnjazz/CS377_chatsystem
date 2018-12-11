CXX=g++
CXXFLAGS += -g -Wall -Wextra -pthread
CPPFLAGS += -isystem src -std=c++14

MKDIR_P = mkdir -p
OBJ_DIR = obj

all: server client 

${OBJ_DIR}:
	${MKDIR_P} ${OBJ_DIR}

obj/%.o: src/%.cpp ${OBJ_DIR}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

server: obj/server.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

client: obj/main_client.o obj/client.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@


clean:
	rm -f server client tests
	rm -rf obj
	rm -f *~ obj/*.o obj/*.a *.zip
