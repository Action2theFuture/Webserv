#ifndef PARSER_HPP
#define PARSER_HPP

#include "LocationConfig.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// 파싱 결과를 한 번에 전달하기 위한 구조체
struct ParsedRequest
{
    std::string method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> headers;
    std::string body;
    std::vector<UploadedFile> uploaded_files;
    std::map<std::string, std::string> form_fields;
    int consumed;
    bool isPartial;
    std::string httpVersion;
};

class Parser
{
  public:
    Parser();
    ~Parser();
    // 외부에서 호출하는 파싱 함수 (인자 2개)
    bool parse(const std::string &data, ParsedRequest &req);

  private:
    // Rule of Three 준수를 위한 복사 금지
    Parser(const Parser &);
    Parser &operator=(const Parser &);

    // HttpRequestParser.cpp – 메인 파싱 로직 (각 함수 25줄 이하)
    bool parseRequestLine(const std::string &line, ParsedRequest &req);
    bool parseHeaders(const std::string &data, size_t &offset, std::map<std::string, std::string> &headers,
                      bool &isPartial);

    // HttpParserUtils.cpp – 유틸리티 함수들
    bool parseHeaderLine(const std::string &line, std::map<std::string, std::string> &headers);
    bool extractBoundary(const std::string &content_type, std::string &boundary);

    // HttpMultipartParser.cpp – multipart/form-data 파싱 관련 함수들
    bool parseMultipartFormData(const std::string &body, const std::string &boundary, ParsedRequest &req);
    bool parsePartHeaders(const std::string &headers_str, std::map<std::string, std::string> &part_headers);
    bool parseContentDisposition(const std::string &disposition_str,
                                 std::map<std::string, std::string> &disposition_map);
};

#endif // PARSER_HPP
