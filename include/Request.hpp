#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "CGIHandler.hpp"
#include "Define.hpp"
#include "Log.hpp"
#include "Parser.hpp"
#include "Response.hpp"
#include "ResponseHandlers.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp"

#include <map>
#include <sstream>
#include <string>

class Request
{
  public:
    Request();
    ~Request();

    bool parse(const std::string &data, int &consumed, bool &isPartial);

    const std::vector<UploadedFile> &getUploadedFiles() const;
    const std::map<std::string, std::string> &getFormFields() const;

    std::string getMethod() const;
    std::string getPath() const;
    std::string getQueryString() const;
    std::map<std::string, std::string> getQueryParams() const;
    std::map<std::string, std::string> getHeaders() const;
    std::string getBody() const;

    void setUploadedFiles(const std::vector<UploadedFile> &files);
    void setFormFields(const std::map<std::string, std::string> &fields);
    void setBody(const std::string &body_data);

  private:
    std::string method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> headers;
    std::vector<UploadedFile> uploaded_files;
    std::map<std::string, std::string> form_fields;
    std::string body;

    bool parseRequestLine(const std::string &line);
    bool parseHeaderLine(const std::string &line);
};

#endif