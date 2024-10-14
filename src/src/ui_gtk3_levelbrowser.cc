#include "ui_gtk3_levelbrowser.hh"
#include "tms/backend/print.h"

#ifdef GTK3_LEVEL_BROWSER_ENABLE

#include "main.hh"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <SDL_mutex.h>

using json = nlohmann::json;

template <typename T>
std::unique_ptr<T> get_nullable(const nlohmann::json &j, const std::string &field_name) {
    if (j.contains(field_name) && !j.at(field_name).is_null()) {
        return std::make_unique<T>(j.at(field_name).get<T>());
    }
    return nullptr;
}

api::user::user(json &j) {
    id = j["id"];
    name = j.at("name").get<std::string>();
    customcolor = get_nullable<std::string>(j, "customcolor");
}

api::recent_level::recent_level(json &j): u(j) {
    id = j["id"];
    title = j["title"].get<std::string>();
}

api::level::level(json &j): u(j) {
    id = j["id"];
    cat = j["cat"];
    title = j["title"].get<std::string>();
    description = j["description"].get<std::string>();
    author = j["author"];
    time = j["time"];
    parent = get_nullable<uint32_t>(j, "parent");
    revision = j["revision"];
    revision_time = get_nullable<uint32_t>(j, "revision_time");
    visibility = j["visibility"];
    views = j["views"];
    likes = j["likes"];
    downloads = j["downloads"];
    platform = j["platform"].get<std::string>();
}

// TODO move to network.cc?

// TODO non-blocking request

std::vector<api::recent_level> api::get_recent_levels(uint32_t offset, uint32_t limit) {
    if (!P.curl) tms_fatalf("curl not initialized");
    SDL_LockMutex(P.curl_mutex);

    std::vector<api::recent_level> levels;

    char url[256];
    snprintf(url, 255, "https://%s/api/latest_levels?offset=%u&limit=%u", P.community_host, offset, limit);

    curl_easy_setopt(P.curl, CURLOPT_URL, url);

    std::string response;
    curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, &response);

    // TODO handle error
    CURLcode res = curl_easy_perform(P.curl);
    if (res != CURLE_OK) tms_fatalf("[fuck] curl error");

    // TODO handle error
    long http_code = 0;
    curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) tms_fatalf("[fuck] HTTP error %ld", http_code);

    SDL_UnlockMutex(P.curl_mutex);

    json data = json::parse(response);

    // Data contains an array of api::recent_level

    for (const auto& item : data) {
        levels.emplace_back(item);
    }

    return levels;
}

// TODO implement this
api::level api::get_level(uint32_t id) {
    tms_fatalf("not implemented"); exit(1);
}

#endif

