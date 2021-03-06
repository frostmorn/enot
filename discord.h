#ifdef GHOST_DISCORD
#ifndef DISCORD_H
#define DISCORD_H
#include <curl/curl.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

int discord_request(std::string json_data);
int discord_game_created(std::string discord_g_create_webhook_url, std::string game_name, std::string game_owner, std::string map_path);
int discord_bug_message(std::string discord_bug_webhook_url, std::string game_name,  std::string bug_provider, std::string bug_description);
#endif
#endif