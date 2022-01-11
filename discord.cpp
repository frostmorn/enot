#ifdef GHOST_DISCORD
#include "discord.h"
#include "includes.h"
void DiscordSendMessageCorrutine(struct DiscordSendMessage *data){
    CURLcode res;
    struct curl_slist *headers = NULL;
    CURL* curl = curl_easy_init();
    if(curl) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, data->webhook_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);
     
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        auto s = data->json.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.c_str());
     
        res = curl_easy_perform(curl);
     
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */ 
        curl_easy_cleanup(curl);
        /* slist cleanup */
        curl_slist_free_all(headers);
    }
}
DiscordConnector::DiscordConnector(const std::string &webhook_url){
    curl_global_init(CURL_GLOBAL_ALL);
    if (!webhook_url.empty()){
        m_DiscordWebhookUrl = webhook_url;
        m_DiscordThread = new std::thread(&DiscordConnector::DiscordThreadLoop, this);
    }
}

DiscordConnector::~DiscordConnector(){
    if (m_DiscordThread){
        m_DiscordThread->join();
        delete m_DiscordThread;
    }
    curl_global_cleanup();
}

void DiscordConnector::Send(const nlohmann::json &json, const std::string &webhook_url){
    struct DiscordSendMessage dsendmsg_data;
    dsendmsg_data.json = json;
    dsendmsg_data.webhook_url = webhook_url;
    m_DiscordMessages.push(dsendmsg_data);
}
void DiscordConnector::Send(const nlohmann::json &json){
    Send(json, m_DiscordWebhookUrl);
}
void DiscordConnector::SendFile(const std::string &file_path, const nlohmann::json &json, const std::string &webhook_url){
    CURLcode res;
    CURL* curl = curl_easy_init();
    struct curl_slist *headers = NULL;
    curl_mime *form = NULL;
    curl_mimepart *file_field = NULL;
    curl_mimepart *json_field = NULL;

    if(curl) {

        form = curl_mime_init(curl);
    
        file_field = curl_mime_addpart(form);
        json_field = curl_mime_addpart(form);
        // Extract filename from file path
        auto filename = file_path.substr(file_path.find_last_of("/\\") + 1);

        curl_mime_filename(file_field, file_path.c_str());
        curl_mime_filedata(file_field, filename.c_str());
        if (json){
            auto s = json.dump();
            curl_mime_name(json_field,"payload_json");
            curl_mime_data(json_field, s.c_str(), CURL_ZERO_TERMINATED);
        }
        headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        // free form
        curl_mime_free(form);
        /* always cleanup */ 
        curl_easy_cleanup(curl);
            
        /* free slist */
        curl_slist_free_all(headers);
    }
}
void DiscordConnector::SendFile(const std::string &file_path, const nlohmann::json &json){
    SendFile(file_path, json, m_DiscordWebhookUrl);
}
void DiscordConnector::SendFile(const std::string &file_path){
    SendFile(file_path, 0);
}
void DiscordConnector::BugMessage(const std::string &bug_message, const std::string &bug_provider, const std::string &realm_provider, const std::string &webhook_url){

    nlohmann::json j_msg;
    nlohmann::json j_embed;
    j_embed["title"] = ":bug: BUG Reported!";
    j_embed["fields"][0]["name"] = ":crossed_swords: Realm:";
    j_embed["fields"][0]["value"] = realm_provider;
    j_embed["fields"][0]["inline"] = true;
    j_embed["fields"][1]["name"] = ":beer: Author:";
    j_embed["fields"][1]["value"] = bug_provider;
    j_embed["fields"][1]["inline"] = true;
    j_embed["fields"][2]["name"] = ":thinking: Bug message:";
    j_embed["fields"][2]["value"] = bug_message;
    j_embed["color"] = 0xFF0000;

    j_msg["embeds"][0]= j_embed;
    // std::cout<<j_msg.dump()<<std::endl;
    Send(j_msg, webhook_url);
}
void DiscordConnector::BugMessage(const std::string &bug_message, const std::string &bug_provider, const std::string &realm_provider){
    BugMessage(bug_message, bug_provider, realm_provider, m_DiscordWebhookUrl);
}

void DiscordConnector::DiscordThreadLoop(){
    while(1){
        // Handle discord loop
        while(!m_DiscordMessages.empty()){
            struct DiscordSendMessage discord_message = m_DiscordMessages.front();
            DiscordSendMessageCorrutine(&discord_message);
            m_DiscordMessages.pop();
        }
        MILLISLEEP(100);
    }

}
// int main(){
//     auto d = new DiscordConnector();
//     d->BugMessage("Hello");
// }
#endif