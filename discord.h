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

class DiscordConnector{
    private:        
        std::queue<struct DiscordSendMessage > m_DiscordMessages;
        std::thread *m_DiscordThread;
        std::string m_DiscordWebhookUrl;
        void Send(const nlohmann::json &json, const std::string &webhook_url);
        void Send(const nlohmann::json &json);
        void SendFile(const std::string &file_path, const nlohmann::json &json, const std::string &webhook_url);
        void SendFile(const std::string &file_path, const nlohmann::json &json);
        void SendFile(const std::string &file_path);
    public:
        DiscordConnector(const std::string &webhook_url);
        ~DiscordConnector();
        

        void BugMessage(const std::string &bug_message, const std::string &bug_provider, const std::string &realm_provider, const std::string &webhook_url);
        void BugMessage(const std::string &bug_message, const std::string &bug_provider, const std::string &realm_provider);
        void DiscordThreadLoop();
};
#endif
#endif