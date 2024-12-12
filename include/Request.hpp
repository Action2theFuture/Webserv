#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <sstream>

class Request {
public:
    Request();
    ~Request();
    
    bool parse(const std::string &data);
    
    std::string getMethod() const;
    std::string getPath() const;
    std::string getQueryString() const;
    std::map<std::string, std::string> getHeaders() const;
    std::string getBody() const;

private:
    std::string method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::string body;

    bool parseRequestLine(const std::string &line);
    bool parseHeaderLine(const std::string &line);
};

#endif
