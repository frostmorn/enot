#include "discord.h"
#include <iostream>
std::string escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
            o << "\\u"
              << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
        } else {
            o << *c;
        }
    }
    return o.str();
}
int discord_request(std::string webhook_url, std::string json_data){
    return -2;
    if (webhook_url==""){
        return -1;
    }    
    CURL *curl;
        CURLcode res;
        struct curl_slist *list = NULL;
        /* In windows, this will init the winsock stuff */ 
        curl_global_init(CURL_GLOBAL_ALL);
        /* get a curl handle */ 
        curl = curl_easy_init();
        if(curl) {
            /* First set the URL that is about to receive our POST. This URL can
                just as well be a https:// URL if that is what should receive the
                data. */ 
            list = curl_slist_append(list, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            /* Now specify the POST data */ 
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());

            /* Perform the request, res will get the return code */ 
            res = curl_easy_perform(curl);
            /* Check for errors */ 
            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));

            /* always cleanup */ 
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
        return 0;    
}

int discord_game_created(std::string discord_g_create_webhook_url, std::string game_name, std::string game_owner, std::string map_path){
    game_name = escape_json(game_name);
    game_owner = escape_json(game_owner);
    map_path = escape_json(map_path);
    std::string json_data = "{\"username\":\"EN0T\",\"avatar_url\":\"https:\\/\\/i.imgur.com\\/PFe9z5O.png\",\"content\":\"\",\"embeds\":[{\"author\":{\"name\":\"\\u0415\\u043d\\u043e\\u0442\\u0438\\u043a\",\"icon_url\":\"https:\\/\\/i.imgur.com\\/PFe9z5O.png\"},\"title\":\"Created public game\",\"color\":0,\"fields\":[{\"name\":\"Game name\",\"value\":\""+game_name+"\",\"inline\":true},{\"name\":\"Game owner\",\"value\":\""+game_owner+"\",\"inline\":true},{\"name\":\"Map name\",\"value\":\""+map_path+"\"}]}]}";
	return discord_request(discord_g_create_webhook_url, json_data);
}


int discord_bug_message(std::string discord_bug_webhook_url, std::string game_name,  std::string bug_provider, std::string bug_description)
{
    game_name = escape_json(game_name);
    bug_provider = escape_json(bug_provider);
    bug_description = escape_json(bug_description);
    std::string json_data = 
        "{\n"
        "  \"username\" : \"Енотик\",\n"
        "  \"avatar_url\" : \"https://i.imgur.com/PFe9z5O.png\",\n"
        "  \"content\": \"\",\n"
        "  \"embeds\": [\n"
        "    {\n"
        "      \"author\": {\n"
        "        \"name\": \"Енотик\",\n"
        "        \"icon_url\": \"https://i.imgur.com/PFe9z5O.png\"\n"
        "      },\n"
        "      \"title\": \"New bug message\",\n"
        "      \"color\": 16711680,\n"
        "      \"fields\": [\n"
        "        {\n"
        "          \"name\": \"Game name\",\n"
        "          \"value\": \"" +game_name + "\",\n"
        "          \"inline\": true\n"
        "        },\n"
        "        {\n"
        "          \"name\": \"Bug provider\",\n"
        "          \"value\": \""+ bug_provider +"\",\n"
        "          \"inline\": true\n"
        "        },\n"
        "        {\n"
        "          \"name\": \"Bug descripton\",\n"
        "          \"value\": \""+ bug_description +"\",\n"
        "          \"inline\":false\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ]\n"
    "}";
    return discord_request(discord_bug_webhook_url, json_data);
}