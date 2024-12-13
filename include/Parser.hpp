#ifndef PARSER_HPP
#define PARSER_HPP

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
               std::map<std::string, std::string> &headers, std::string &body);

  private:
    bool parseRequestLine(const std::string &line, std::string &method, std::string &path, std::string &query_string);
    bool parseHeaderLine(const std::string &line, std::map<std::string, std::string> &headers);
};

#endif
