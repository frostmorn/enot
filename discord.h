
#include <curl/curl.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

std::string escape_json(const std::string &s);
int discord_request(std::string json_data);
int discord_game_created(std::string game_name, std::string game_owner, std::string map_path);
int discord_bug_message(std::string game_name, std::string map_path, std::string bug_provider, std::string bug_description)