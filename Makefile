all:
	g++ secret.cpp main.cpp -luWS -lz -lcurl -lssl -pthread -o rolbot
