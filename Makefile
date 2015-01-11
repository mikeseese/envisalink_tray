all: main server client

main:
	g++ -o bin/main src/main.cpp -lboost_system -lboost_thread -lpthread

server:
	g++ -o bin/server src/server.cpp -lboost_system -lboost_thread -lpthread

client:
	g++ -o bin/client src/client.cpp -lboost_system -lboost_thread -lpthread