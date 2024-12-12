#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include <string>

class Response {
public:
    Response();
    ~Response();
    
    static Response createResponse(const Request &request);
    std::string toString() const;

private:
    std::string status;
    std::map<std::string, std::string> headers;
    std::string body;

    void setStatus(const std::string &status_code);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &content);
};

#endif
