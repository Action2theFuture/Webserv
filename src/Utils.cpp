#include "Utils.hpp"

std::string getMimeType(const std::string &path)
{
    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos)
    {
        std::string ext = path.substr(dot + 1);
        if (ext == "html" || ext == "htm")
            return "text/html";
        if (ext == "css")
            return "text/css";
        if (ext == "js")
            return "application/javascript";
        if (ext == "png")
            return "image/png";
        if (ext == "jpg" || ext == "jpeg")
            return "image/jpeg";
        if (ext == "gif")
            return "image/gif";
        if (ext == "txt")
            return "text/plain";
        // 필요한 MIME 타입 추가
    }
    return "application/octet-stream"; // 기본 MIME 타입
}

std::string normalizePath(const std::string &path)
{
    std::string normalized;
    size_t i = 0;
    while (i < path.size())
    {
        if (path[i] == '/' && i + 1 < path.size() && path[i + 1] == '/')
        {
            // 연속된 슬래시 제거
            ++i;
            continue;
        }
        if (path[i] == '/' && i + 1 < path.size() && path[i + 1] == '.' && (i + 2 == path.size() || path[i + 2] == '/'))
        {
            // "/./" 제거
            i += 2;
            continue;
        }
        if (path[i] == '/' && i + 1 < path.size() && path[i + 1] == '.' && i + 2 < path.size() && path[i + 2] == '.' &&
            (i + 3 == path.size() || path[i + 3] == '/'))
        {
            // "/../" 제거
            // 이전 디렉토리 제거
            size_t last_slash = normalized.find_last_of('/');
            if (last_slash != std::string::npos)
            {
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

std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// C++98 호환을 위한 int to string 변환 함수 정의
std::string intToString(int number)
{
    std::stringstream ss;
    ss << number;
    return ss.str();
}

// 파일 이름 정제를 위한 함수 객체(디렉토리 트래버셜 방지)
struct IsNotAllowedChar
{
    bool operator()(char c) const
    {
        return !(isalnum(c) || c == '.' || c == '_');
    }
};

std::string sanitizeFilename(const std::string &filename)
{
    std::string sanitized = filename;
    // 경로 구분자 제거
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '/'), sanitized.end());
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\\'), sanitized.end());

    // 알파벳, 숫자, 점, 밑줄만 남김
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(), IsNotAllowedChar()), sanitized.end());

    return sanitized;
}

// 허용된 파일 확장자 확인
bool isAllowedExtension(const std::string &filename, const std::vector<std::string> &allowed_extensions)
{
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos)
    {
        return false; // 확장자가 없음
    }
    std::string ext = filename.substr(dot);
    return std::find(allowed_extensions.begin(), allowed_extensions.end(), ext) != allowed_extensions.end();
}

bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    }
    return true;
}

std::string toLower(const std::string &str)
{
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str;
}

void trimString(std::string &str)
{
    // 앞뒤 공백 제거 (C++98 호환)
    static const char *whitespaces = " \t\r\n";
    std::string::size_type start = str.find_first_not_of(whitespaces);
    if (start == std::string::npos)
    {
        str.clear();
        return;
    }
    std::string::size_type end = str.find_last_not_of(whitespaces);
    str = str.substr(start, end - start + 1);
}
