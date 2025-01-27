#ifndef PARSER_HPP
#define PARSER_HPP

#include "LocationConfig.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

class Parser
{
  public:
    Parser()
    {
    }
    ~Parser()
    {
    }

    bool parse(const std::string &data, std::string &method, std::string &path, std::string &query_string,
               std::map<std::string, std::string> &headers, std::string &body,
               std::vector<UploadedFile> &uploaded_files, std::map<std::string, std::string> &form_fields,
               int &consumed, bool &isPartial);

  private:
    bool parseRequestLine(const std::string &line, std::string &method, std::string &path, std::string &query_string);
    bool parseHeaderLine(const std::string &line, std::map<std::string, std::string> &headers);
    // 파일 업로드 파싱 메서드
    bool parseMultipartFormData(const std::string &body, const std::string &boundary,
                                std::vector<UploadedFile> &uploaded_files,
                                std::map<std::string, std::string> &form_fields);
    bool extractBoundary(const std::string &content_type, std::string &boundary);
    bool parsePartHeaders(const std::string &headers_str, std::map<std::string, std::string> &part_headers);
    bool parseContentDisposition(const std::string &disposition_str,
                                 std::map<std::string, std::string> &disposition_map);
};

#endif
