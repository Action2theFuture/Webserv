#include "Utils.hpp"

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

void ensureLogDirectoryExists() {
    const char* logDir = "./logs";
    struct stat st;
    if (stat(logDir, &st) == -1) {
        if (mkdir(logDir, 0755) == -1) {
            std::cerr << "Error: Failed to create log directory " << logDir << ": " << strerror(errno) << std::endl;
        } else {
            std::cerr << "Log directory created: " << logDir << std::endl;
        }
    }
}

void logError(const std::string &message) {
    ensureLogDirectoryExists();
    // 로그 파일 열기
    std::ofstream log_file(LOG_FILE, std::ios::app);
    
    // 로그 파일 열기에 실패한 경우 파일 생성 시도
    if (!log_file.is_open()) {
        // 파일 생성 시도
        std::ofstream create_file(LOG_FILE);
        if (!create_file.is_open()) {
            std::cerr << "Error: Failed to create log file " << LOG_FILE << std::endl;
            // 표준 에러로만 출력
            std::cerr << message << std::endl;
            return;
        }
        create_file.close();
        log_file.open(LOG_FILE, std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Error: Failed to open newly created log file " << LOG_FILE << std::endl;
            // 표준 에러로만 출력
            std::cerr << message << std::endl;
            return;
        }
    }

    // 로그 파일에 기록
    log_file << message << std::endl;
    log_file.close();

    // 표준 에러에도 출력
    std::cerr << message << std::endl;
}

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// C++98 호환을 위한 int to string 변환 함수 정의
std::string intToString(int number) {
    std::stringstream ss;
    ss << number;
    return ss.str();
}
