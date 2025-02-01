#include "Parser.hpp"

bool Parser::parse(const std::string &data, std::string &method, std::string &path, std::string &query_string,
                   std::map<std::string, std::string> &queryParams, std::map<std::string, std::string> &headers,
                   std::string &body, std::vector<UploadedFile> &uploaded_files,
                   std::map<std::string, std::string> &form_fields, int &consumed, bool &isPartial)
{
    consumed = 0;
    isPartial = false;

    size_t line_end = data.find("\r\n");
    if (line_end == std::string::npos)
    {
        // 요청 라인도 불완전
        isPartial = true;
        return true; // true=에러 아님, 더 받아야 함
    }

    // 요청 라인 추출
    std::string request_line = data.substr(0, line_end);
    if (!parseRequestLine(request_line, method, path, query_string, queryParams))
    {
        // 문법 오류
        return false;
    }

    // 헤더 파싱
    size_t offset = line_end + 2; // "\r\n" 길이=2
    while (true)
    {
        // 다음 줄의 끝
        size_t next_end = data.find("\r\n", offset);
        if (next_end == std::string::npos)
        {
            // 헤더가 아직 더 있을 수 있으나 불완전
            isPartial = true;
            return true;
        }

        if (next_end == offset)
        {
            // 빈 줄 => 헤더 끝
            offset += 2;
            break;
        }

        // 헤더 한 줄 추출
        std::string header_line = data.substr(offset, next_end - offset);
        if (!parseHeaderLine(header_line, headers))
        {
            return false;
        }
        offset = next_end + 2;
    }
    // 바디 파싱
    int content_length = 0;
    if (headers.find("Content-Length") != headers.end())
    {
        content_length = std::atoi(headers["Content-Length"].c_str());
        if (content_length < 0)
            content_length = 0;
    }

    size_t total_needed = offset + content_length;
    if (data.size() < total_needed)
    {
        // 아직 바디가 덜 옴
        isPartial = true;
        return true;
    }

    // 바디 추출
    body = data.substr(offset, content_length);

    // multipart/form-data
    std::map<std::string, std::string>::iterator ct_it = headers.find("Content-Type");
    if (ct_it != headers.end())
    {
        std::string content_type = ct_it->second;
        // 소문자 변환 or find("multipart/form-data")
        if (content_type.find("multipart/form-data") != std::string::npos)
        {
            std::string boundary;
            if (!extractBoundary(content_type, boundary))
            {
                return false;
            }
            if (!parseMultipartFormData(body, boundary, uploaded_files, form_fields))
            {
                return false;
            }
        }
        // TODO: else if application/x-www-form-urlencoded 등 처리
    }

    // 완전히 한 요청 소비
    consumed = total_needed;
    return true;
}

bool Parser::parseRequestLine(const std::string &line, std::string &method, std::string &path,
                              std::string &query_string, std::map<std::string, std::string> &queryParams)
{
    // 예: "GET /upload?foo=bar HTTP/1.1"
    // 간단히 split 2~3개만 파싱
    // 좀 더 엄격히 할거면 find(' ') 2회
    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        return false;
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
    {
        // HTTP 버전 없는 단순 구조
        secondSpace = line.size();
    }

    method = line.substr(0, firstSpace);
    std::string url = line.substr(firstSpace + 1, secondSpace - (firstSpace + 1));

    // 쿼리 문자열 분리
    size_t qmark = url.find('?');
    if (qmark != std::string::npos)
    {
        query_string = url.substr(qmark + 1);
        path = url.substr(0, qmark);

        // 쿼리 스트링 파싱하여 키-값 쌍의 맵 생성
        std::stringstream ss(query_string);
        std::string pair;

        while (std::getline(ss, pair, '&'))
        {
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string::npos)
            {
                std::string key = urlDecode(pair.substr(0, eq_pos));
                std::string value = urlDecode(pair.substr(eq_pos + 1));
                queryParams[key] = value;
            }
            else
            {
                // '='가 없는 경우, 키만 존재
                std::string key = urlDecode(pair);
                queryParams[key] = "";
            }
        }
    }
    else
    {
        path = url;
        query_string.clear();
    }

    // (HTTP 버전 등은 secondSpace 뒤로 존재 가능)
    return true;
}

bool Parser::parseHeaderLine(const std::string &line, std::map<std::string, std::string> &headers)
{
    size_t pos = line.find(":");
    if (pos == std::string::npos)
        return false;
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    // trim
    trimString(key);
    trimString(value);
    headers[key] = value;
    return true;
}

bool Parser::parseMultipartFormData(const std::string &body, const std::string &boundary,
                                    std::vector<UploadedFile> &uploaded_files,
                                    std::map<std::string, std::string> &form_fields)
{
    std::string delimiter = "--" + boundary;
    size_t pos = 0;
    size_t body_length = body.size();

    while ((pos = body.find(delimiter, pos)) != std::string::npos)
    {
        pos += delimiter.length();
        if (pos + 2 <= body_length && body.compare(pos, 2, "--") == 0)
            break;

        // CRLF 건너뛰기
        if (pos + 2 <= body_length && body.compare(pos, 2, "\r\n") == 0)
            pos += 2;
        else
        {
            std::cerr << "Missing CRLF after boundary." << std::endl;
            return false;
        }

        // 헤더와 바디 분리
        size_t headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == std::string::npos)
        {
            std::cerr << "Headers not found in multipart." << std::endl;
            return false;
        }

        std::string part_headers_str = body.substr(pos, headers_end - pos);
        pos = headers_end + 4; // skip \r\n\r\n

        // 파트 헤더
        std::map<std::string, std::string> part_headers;
        if (!parsePartHeaders(part_headers_str, part_headers))
            return false;

        // Content-Disposition 등 파싱
        std::map<std::string, std::string>::iterator cd_it = part_headers.find("Content-Disposition");
        if (cd_it == part_headers.end())
        {
            continue;
        }
        std::map<std::string, std::string> disposition_map;
        if (!parseContentDisposition(cd_it->second, disposition_map))
            continue;

        if (disposition_map.find("name") == disposition_map.end())
            continue;
        std::string field_name = disposition_map["name"];

        // 파일 여부?
        if (disposition_map.find("filename") != disposition_map.end())
        {
            // 파일
            size_t next_boundary = body.find(delimiter, pos);
            size_t data_end = (next_boundary != std::string::npos) ? (next_boundary - 2) : body_length;
            if (data_end < pos)
                return false;

            UploadedFile file;
            file.name = field_name;
            file.filename = disposition_map["filename"];
            // Content-Type
            if (part_headers.find("Content-Type") != part_headers.end())
                file.content_type = part_headers["Content-Type"];
            else
                file.content_type = "application/octet-stream";

            file.data.assign(body.begin() + pos, body.begin() + data_end);
            pos = data_end;

            uploaded_files.push_back(file);
        }
        else
        {
            // 일반 form field
            size_t next_boundary = body.find(delimiter, pos);
            size_t data_end = (next_boundary != std::string::npos) ? (next_boundary - 2) : body_length;
            if (data_end < pos)
                return false;

            std::string field_value = body.substr(pos, data_end - pos);
            pos = data_end;
            form_fields[field_name] = field_value;
        }
    }

    return true;
}

bool Parser::extractBoundary(const std::string &content_type, std::string &boundary)
{
    // 끝에 세미콜론이 있을 때 제거
    std::string ct = content_type;
    if (!ct.empty() && ct[ct.size() - 1] == ';')
    {
        ct.erase(ct.size() - 1);
    }

    // 앞뒤 공백 제거
    trimString(ct);

    std::string boundary_prefix = "boundary=";
    size_t pos = content_type.find(boundary_prefix);
    if (pos == std::string::npos)
        return false;
    // boundary 부분 추출
    boundary = ct.substr(pos + boundary_prefix.length());

    // 혹시 boundary가 큰따옴표로 감싸져 있으면 제거
    if (boundary.size() >= 2 && boundary[0] == '"' && boundary[boundary.size() - 1] == '"')
    {
        boundary = boundary.substr(1, boundary.size() - 2);
    }

    trimString(boundary);
    return true;
}

bool Parser::parsePartHeaders(const std::string &headers_str, std::map<std::string, std::string> &part_headers)
{
    // '\r\n' 단위로 자르기
    size_t start = 0;
    while (true)
    {
        size_t line_end = headers_str.find("\r\n", start);
        if (line_end == std::string::npos)
        {
            // 마지막 라인
            if (start < headers_str.size())
            {
                std::string last_line = headers_str.substr(start);
                // parse key:value
                parseHeaderLine(last_line, part_headers);
            }
            break;
        }
        if (line_end == start)
        {
            // 빈 줄?
            break;
        }
        std::string line = headers_str.substr(start, line_end - start);
        parseHeaderLine(line, part_headers);
        start = line_end + 2;
    }
    return true;
}

bool Parser::parseContentDisposition(const std::string &header_value, std::map<std::string, std::string> &params)
{
    // "form-data; name=\"file\"; filename=\"Cat.jpg\""
    // 세미콜론 기준 분리
    size_t start = 0;
    // 첫 토큰(form-data)은 무시
    size_t sc = header_value.find(';', start);
    if (sc == std::string::npos)
        return true; // or false?

    start = sc + 1;
    while (true)
    {
        // 다음 세미콜론 위치
        size_t nextSc = header_value.find(';', start);
        std::string token;
        if (nextSc == std::string::npos)
        {
            token = header_value.substr(start);
        }
        else
        {
            token = header_value.substr(start, nextSc - start);
        }
        trimString(token);

        size_t eq_pos = token.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string key = token.substr(0, eq_pos);
            std::string value = token.substr(eq_pos + 1);

            trimString(key);
            trimString(value);

	    if (!value.empty() && value[0] == '"' && value[value.size() - 1] == '"')
            {
                value = value.substr(1, value.size() - 2);
            }
            // 소문자로 변환 (대소문자 무시)
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            params[key] = value;
        }

        if (nextSc == std::string::npos)
            break;
        start = nextSc + 1;
    }

    return true;
}
