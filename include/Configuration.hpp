#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "Log.hpp"
#include "ServerConfig.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// 전역 유틸리티 함수로 선언
const LocationConfig *findBestMatchingLocation(const std::string &path, const ServerConfig &server_config);

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

    unsigned long parseSize(const std::string &value);
    void parseServerConfig(const std::string &line, ServerConfig &server_config);
    void parseLocationConfig(const std::string &line, LocationConfig &location_config);
    void printConfiguration() const;
};

#endif // CONFIGURATION_HPP