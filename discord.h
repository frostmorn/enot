#ifdef GHOST_DISCORD
#ifndef DISCORD_H
#define DISCORD_H
#include <curl/curl.h>
#include <nlohmann/json.hpp>
//#include <vector>
#include <thread>
#include <iostream>
#include <queue>
struct DiscordSendMessage{
    nlohmann::json json;
    std::string webhook_url;
};
struct DiscordSendFileMessage{
    nlohmann::json json;
    std::string file_path;
    std::string webhook_url;
};

class DiscordConnector{
    private:        
        std::queue<struct DiscordSendMessage > m_DiscordMessages;
        std::queue<struct DiscordSendFileMessage> m_DiscordFileMessages;
        std::thread *m_DiscordThread;
        void Send(const nlohmann::json &json, const std::string &webhook_url);
        void SendFile(const std::string &file_path, const nlohmann::json &json, const std::string &webhook_url);

    public:
        DiscordConnector();
        ~DiscordConnector();
        
        void BugMessage(const std::string &bug_message, const std::string &bug_provider, const std::string &realm_provider, const std::string &webhook_url);
        void ReplayMessage(const std::string &game_name, const std::string &map_name, const uint32_t &war3_version, const std::string &replay_file, const std::string &webhook_url);

        void DiscordThreadLoop();
};
#endif
#endif