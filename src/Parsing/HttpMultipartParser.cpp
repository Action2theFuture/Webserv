#include "HttpRequestParser.hpp"
#include "Utils.hpp" // trimString 함수 등 포함
#include <algorithm>
#include <iostream>

bool Parser::parseMultipartFormData(const std::string &body, const std::string &boundary, ParsedRequest &req)
{
    std::string delimiter = "--" + boundary;
    size_t pos = 0, body_length = body.size();
    while ((pos = body.find(delimiter, pos)) != std::string::npos)
    {
        pos += delimiter.length();
        if (pos + 2 <= body_length && body.compare(pos, 2, "--") == 0)
            break;
        if (pos + 2 <= body_length && body.compare(pos, 2, "\r\n") == 0)
            pos += 2;
        else
            return false;
        size_t headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos)
            return false;
        std::string part_headers_str = body.substr(pos, headers_end - pos);
        pos = headers_end + 4;
        std::map<std::string, std::string> part_headers;
        if (!parsePartHeaders(part_headers_str, part_headers))
            return false;
        std::map<std::string, std::string>::iterator cd_it = part_headers.find("Content-Disposition");
        if (cd_it == part_headers.end())
            continue;
        std::map<std::string, std::string> disp;
        if (!parseContentDisposition(cd_it->second, disp))
            continue;
        if (disp.find("name") == disp.end())
            continue;
        std::string field_name = disp["name"];
        if (disp.find("filename") != disp.end())
        {
            size_t next_boundary = body.find(delimiter, pos);
            size_t data_end = (next_boundary != std::string::npos) ? (next_boundary - 2) : body_length;
            if (data_end < pos)
                return false;
            UploadedFile file;
            file.name = field_name;
            file.filename = disp["filename"];
            if (part_headers.find("Content-Type") != part_headers.end())
                file.content_type = part_headers["Content-Type"];
            else
                file.content_type = "application/octet-stream";
            file.data.assign(body.begin() + pos, body.begin() + data_end);
            file.filesize = file.data.size();
            pos = data_end;
            req.uploaded_files.push_back(file);
        }
        else
        {
            size_t next_boundary = body.find(delimiter, pos);
            size_t data_end = (next_boundary != std::string::npos) ? (next_boundary - 2) : body_length;
            if (data_end < pos)
                return false;
            std::string field_value = body.substr(pos, data_end - pos);
            pos = data_end;
            req.form_fields[field_name] = field_value;
        }
    }
    return true;
}

bool Parser::parsePartHeaders(const std::string &headers_str, std::map<std::string, std::string> &part_headers)
{
    size_t start = 0;
    while (1)
    {
        size_t line_end = headers_str.find("\r\n", start);
        if (line_end == std::string::npos)
        {
            if (start < headers_str.size())
                parseHeaderLine(headers_str.substr(start), part_headers);
            break;
        }
        if (line_end == start)
            break;
        std::string line = headers_str.substr(start, line_end - start);
        parseHeaderLine(line, part_headers);
        start = line_end + 2;
    }
    return true;
}

bool Parser::parseContentDisposition(const std::string &disposition_str, std::map<std::string, std::string> &disp_map)
{
    size_t start = disposition_str.find(';');
    if (start == std::string::npos)
        return true;
    start++; // 첫 토큰 건너뛰기
    while (1)
    {
        size_t next = disposition_str.find(';', start);
        std::string token =
            (next == std::string::npos) ? disposition_str.substr(start) : disposition_str.substr(start, next - start);
        trimString(token);
        size_t eq = token.find('=');
        if (eq != std::string::npos)
        {
            std::string key = token.substr(0, eq);
            std::string value = token.substr(eq + 1);
            trimString(key);
            trimString(value);
            if (!value.empty() && value[0] == '"' && value[value.size() - 1] == '"')
                value = value.substr(1, value.size() - 2);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            disp_map[key] = value;
        }
        if (next == std::string::npos)
            break;
        start = next + 1;
    }
    return true;
}
