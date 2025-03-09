#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "CGIHandler.hpp"
#include "Define.hpp"
#include "HttpRequestParser.hpp" // 이름 변경된 Parser 인터페이스
#include "Log.hpp"
#include "Response.hpp"
#include "ResponseHandlers.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp"

#include <map>
#include <sstream>
#include <string>
#include <vector>

// Request 객체는 HTTP 요청 파싱 결과를 저장합니다.
class Request
{
  public:
    Request();
    ~Request();

    // data를 파싱하여 consumed와 isPartial를 갱신합니다.
    bool parse(const std::string &data, int &consumed, bool &isPartial);

    // Getter
    const std::vector<UploadedFile> &getUploadedFiles() const;
    const std::map<std::string, std::string> &getFormFields() const;
    std::string getMethod() const;
    std::string getPath() const;
    std::string getQueryString() const;
    std::string getHTTPVersion() const;
    std::map<std::string, std::string> getQueryParams() const;
    std::map<std::string, std::string> getHeaders() const;
    std::string getBody() const;

    // Setter
    void setUploadedFiles(const std::vector<UploadedFile> &files);
    void setFormFields(const std::map<std::string, std::string> &fields);
    void setBody(const std::string &body_data);

  private:
    std::string _method;
    std::string _path;
    std::string _query_string;
    std::map<std::string, std::string> _queryParams;
    std::map<std::string, std::string> _headers;
    std::vector<UploadedFile> _uploaded_files;
    std::map<std::string, std::string> _form_fields;
    std::string _httpVersion;
    std::string _body;
};

#endif // REQUEST_HPP
