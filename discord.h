#ifndef DISCORD_H
#define DISCORD_H
#include <curl/curl.h>
#include <iostream>
#include <algorithm>
int discord_request(std::string json_data){
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
            curl_easy_setopt(curl, CURLOPT_URL, "https://discordapp.com/api/webhooks/734409157289181197/NNA5Kwr7UuQzns3Ogy5eIxUxQXkaQ-gUKes09WgBw2nhUUHAgnBMUPzVdCRTJTCpjx_6");
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

int discord_game_created(std::string game_name, std::string game_owner, std::string game_map)
{
    std::string json_data = "{\"username\":\"EN0T\",\"avatar_url\":\"https:\\/\\/i.imgur.com\\/PFe9z5O.png\",\"content\":\"\",\"embeds\":[{\"author\":{\"name\":\"\\u0415\\u043d\\u043e\\u0442\\u0438\\u043a\",\"icon_url\":\"https:\\/\\/i.imgur.com\\/PFe9z5O.png\"},\"title\":\"Created public game\",\"color\":0,\"fields\":[{\"name\":\"Game name\",\"value\":\""+game_name+"\",\"inline\":true},{\"name\":\"Game owner\",\"value\":\""+game_owner+"\",\"inline\":true},{\"name\":\"Map name\",\"value\":\""+game_map+"\"}]}]}";
	return discord_request(json_data);
}

#endif