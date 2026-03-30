#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <regex>
#include <sstream>
#include <vector>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;

// Configuration
std::string Server = "https://www.kogama.com";
std::string Webhook = "YOUR_WEBHOOK_URL_HERE";
std::string Avatar = "https://play-lh.googleusercontent.com/Oq0WGvSCr_I2c0TWNPSjYv_VjMOjhbEQoH6gwRXnR1Pn6jz2VRn6TpzrTuLIJ5pnkPBR";
int EMBED_COLOR = 16219718;

// Rate limiting
const int RATE_LIMIT_MS = 1000;
const int MAX_RETRIES = 3;

// Gap threshold, if this many consecutive gaps are hit, assume true end
// Tune this up if legitimate gaps wider than this exist on the platform
const int TRUE_END_THRESHOLD = 10;

std::chrono::steady_clock::time_point last_post_time;

int LAST_ID = 0;
int LATEST_KNOWN_ID = 0;
int consecutive_404 = 0;

std::string format_date(const std::string& iso_date)
{
    try {
        std::regex iso_regex(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2}))");
        std::smatch match;
        if (std::regex_search(iso_date, match, iso_regex)) {
            std::stringstream ss;
            ss << match[1].str() << "-" << match[2].str() << "-" << match[3].str() << " "
               << match[4].str() << ":" << match[5].str() << ":" << match[6].str();
            return ss.str();
        }
        return iso_date;
    } catch (...) {
        return iso_date;
    }
}

std::string format_image(const std::string& url)
{
    if (url.empty()) return Avatar;
    std::string result = url;

    if (result.find("Avatar_images") != std::string::npos) {
        size_t last_underscore = result.find_last_of('_');
        size_t last_dot = result.find_last_of('.');
        if (last_underscore != std::string::npos && last_dot != std::string::npos)
            result = result.substr(0, last_underscore) + result.substr(last_dot);
    }

    if (result.find("cache") != std::string::npos) {
        size_t cache_pos = result.find("cache");
        if (cache_pos != std::string::npos)
            result.replace(cache_pos, 5, "images");
    }

    if (result.find("http") == std::string::npos && !result.empty())
        result = "https:" + result;

    return result;
}

std::string clean_markdown(const std::string& text)
{
    std::string result = text;
    std::vector<std::string> chars = {"*", "_", "~", "`", ">", "\\"};
    for (const auto& ch : chars) {
        size_t pos = 0;
        while ((pos = result.find(ch, pos)) != std::string::npos) {
            result.insert(pos, "\\");
            pos += 2;
        }
    }
    return result;
}

std::string clean_html(const std::string& text)
{
    std::string result = text;
    std::regex tag_regex("<[^>]*>");
    result = std::regex_replace(result, tag_regex, "");
    return result;
}

std::string get_emoji_for_type(const std::string& type)
{
    if (type == "status_updated")   return "📝";
    if (type == "wall_post")        return "📢";
    if (type == "game_published")   return "🎮";
    if (type == "marketplace_buy")  return "🛒";
    if (type == "badge_earned")     return "🏆";
    if (type == "username_updated") return "🔄";
    return "📌";
}

// CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::pair<std::string, long> http_get_with_status(const std::string& url)
{
    CURL* curl = curl_easy_init();
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        std::cout << "[GET ERROR] " << curl_easy_strerror(res) << "\n";
        response.clear();
    }

    curl_easy_cleanup(curl);
    return {response, http_code};
}

bool http_post_with_retry(const std::string& url, const std::string& payload, int retry_count = 0)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_post_time).count();
    if (elapsed < RATE_LIMIT_MS && retry_count == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(RATE_LIMIT_MS - elapsed));

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);

    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && code == 204) {
        last_post_time = std::chrono::steady_clock::now();
        std::cout << "[POST] SUCCESS (204)\n";
        return true;
    }

    if (code == 429 && retry_count < MAX_RETRIES) {
        std::cout << "[POST] Rate limited, retrying... (" << retry_count + 1 << "/" << MAX_RETRIES << ")\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return http_post_with_retry(url, payload, retry_count + 1);
    }

    if (code != 204 && retry_count < MAX_RETRIES) {
        std::cout << "[POST] FAILED - HTTP " << code << ", retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return http_post_with_retry(url, payload, retry_count + 1);
    }

    return false;
}

bool http_post(const std::string& url, const std::string& payload)
{
    return http_post_with_retry(url, payload);
}

enum class FeedStatus {
    HAS_DATA,
    TRUE_END,
    GAP,
    FETCH_ERROR
};

FeedStatus classify_response(const std::string& response, long status)
{
    if (response.empty() || response[0] == '<')
        return FeedStatus::GAP;

    if (status == 404)
        return FeedStatus::GAP;

    if (status != 200)
        return FeedStatus::FETCH_ERROR;

    try {
        json parsed = json::parse(response);

        if (!parsed.contains("data"))
            return FeedStatus::FETCH_ERROR;

        const auto& data = parsed["data"];

        if (data.is_null())
            return FeedStatus::TRUE_END;

        if (data.is_array() && data.empty())
            return FeedStatus::TRUE_END;

        if (data.is_object() || (data.is_array() && !data.empty()))
            return FeedStatus::HAS_DATA;

    } catch (...) {
        return FeedStatus::FETCH_ERROR;
    }

    return FeedStatus::TRUE_END;
}

void send_discord_notification(const json& data)
{
    std::string type = data.value("feed_type", "unknown");
    std::string name = data.value("profile_username", "Unknown");
    int id = data.value("profile_id", 0);
    std::string created = data.value("created", "");
    std::string profile_image = format_image(data["profile_images"].value("large", ""));

    std::string description;
    json extra;

    try {
        if (data["_data"].is_string())
            extra = json::parse(data["_data"].get<std::string>());
    } catch (...) {}

    if (type == "status_updated") {
        description = "**Status update:**\n" + clean_html(clean_markdown(extra.value("status_message", "")));
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);
    }
    else if (type == "wall_post") {
        int other_id = data.value("other_profile_id", 0);
        std::string other_name = data.value("other_username", "Unknown");
        description = "**Wall post to:** [" + clean_markdown(other_name) + "](" + Server + "/profile/" + std::to_string(other_id) + "/)\n";
        description += "**Message:**\n" + clean_html(clean_markdown(extra.value("status_message", "")));
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);
    }
    else if (type == "game_published") {
        int planet_id = data.value("planet_id", 0);
        std::string planet_name = data.value("planet_name", "Unnamed");
        std::string planet_image = format_image(data["planet_images"].value("large", ""));
        description = "**Published game:** [" + clean_markdown(planet_name) + "](" + Server + "/games/play/" + std::to_string(planet_id) + "/)";
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);

        json embed = {
            {"title", clean_markdown(name)},
            {"url", Server + "/profile/" + std::to_string(id) + "/"},
            {"description", description},
            {"color", EMBED_COLOR},
            {"thumbnail", {{"url", profile_image}}},
            {"image", {{"url", planet_image}}},
            {"footer", {{"text", format_date(created)}}},
            {"timestamp", created}
        };
        json payload = {
            {"username", "KoGaMa Monitor"},
            {"Avatar_url", Avatar},
            {"embeds", {embed}},
            {"allowed_mentions", {{"parse", json::array()}}}
        };
        http_post(Webhook, payload.dump());
        return;
    }
    else if (type == "marketplace_buy") {
        int creator_id = extra.value("creditor_profile_id", 0);
        std::string creator_name = extra.value("creditor_username", "Unknown");
        std::string product_name = extra.value("product_name", "Unknown");

        std::string product_id;
        if (extra.contains("product_id")) {
            if (extra["product_id"].is_string())
                product_id = extra["product_id"].get<std::string>();
            else if (extra["product_id"].is_number())
                product_id = std::to_string(extra["product_id"].get<int>());
        }

        std::string Avatar_image = format_image(data["Avatar_images"].value("large", ""));
        description = "**Purchased Avatar:** [" + clean_markdown(product_name) + "](" + Server + "/marketplace/Avatar/" + product_id + "/)\n";
        description += "**Creator:** [" + clean_markdown(creator_name) + "](" + Server + "/profile/" + std::to_string(creator_id) + "/)";
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);

        json embed = {
            {"title", clean_markdown(name)},
            {"url", Server + "/profile/" + std::to_string(id) + "/"},
            {"description", description},
            {"color", EMBED_COLOR},
            {"thumbnail", {{"url", profile_image}}},
            {"image", {{"url", Avatar_image}}},
            {"footer", {{"text", format_date(created)}}},
            {"timestamp", created}
        };
        json payload = {
            {"username", "KoGaMa Monitor"},
            {"Avatar_url", Avatar},
            {"embeds", {embed}},
            {"allowed_mentions", {{"parse", json::array()}}}
        };
        http_post(Webhook, payload.dump());
        return;
    }
    else if (type == "badge_earned") {
        std::string badge_name = data.value("badge_name", "Unknown");
        std::string badge_image = format_image(data["badge_images"].value("large", ""));
        description = "**Earned badge:** " + clean_markdown(badge_name);
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);

        json embed = {
            {"title", clean_markdown(name)},
            {"url", Server + "/profile/" + std::to_string(id) + "/"},
            {"description", description},
            {"color", EMBED_COLOR},
            {"thumbnail", {{"url", profile_image}}},
            {"image", {{"url", badge_image}}},
            {"footer", {{"text", format_date(created)}}},
            {"timestamp", created}
        };
        json payload = {
            {"username", "KoGaMa Monitor"},
            {"Avatar_url", Avatar},
            {"embeds", {embed}},
            {"allowed_mentions", {{"parse", json::array()}}}
        };
        http_post(Webhook, payload.dump());
        return;
    }
    else if (type == "username_updated") {
        std::string old_name = extra.value("username_old", "Unknown");
        std::string new_name = extra.value("username_new", "Unknown");
        description = "**Username changed:**\n";
        description += "**Old:** " + clean_markdown(old_name) + "\n";
        description += "**New:** " + clean_markdown(new_name);
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);
    }
    else {
        description = "Unknown event type: " + type;
        description += "\n\n**Post ID:** " + std::to_string(LAST_ID);
        description += "\n**Profile ID:** " + std::to_string(id);
    }

    json embed = {
        {"title", clean_markdown(name)},
        {"url", Server + "/profile/" + std::to_string(id) + "/"},
        {"description", description},
        {"color", EMBED_COLOR},
        {"thumbnail", {{"url", profile_image}}},
        {"footer", {{"text", format_date(created)}}},
        {"timestamp", created}
    };
    json payload = {
        {"username", "KoGaMa Monitor"},
        {"Avatar_url", Avatar},
        {"embeds", {embed}},
        {"allowed_mentions", {{"parse", json::array()}}}
    };
    http_post(Webhook, payload.dump());
}

void loop()
{
    last_post_time = std::chrono::steady_clock::now();
    consecutive_404 = 0;

    while (true)
    {
        std::string url = Server + "/api/feed/0/" + std::to_string(LAST_ID) + "/";
        std::cout << "[REQ] " << url << "\n";

        std::pair<std::string, long> res = http_get_with_status(url);
        std::string response = res.first;
        long status = res.second;

        FeedStatus fs = classify_response(response, status);
        if (fs == FeedStatus::TRUE_END) {
            std::cout << "[END] True end at ID " << LAST_ID
                      << ". Polling every 10s...\n";
            std::ofstream("last_id.txt") << LAST_ID;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }
        if (fs == FeedStatus::GAP) {
            consecutive_404++;
            int Gap_start = LAST_ID - (consecutive_404 - 1);

            std::cout << "[GAP] ID " << LAST_ID << " (consecutive: "
                      << consecutive_404 << ", started at: " << Gap_start << ")\n";

            if (consecutive_404 >= TRUE_END_THRESHOLD) {
                std::cout << "[TRUE END] " << TRUE_END_THRESHOLD
                          << " consecutive gaps hit. Rewinding to " << Gap_start
                          << " and polling every 10s...\n";
                LAST_ID = Gap_start;
                consecutive_404 = 0;
                std::ofstream("last_id.txt") << LAST_ID;
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }
            LAST_ID++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (fs == FeedStatus::FETCH_ERROR) {
            std::cout << "[ERROR] Transient error at ID " << LAST_ID
                      << ". Retrying in 2s...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        consecutive_404 = 0;

        try {
            json parsed = json::parse(response);
            auto& data_array = parsed["data"];

            if (data_array.is_array()) {
                for (auto& item : data_array) {
                    if (item.contains("id") && item["id"].is_number()) {
                        int item_id = item["id"].get<int>();
                        if (item_id > LATEST_KNOWN_ID) {
                            LATEST_KNOWN_ID = item_id;
                            std::cout << "[UPDATE] Latest known ID: " << LATEST_KNOWN_ID << "\n";
                        }
                    }
                    send_discord_notification(item);
                }
            } else if (data_array.is_object()) {
                if (data_array.contains("id") && data_array["id"].is_number()) {
                    int item_id = data_array["id"].get<int>();
                    if (item_id > LATEST_KNOWN_ID) {
                        LATEST_KNOWN_ID = item_id;
                        std::cout << "[UPDATE] Latest known ID: " << LATEST_KNOWN_ID << "\n";
                    }
                }
                send_discord_notification(data_array);
            }

            LAST_ID++;
            std::ofstream("last_id.txt") << LAST_ID;
            std::cout << "[PROGRESS] Processed ID: " << (LAST_ID - 1)
                      << " -> Next: " << LAST_ID << "\n";

        } catch (const std::exception& e) {
            std::cout << "[JSON ERROR] " << e.what() << "\n";
            LAST_ID++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// Main
int main()
{
    std::ifstream file("last_id.txt");
    if (file.is_open()) {
        file >> LAST_ID;
        file.close();
        std::cout << "[LOAD] Resuming from ID: " << LAST_ID << "\n";
    } else {
        LAST_ID = 31777744;
        std::cout << "[START] No last_id.txt found, starting at: " << LAST_ID << "\n";
    }

    LATEST_KNOWN_ID = LAST_ID;

    curl_global_init(CURL_GLOBAL_ALL);

    std::cout << "[MONITOR] KoGaMa Feed Monitor Started\n";
    std::cout << "[MONITOR] Rate limit:    " << RATE_LIMIT_MS << "ms between posts\n";
    std::cout << "[MONITOR] True end gap:  " << TRUE_END_THRESHOLD << " consecutive missing IDs\n";
    std::cout << "[MONITOR] On true end:   rewind to gap start, poll every 10s\n\n";

    loop();

    curl_global_cleanup();
    return 0;
}