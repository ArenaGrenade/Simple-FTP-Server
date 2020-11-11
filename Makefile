SERV=serv
CLI=cli
IP=127.0.0.1
PORT=8000

.PHONY: build debug server client

build:
	@g++ utils.cpp server.cpp -o $(SERV)
	@g++ utils.cpp client.cpp -o $(CLI)
debug:
	@g++ server.cpp -g -o $(SERV)
	@g++ client.cpp -g -o $(CLI)
server:
	@./$(SERV) $(IP) $(PORT)
client:	
	@./$(CLI) $(IP) $(PORT)