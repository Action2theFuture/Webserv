#include "ResponseHandlers.hpp"

namespace ResponseHandlers
{
Response handleRedirection(const LocationConfig &location_config)
{
    Response res;
    res.setStatus("301 Moved Permanently");
    res.setHeader("Location", location_config.redirect);

    std::string body = "<h1>301 Moved Permanently</h1>";
    res.setBody(body);
    {
        std::stringstream ss_len;
        ss_len << body.size();
        res.setHeader("Content-Length", ss_len.str());
    }
    res.setHeader("Content-Type", "text/html");
    return res;
}

Response handleCGI(const Request &request, const std::string &real_path, const ServerConfig &server_config)
{
    Response res;
    CGIHandler cgi_handler;
    std::string cgi_output, cgi_content_type;

    if (!cgi_handler.execute(request, real_path, cgi_output, cgi_content_type))
    {
        return Response::createErrorResponse(500, server_config);
    }

    size_t pos = cgi_output.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        std::string headers = cgi_output.substr(0, pos);
        std::string body = cgi_output.substr(pos + 4);

        std::istringstream header_stream(headers);
        std::string header_line;
        while (std::getline(header_stream, header_line))
        {
            if (!header_line.empty() && !header_line.empty() && header_line[header_line.size() - 1] == '\r')
            {
                header_line.resize(header_line.size() - 1);
            }
            size_t colon = header_line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = trim(header_line.substr(0, colon));
                std::string value = trim(header_line.substr(colon + 1));
                res.setHeader(key, value);
            }
        }
        res.setStatus("200 OK");
        res.setBody(body);

        std::stringstream ss_len;
        ss_len << body.size();
        res.setHeader("Content-Length", ss_len.str());
        if (!cgi_content_type.empty())
        {
            res.setHeader("Content-Type", cgi_content_type);
        }
    }
    else
    {
        // 헤더 구분자 "\r\n\r\n" 없다면 전체를 body로 처리
        res.setStatus("200 OK");
        res.setBody(cgi_output);

        std::stringstream ss_len;
        ss_len << cgi_output.size();
        res.setHeader("Content-Length", ss_len.str());
        res.setHeader("Content-Type", "text/html");
    }
    return res;
}

Response handleStaticFile(const std::string &real_path, const ServerConfig &server_config)
{
    Response res;
    int fd = open(real_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return Response::createErrorResponse(404, server_config);
    }

    char buffer[BUFFER_SIZE];
    std::string file_content;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        file_content.append(buffer, bytes_read);
    }
    close(fd);

    if (bytes_read == -1)
    {
        perror("read");
        return Response::createErrorResponse(500, server_config);
    }

    std::string content_type = getMimeType(real_path);
    res.setStatus("200 OK");
    res.setBody(file_content);
    {
        std::stringstream ss_len;
        ss_len << file_content.size();
        res.setHeader("Content-Length", ss_len.str());
    }
    res.setHeader("Content-Type", content_type);

    return res;
}

Response handleUpload(const std::string &real_path, const Request &request, const LocationConfig &location_config,
                      const ServerConfig &server_config)
{
    using namespace ResponseUtils;
    Response res;
    // 파일 업로드 파싱
    const std::vector<UploadedFile> &files = request.getUploadedFiles();
    const std::map<std::string, std::string> &form_fields = request.getFormFields();

    if (files.empty())
    {
        std::cerr << "uploaded files are empty" << std::endl;
        res = Response::createErrorResponse(400, server_config); // Bad Request
        return res;
    }

    // 업로드 디렉토리 확인
    std::string upload_dir = location_config.upload_directory;
    std::cerr << "Upload directory 1 : " << upload_dir << std::endl;
    if (upload_dir.empty())
    {
        upload_dir = server_config.root; // 기본 루트 디렉토리 사용
    }
    std::cerr << "Upload directory 2 : " << upload_dir << std::endl;

    if (!ensureDirectoryExists(upload_dir))
    {
        // 생성 실패 or 디렉토리 아님
        return Response::createErrorResponse(500, server_config); // Internal Server Error
    }

    // 허용된 파일 확장자 목록 (필요 시 설정 파일에서 불러오기)
    // std::vector<std::string> allowed_extensions = {".jpg", ".jpeg", ".png", ".gif", ".txt", ".pdf"};
    std::vector<std::string> allowed_extensions = location_config.allowed_extensions;

    // 추가적인 폼 필드 처리 (예: description)
    std::string description;
    std::map<std::string, std::string>::const_iterator desc_it = form_fields.find("description");
    if (desc_it != form_fields.end())
    {
        description = desc_it->second;
    }

    std::vector<std::string> uploaded_filenames; // 업로드된 파일 이름을 저장할 벡터
    for (std::vector<UploadedFile>::const_iterator it = files.begin(); it != files.end(); ++it)
    {
        const UploadedFile &file = *it;

        // 파일 이름 정제
        std::string sanitized_filename = sanitizeFilename(file.filename);

        // 허용된 확장자 확인
        if (!isAllowedExtension(sanitized_filename, allowed_extensions))
        {
            LogConfig::logError("Disallowed extensions");
            return Response::createErrorResponse(400, server_config); // Bad Request
        }

        std::string file_path = upload_dir + "/" + sanitized_filename;

        // 파일 저장
        std::ofstream ofs(file_path.c_str(), std::ios::binary);
        if (!ofs.is_open())
        {
            std::cerr << "Failed to open file for writing: " << file_path << std::endl;
            return Response::createErrorResponse(500, server_config); // Internal Server Error
        }
        if (!file.data.empty())
        {
            ofs.write(&file.data[0], file.data.size());
        }
        ofs.close();

        // 업로드된 파일 이름 추가
        uploaded_filenames.push_back(sanitized_filename);
    }
    return handleStaticFile(real_path, server_config);
}

Response handleFileList(const Request &request, const LocationConfig &location_config,
                        const ServerConfig &server_config)
{
    using namespace ResponseUtils;

    std::string method = request.getMethod();
    if (iequals(method, "GET"))
    {
        std::vector<std::string> files;
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(location_config.upload_directory.c_str())) != NULL)
        {
            while ((ent = readdir(dir)) != NULL)
            {
                // 현재 디렉토리와 상위 디렉토리는 제외
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                    continue;

                // 파일인지 디렉토리인지 확인
                std::string filePath = std::string(location_config.upload_directory) + "/" + ent->d_name;
                struct stat st;
                if (stat(filePath.c_str(), &st) == 0)
                {
                    if (S_ISREG(st.st_mode))
                    {
                        files.push_back(ent->d_name);
                    }
                }
            }
            closedir(dir);
        }
        else
        {
            std::cerr << "디렉토리를 열 수 없습니다: " << location_config.upload_directory << std::endl;
            return Response::createErrorResponse(500, server_config);
        }
        // 파일 목록을 JSON으로 변환
        std::string jsonContent = generateFileListJSON(files);

        // 응답 생성
        Response res;
        res.setStatus("200 OK");
        res.setHeader("Content-Type", "application/json; charset=UTF-8");
        res.setHeader("Content-Length", std::to_string(jsonContent.size()));
        res.setBody(jsonContent);
        return res;
    }
    else if (iequals(method, "DELETE"))
    {
        // 쿼리 스트링에서 filename 추출
        std::map<std::string, std::string> queryParams = request.getQueryParams();

        if (queryParams.find("filename") == queryParams.end())
        {
            // filename 파라미터 없음
            std::string responseBody = "filename 파라미터가 필요합니다.";
            return Response::createErrorResponse(400, server_config);
        }

        std::string filename = queryParams["filename"];
        // 파일 이름 검증
        std::string sanitized_filename = sanitizeFilename(filename);
        if (!isValidFilename(sanitized_filename))
        {
            std::string responseBody = "유효하지 않은 파일 이름입니다.";
            return Response::createErrorResponse(400, server_config);
        }

        // 파일 삭제 경로 생성
        std::string filePath = location_config.upload_directory + "/" + sanitized_filename;

        // 파일 존재 여부 확인
        if (access(filePath.c_str(), F_OK) == -1)
        {
            std::string responseBody = "파일이 존재하지 않습니다.";
            return Response::createErrorResponse(404, server_config);
        }

        // 파일 삭제
        if (remove(filePath.c_str()) != 0)
        {
            std::cerr << "파일 삭제 오류: " << filePath << std::endl;
            std::string responseBody = "파일 삭제에 실패했습니다.";
            return Response::createErrorResponse(500, server_config);
        }

        // 성공 응답 (JSON 형식)
        std::string responseBody = "{ \"message\": \"파일이 성공적으로 삭제되었습니다.\" }";
        Response res;
        res.setStatus("200 OK");
        res.setHeader("Content-Type", "application/json; charset=UTF-8");
        res.setHeader("Content-Length", std::to_string(responseBody.size()));
        res.setBody(responseBody);
        return res;
    }
    else
    {
        return Response::createErrorResponse(400, server_config);
    }
}
} // namespace ResponseHandlers