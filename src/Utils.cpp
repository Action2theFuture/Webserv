#include "Utils.hpp"
#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>


std::string getMimeType(const std::string &path) {
    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot + 1);
        if (ext == "html" || ext == "htm") return "text/html";
        if (ext == "css") return "text/css";
        if (ext == "js") return "application/javascript";
        if (ext == "png") return "image/png";
        if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
        if (ext == "gif") return "image/gif";
        if (ext == "txt") return "text/plain";
        // 필요한 MIME 타입 추가
    }
    return "application/octet-stream"; // 기본 MIME 타입
}

std::string normalizePath(const std::string &path) {
    std::string normalized;
    size_t i = 0;
    while (i < path.size()) {
        if (path[i] == '/' && i + 1 < path.size() && path[i + 1] == '/') {
            // 연속된 슬래시 제거
            ++i;
            continue;
        }
        if (path[i] == '/' && i + 1 < path.size() && path[i + 1] == '.' &&
            (i + 2 == path.size() || path[i + 2] == '/')) {
            // "/./" 제거
            i += 2;
            continue;
        }
        if (path[i] == '/' && i + 1 < path.size() && path[i + 1] == '.' &&
            i + 2 < path.size() && path[i + 2] == '.' &&
            (i + 3 == path.size() || path[i + 3] == '/')) {
            // "/../" 제거
            // 이전 디렉토리 제거
            size_t last_slash = normalized.find_last_of('/');
            if (last_slash != std::string::npos) {
                normalized.erase(last_slash);
            }
            i += 3;
            continue;
        }
        normalized += path[i];
        ++i;
    }
    return normalized;
}

void logError(const std::string &message) {
    std::ofstream log_file("error.log", std::ios::app);
    if (log_file.is_open()) {
        log_file << message << std::endl;
        log_file.close();
    }
    // 추가적으로, 표준 에러에도 출력
    std::cerr << message << std::endl;
}