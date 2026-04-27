#pragma once

#include <string>

namespace core::network {

struct TextResult {
    bool ready = false;
    bool ok = false;
    std::string body;
};

bool isHttpUrl(const std::string& url);
std::string cacheFilePath(const std::string& key, const std::string& extension, const std::string& bucket);
bool downloadUrlToFile(const std::string& url, const std::string& localPath);
bool downloadUrlToString(const std::string& url, std::string& output);

void requestText(const std::string& key, const std::string& url);
TextResult textResult(const std::string& key);
bool consumeAnyTextReady();
void postNetworkReadyEvent();
void shutdown();

} // namespace core::network
