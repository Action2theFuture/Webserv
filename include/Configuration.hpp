#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "Define.hpp"
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
    Configuration()
    {
    }
    std::vector<ServerConfig> servers; // 서버 설정 리스트

    // 구성 파일 파싱
    bool parseConfigFile(const std::string &filename);

    // 내부 출력, 초기화 함수
    void printConfiguration() const;
    ServerConfig initializeServerConfig();
    LocationConfig initializeLocationConfig(const std::string &line);

  private:
    // Rule of Three 준수를 위한 복사 금지
    Configuration(const Configuration &);
    Configuration &operator=(const Configuration &);

    // 파싱 관련 함수 (ConfigurationParse.cpp에 구현)
    void parseServerConfig(const std::string &line, ServerConfig &server_config);
    void parseLocationConfig(const std::string &line, LocationConfig &location_config);
    void processServerLine(const std::string &line, ServerConfig &server_config);
    void processLocationLine(const std::string &line, LocationConfig &location_config);
};

#endif // CONFIGURATION_HPP
