all:
	g++ -std=c++17 -I$(HOME)/local/include -L$(HOME)/local/lib -g -O2 WsRaccoonClient.cc secret.cpp main.cpp -lwebsockets -lcurl -lz -pthread -o rolbot
