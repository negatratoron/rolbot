#include <chrono>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

#include <curl/curl.h>
#include "picojson.h"
#include <uWS/uWS.h>

#include "taunts.h"

using std::cout;
using std::endl;

using json = picojson::value;
using jsobject = json::object;
using WS = uWS::WebSocket<uWS::CLIENT>*;

// defined in secret.cpp
extern std::string client_id;
extern std::string client_secret;
extern std::string token;

int heartbeat_sequence_num = 0;
int expected_heartbeat_ack = 0;

void heartbeat(WS ws) {
    if (expected_heartbeat_ack) {
	std::cout << "Expected heartbeat ack." << std::endl;
	ws->close(500);
    }
    else {
	expected_heartbeat_ack += 1;
    }
    jsobject pkt;
    pkt.insert({"op", json(1.0)});
    pkt.insert({"d", json(double(heartbeat_sequence_num))});
    ws->send(json(pkt).serialize().c_str());
    heartbeat_sequence_num += 1;
}

void heartbeatInterval(WS ws, int interval) {
    std::thread([ws, interval]() {
	while (true) {
	    heartbeat(ws);
	    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	}
    }).detach();
}

void send_message(std::string channel_id, std::string message) {
    CURL* curl = curl_easy_init();

    std::ostringstream url;
    url << "https://discordapp.com/api/channels/";
    url << channel_id;
    url << "/messages";
    std::string url_str = url.str();

    curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());

    jsobject msg;
    msg.insert({"content", json(message)});
    std::string postdata = json(msg).serialize();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postdata.size());

    FILE* blah = fopen("blah.txt", "w");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, blah);

    curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Authorization: Bot NDQ0MDU4MTY2MTMzNzg0NTc2.DdauQQ.NswV6T8IAEUl7sRLqmft0jeMtm4");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    cout << int(curl_easy_perform(curl)) << endl;

    curl_slist_free_all(headers);
}

void set_roles(std::string guild_id, std::string user_id, std::string role) {
    CURL* curl = curl_easy_init();

    std::ostringstream url;
    url << "https://discordapp.com/api/guilds/";
    url << guild_id;
    url << "/members/";
    url << user_id;
    std::string url_str = url.str();

    curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    jsobject msg;
    std::vector<json> roles;
    roles.push_back(json(role));
    msg.insert({"roles", json(roles)});
    std::string postdata = json(msg).serialize();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postdata.size());

    FILE* blah = fopen("blah.txt", "w");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, blah);

    curl_slist *headers = nullptr;
    std::string auth_header = "Authorization: Bot " + token;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    cout << int(curl_easy_perform(curl)) << endl;

    curl_slist_free_all(headers);
}

std::string identify_packet() {
    jsobject properties;
    properties.insert({"$os", json("linux")});
    properties.insert({"$browser", json("disco")});
    properties.insert({"$device", json("disco")});
    jsobject ident;
    ident.insert({"token", json(token)});
    ident.insert({"properties", json(properties)});
    jsobject pkt;
    pkt.insert({"op", json(2.0)});
    pkt.insert({"d", json(ident)});
    return json(pkt).serialize();
}

int main() {
    uWS::Hub h;

    h.onConnection([](WS ws, uWS::HttpRequest req) {
	std::cout << "Connected" << std::endl;
    });

    h.onDisconnection([](WS ws_, int code, char *message, size_t length) {
	char* str = new char[length + 1];
	memcpy(str, message, length);
	str[length] = 0;
	cout << "Disconnected" << endl << str << endl;
    });

    int heartbeat_interval;
    h.onMessage([&heartbeat_interval](WS ws, char *message, size_t length, uWS::OpCode opCode) {
      std::string str;
      str.resize(length + 1);
      memcpy(&str[0], message, length);
      str[length] = 0;
	json v;
	picojson::parse(v, str);
	jsobject o = v.get<jsobject>();
	std::string t;
	if (o["t"].is<std::string>()) {
	    t = o["t"].get<std::string>();
	}
	int op = o["op"].get<double>();
	if (op == 9) {
	    cout << "invalid session" << endl;
	    std::this_thread::sleep_for(std::chrono::seconds(5));
	    std::string identify = identify_packet();
	    ws->send(identify.c_str());
	}
	else if (op == 10) {
	    std::string identify = identify_packet();
	    ws->send(identify.c_str());

	    jsobject d = o["d"].get<jsobject>();
	    heartbeat_interval = d["heartbeat_interval"].get<double>();
	    heartbeatInterval(ws, heartbeat_interval);
	}
	else if (op == 11) {
	    expected_heartbeat_ack -= 1;
	}
	else if (t == "READY") {
	    cout << "ready" << endl;
	}
	else if (t == "TYPING_START") {}
	else if (t == "PRESENCE_UPDATE") {}
	else if (t == "GUILD_CREATE") {}
	else if (t == "GUILD_ROLE_UPDATE") {}
	else if (t == "GUILD_MEMBER_UPDATE") {}
	else if (t == "MESSAGE_DELETE") {}
	else if (t == "MESSAGE_CREATE") {
	    jsobject d = o["d"].get<jsobject>();
	    std::string content = d["content"].get<std::string>();
	    std::string guild_id = d["guild_id"].get<std::string>();
	    std::string channel_id = d["channel_id"].get<std::string>();
	    jsobject author = d["author"].get<jsobject>();
	    std::string user_id = author["id"].get<std::string>();
	    if (content == "!vinci") {
		set_roles(guild_id, user_id, "444049408095551489");
	    }
	    if (content == "!alin") {
		set_roles(guild_id, user_id, "444051164778987530");
	    }
	    if (content == "!cuotl") {
		set_roles(guild_id, user_id, "444051594980491264");
	    }
	    try {
		size_t taunt_chars;
		int taunt_num = stoi(content, &taunt_chars);
		if (taunt_num >= 1 && taunt_num <= 100) {
		    send_message(channel_id, taunts[taunt_num - 1]);
		}
	    }
	    catch (std::exception e) {
		// not a number, or entered number is out of range
	    }
	}
	else {
	    cout << "Unrecognized message." << endl;
	    cout << message << endl;
	}
    });

    while (true) {
	h.connect("wss://gateway.discord.gg/?v=6&encoding=json");
	h.run();
	std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
