#include "includes.h"
#ifdef GHOST_DISCORD
#include "discord.h"

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}
// Synchronius calls:
void DiscordSendMessageSync(struct DiscordSendMessage *data){
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
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
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

void DiscordSendFileMessageSync(struct DiscordSendFileMessage *data){
    CURLcode res;
    CURL* curl = curl_easy_init();
    curl_mime *form = NULL;
    curl_mimepart *file_field = NULL;
    curl_mimepart *json_field = NULL;
    struct curl_slist *headers = NULL;
    if(curl) {

        form = curl_mime_init(curl);
    
        
        json_field = curl_mime_addpart(form);
        file_field = curl_mime_addpart(form);

        // Extract filename from file path
        
        auto s = data->json.dump();
        curl_mime_name(json_field,"payload_json");
        curl_mime_data(json_field, s.c_str(), CURL_ZERO_TERMINATED);

        auto filename = data->file_path.substr(data->file_path.find_last_of("/\\") + 1);

        curl_mime_filename(file_field, filename.c_str());
        curl_mime_filedata(file_field, data->file_path.c_str());

        headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_URL, data->webhook_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
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


DiscordConnector::DiscordConnector(){
    curl_global_init(CURL_GLOBAL_ALL);
    m_DiscordThread = new std::thread(&DiscordConnector::DiscordThreadLoop, this);
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

void DiscordConnector::SendFile(const std::string &file_path, const nlohmann::json &json, const std::string &webhook_url){
    struct DiscordSendFileMessage dsendfmsg_data;
    dsendfmsg_data.json = json;
    dsendfmsg_data.webhook_url = webhook_url;
    dsendfmsg_data.file_path = file_path;
    m_DiscordFileMessages.push(dsendfmsg_data);
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

void DiscordConnector::ReplayMessage(const std::string &game_name, const std::string &map_name, const uint32_t &war3_version, const std::string &replay_file, const std::string &webhook_url)
{
    nlohmann::json j_msg;
    nlohmann::json j_embed;
    j_embed["title"] = ":arrow_forward: Replay saved!";
    j_embed["fields"][0]["name"] = ":video_game: Game name:";
    j_embed["fields"][0]["value"] = game_name;
    j_embed["fields"][0]["inline"] = true;
    j_embed["fields"][1]["name"] = ":map:  Map Name:";
    j_embed["fields"][1]["value"] = map_name;
    j_embed["fields"][1]["inline"] = true;
    j_embed["fields"][2]["name"] = ":anchor: Warcraft III version:";
    j_embed["fields"][2]["value"] = war3_version;
    j_embed["color"] = 0x00FF00;

    j_msg["embeds"][0]= j_embed;
    SendFile(replay_file, j_msg, webhook_url);
}
void DiscordConnector::DiscordThreadLoop(){
    while(1){
        // Handle discord loop
        while(!m_DiscordMessages.empty()){
            struct DiscordSendMessage discord_message = m_DiscordMessages.front();
            DiscordSendMessageSync(&discord_message);
            m_DiscordMessages.pop();
        }
         while(!m_DiscordFileMessages.empty()){
            struct DiscordSendFileMessage discord_file_message = m_DiscordFileMessages.front();
            DiscordSendFileMessageSync(&discord_file_message);
            m_DiscordFileMessages.pop();
        }
        MILLISLEEP(100);
    }

}
// int main(){
//     auto d = new DiscordConnector();
//     d->BugMessage("Hello");
// }
#endif