#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "ServerConfig.hpp"
#include <string>
#include <vector>

// 전체 구성 관리 클래스
class Configuration
{
  public:
    std::vector<ServerConfig> servers; // 서버 설정 리스트

    // 구성 파일 파싱
    bool parseConfigFile(const std::string &filename);

  private:
    ServerConfig initializeServerConfig();
    LocationConfig initializeLocationConfig(const std::string &line);
    void parseServerConfig(const std::string &line, ServerConfig &server_config);
    void parseLocationConfig(const std::string &line, LocationConfig &location_config);
};

#endif // CONFIGURATION_HPP