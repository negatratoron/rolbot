#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>

#include <curl/curl.h>
#include "picojson.h"
#include "WsRaccoonClient.h"

#include "taunts.h"

using json = picojson::value;
using jsobject = json::object;
using jsarray = json::array;

// defined in secret.cpp
extern std::string client_id;
extern std::string client_secret;
extern std::string token;

extern std::string rr_channel;
extern std::string rr_message;

extern std::string vinci_emoji_name;
extern std::string alin_emoji_name;
extern std::string cuotl_emoji_name;

extern std::string vinci_emoji;
extern std::string alin_emoji;
extern std::string cuotl_emoji;

extern std::string vinci_role;
extern std::string alin_role;
extern std::string cuotl_role;

WsRaccoonClient *wsClient;

int connected = 1;

int heartbeat_interval = -1;
int heartbeat_sequence_num = 0;
int expected_heartbeat_ack = 0;

std::string heartbeat() {
  jsobject pkt;
  pkt.insert({"op", json(1.0)});
  pkt.insert({"d", json(double(heartbeat_sequence_num))});
  heartbeat_sequence_num += 1;
  return json(pkt).serialize();
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
  headers = curl_slist_append(headers, (std::string{"Authorization: Bot "} + token).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_perform(curl);

  curl_slist_free_all(headers);
}

size_t curl_append_str(char *ptr, size_t size, size_t nmemb, void* str_p) {
  auto &str = *(std::string*)str_p;
  auto str_view = std::string{ptr, nmemb};
  str += str_view;
  return nmemb;
}

void
remove_role(std::vector<std::string>& vec, std::string const& role) {
  auto it = std::find(vec.begin(), vec.end(), role);
  if (it != vec.end()) {
    vec.erase(it);
  }
}

void
add_role(std::vector<std::string>& vec, std::string const& role) {
  auto it = std::find(vec.begin(), vec.end(), role);
  if (it == vec.end()) {
    vec.push_back(role);
  }
}

std::vector<std::string>
get_roles(std::string guild_id, std::string user_id) {
  CURL* curl = curl_easy_init();

  std::ostringstream url;
  url << "https://discordapp.com/api/guilds/";
  url << guild_id;
  url << "/members/";
  url << user_id;
  std::string url_str = url.str();

  curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());

  curl_slist *headers = nullptr;
  std::string auth_header = "Authorization: Bot " + token;
  headers = curl_slist_append(headers, auth_header.c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  std::string response;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_append_str);

  curl_easy_perform(curl);

  curl_slist_free_all(headers);

  auto got_roles = std::vector<std::string> {};

  json v;
  picojson::parse(v, response);
  jsobject o = v.get<jsobject>();
  auto user = o["user"].get<jsobject>();
  auto roles = o["roles"].get<jsarray>();

  for (auto const& role : roles) {
    got_roles.push_back(role.get<std::string>());
  }

  return got_roles;
}

void set_roles(std::string guild_id, std::string user_id, std::vector<std::string> set_roles) {
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
  for (auto const& role : set_roles) {
    roles.push_back(json(role));
  }
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
  curl_easy_perform(curl);

  curl_slist_free_all(headers);
  fclose(blah);
}

void react(std::string channel_id, std::string message_id, std::string emoji_name, std::string emoji) {
  CURL* curl = curl_easy_init();

  std::ostringstream url;
  url << "https://discordapp.com/api/channels/";
  url << channel_id;
  url << "/messages/";
  url << message_id;
  url << "/reactions/";
  url << emoji_name;
  url << ":";
  url << emoji;
  url << "/@me";
  std::string url_str = url.str();

  curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);

  FILE* blah = fopen("blah.txt", "w");
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, blah);

  curl_slist *headers = nullptr;
  std::string auth_header = "Authorization: Bot " + token;
  headers = curl_slist_append(headers, auth_header.c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_perform(curl);

  curl_slist_free_all(headers);
  fclose(blah);
}

std::string identify_packet() {
  jsobject properties;
  properties.insert({"$os", json("linux")});
  properties.insert({"$browser", json("disco")});
  properties.insert({"$device", json("disco")});
  jsobject ident;
  ident.insert({"token", json(token)});
  ident.insert({"intents", json((double)(
    (1 << 9) | // guild_messages
    (1 << 10) // guild_message_reactions
  ))});
  ident.insert({"properties", json(properties)});
  jsobject pkt;
  pkt.insert({"op", json(2.0)});
  pkt.insert({"d", json(ident)});
  return json(pkt).serialize();
}

class App : public WsClientCallback {
  void onConnSuccess(std::string *session_id_ptr) override {
    
  }
  void onReceiveMessage(std::string *message_ptr) override {
    auto const &str = *message_ptr;

    json v;
    picojson::parse(v, str);
    jsobject o = v.get<jsobject>();
    std::string t;
    if (o["t"].is<std::string>()) {
      t = o["t"].get<std::string>();
    }
    int op = o["op"].get<double>();
    if (op == 9) {
      std::cout << "invalid session" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::string identify = identify_packet();
      wsClient->sendMessage(&identify);
    }
    else if (op == 10) {
      jsobject d = o["d"].get<jsobject>();
      heartbeat_interval = d["heartbeat_interval"].get<double>();

      std::string identify = identify_packet();
      wsClient->sendMessage(&identify);
    }
    else if (op == 11) {
      expected_heartbeat_ack -= 1;
    }
    else if (t == "READY") {
      std::cout << "ready" << std::endl;
    }
    else if (t == "MESSAGE_DELETE") {}
    else if (t == "MESSAGE_UPDATE") {}
    else if (t == "CHANNEL_PINS_UPDATE") {}
    else if (t == "MESSAGE_CREATE") {
      jsobject d = o["d"].get<jsobject>();
      std::string content = d["content"].get<std::string>();
      std::string guild_id = d["guild_id"].get<std::string>();
      std::string channel_id = d["channel_id"].get<std::string>();
      jsobject author = d["author"].get<jsobject>();
      std::string user_id = author["id"].get<std::string>();
      if (content == "!vinci") {
        auto roles = get_roles(guild_id, user_id);

        add_role(roles, vinci_role);
        remove_role(roles, alin_role);
        remove_role(roles, cuotl_role);

        set_roles(guild_id, user_id, roles);
      }
      if (content == "!alin") {
        auto roles = get_roles(guild_id, user_id);

        remove_role(roles, vinci_role);
        add_role(roles, alin_role);
        remove_role(roles, cuotl_role);

        set_roles(guild_id, user_id, roles);
      }
      if (content == "!cuotl") {
        auto roles = get_roles(guild_id, user_id);

        remove_role(roles, vinci_role);
        remove_role(roles, alin_role);
        add_role(roles, cuotl_role);

        set_roles(guild_id, user_id, roles);
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
    else if (t == "MESSAGE_REACTION_ADD") {
      jsobject d = o["d"].get<jsobject>();
      if (rr_message == d["message_id"].get<std::string>()) {
        auto guild_id = d["guild_id"].get<std::string>();
        auto user_id = d["user_id"].get<std::string>();
        auto emoji_id = d["emoji"].get<jsobject>()["id"].get<std::string>();

        if (vinci_emoji == emoji_id) {
          auto roles = get_roles(guild_id, user_id);
          add_role(roles, vinci_role);
          set_roles(guild_id, user_id, roles);
        }
        else if (alin_emoji == emoji_id) {
          auto roles = get_roles(guild_id, user_id);
          add_role(roles, alin_role);
          set_roles(guild_id, user_id, roles);
        }
        else if (cuotl_emoji == emoji_id) {
          auto roles = get_roles(guild_id, user_id);
          add_role(roles, cuotl_role);
          set_roles(guild_id, user_id, roles);
        }
      }
    }
    else if (t == "MESSAGE_REACTION_REMOVE") {
      jsobject d = o["d"].get<jsobject>();
      if (rr_message == d["message_id"].get<std::string>()) {
        auto guild_id = d["guild_id"].get<std::string>();
        auto user_id = d["user_id"].get<std::string>();
        auto emoji_id = d["emoji"].get<jsobject>()["id"].get<std::string>();

        if (vinci_emoji == emoji_id) {
          auto roles = get_roles(guild_id, user_id);
          remove_role(roles, vinci_role);
          set_roles(guild_id, user_id, roles);
        }
        else if (alin_emoji == emoji_id) {
          auto roles = get_roles(guild_id, user_id);
          remove_role(roles, alin_role);
          set_roles(guild_id, user_id, roles);
        }
        else if (cuotl_emoji == emoji_id) {
          auto roles = get_roles(guild_id, user_id);
          remove_role(roles, cuotl_role);
          set_roles(guild_id, user_id, roles);
        }
      }
    }
    else if (t == "MESSAGE_REACTION_REMOVE_ALL") {}
    else if (t == "MESSAGE_REACTION_REMOVE_EMOJI") {}
    else {
      std::cout << "Unrecognized message." << std::endl;
      std::cout << str << std::endl;
    }
  }
  void onConnClosed() override {
    std::cout << "Connection closed!" << std::endl;
    connected = 0;
  }
  void onConnError() override {
    std::cout << "Connection error!" << std::endl;
    connected = 0;
  }
};

int main() {
  std::string url = "gateway.discord.gg";
  std::string path = "/?v=8&encoding=json";

  App app;
  wsClient = new WsRaccoonClient(&url, 443, &path, &app);
  wsClient->start();

  while (connected) {
    if (heartbeat_interval == -1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval - 1000));
    if (expected_heartbeat_ack) {
      std::cout << "expected heartbeat ack" << std::endl;
      wsClient->stop();
      return 1;
    }
    expected_heartbeat_ack += 1;
    auto packet = heartbeat();
    wsClient->sendMessage(&packet);
  }

  return 1;
}
