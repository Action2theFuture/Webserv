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

    const std::vector<UploadedFile> &files = request.getUploadedFiles();
    const std::map<std::string, std::string> &form_fields = request.getFormFields();

    if (!validateUploadedFiles(files))
    {
        std::cerr << "Uploaded files are empty" << std::endl;
        res = Response::createErrorResponse(400, server_config); // Bad Request
        return res;
    }

    std::string upload_dir;
    if (!getUploadDirectory(location_config, server_config, upload_dir))
    {
        return Response::createErrorResponse(500, server_config); // Internal Server Error
    }

    std::vector<std::string> allowed_extensions = location_config.allowed_extensions;

    std::string description;
    std::map<std::string, std::string>::const_iterator desc_it = form_fields.find("description");
    if (desc_it != form_fields.end())
    {
        description = desc_it->second;
    }

    std::vector<std::string> uploaded_filenames;

    for (std::vector<UploadedFile>::const_iterator it = files.begin(); it != files.end(); ++it)
    {
        std::string sanitized_filename;
        if (!saveUploadedFile(upload_dir, *it, sanitized_filename))
        {
            return Response::createErrorResponse(500, server_config); // Internal Server Error
        }

        if (!isFileExtensionAllowed(sanitized_filename, allowed_extensions))
        {
            std::string errorMsg = "Disallowed extensions: File Name: " + sanitized_filename;
            LogConfig::logError(errorMsg);
            return Response::createErrorResponse(400, server_config); // Bad Request
        }

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
        return handleGetFileList(location_config, server_config);
    }
    else if (iequals(method, "DELETE"))
    {
        return handleDeleteFile(request, location_config, server_config);
    }
    else
    {
        std::string errorMsg = "Unsupported HTTP method: " + method;
        LogConfig::logError(errorMsg);
        return Response::createErrorResponse(400, server_config);
    }
}

Response handleGetFileList(const LocationConfig &location_config, const ServerConfig &server_config)
{
    using namespace ResponseUtils;
    Response res;
    std::vector<std::string> files;

    if (!listUploadedFiles(location_config.upload_directory, files))
    {
        return Response::createErrorResponse(500, server_config);
    }

    // 파일 목록을 JSON으로 변환
    std::string jsonContent = generateFileListJSON(files);

    // 성공 응답 생성
    res.setStatus("200 OK");
    res.setHeader("Content-Type", "application/json; charset=UTF-8");
    res.setHeader("Content-Length", intToString(jsonContent.size()));
    res.setBody(jsonContent);
    return res;
}

Response handleDeleteFile(const Request &request, const LocationConfig &location_config,
                          const ServerConfig &server_config)
{
    using namespace ResponseUtils;
    Response res;

    // 쿼리 스트링에서 filename 추출
    std::map<std::string, std::string> queryParams = request.getQueryParams();

    if (queryParams.find("filename") == queryParams.end())
    {
        std::string errorMsg = "filename parameter in DELETE is required.";
        LogConfig::logError(errorMsg);
        return Response::createErrorResponse(400, server_config);
    }

    std::string filename = queryParams["filename"];
    // 파일 이름 검증
    std::string sanitized_filename = sanitizeFilename(filename);
    if (!isValidFilename(sanitized_filename))
    {
        std::string errorMsg = "Invalid filename: " + sanitized_filename;
        LogConfig::logError(errorMsg);
        return Response::createErrorResponse(400, server_config);
    }

    // 파일 삭제
    if (!deleteUploadedFile(location_config.upload_directory, sanitized_filename))
    {
        return Response::createErrorResponse(500, server_config); // Internal Server Error
    }

    // 성공 응답 (JSON 형식)
    std::string responseBody = "{ \"message\": \"File successfully deleted.\" }";
    res.setStatus("200 OK");
    res.setHeader("Content-Type", "application/json; charset=UTF-8");
    res.setHeader("Content-Length", intToString(responseBody.size()));
    res.setBody(responseBody);
    return res;
}

Response handleDeleteAllFiles(const LocationConfig &location_config, const ServerConfig &server_config)
{
    using namespace ResponseUtils;
    Response res;

    // 모든 파일 삭제
    if (!deleteAllUploadedFiles(location_config.upload_directory))
    {
        return Response::createErrorResponse(500, server_config); // Internal Server Error
    }

    // 성공 응답 (JSON 형식)
    std::string responseBody = "{ \"message\": \"All files successfully deleted.\" }";
    res.setStatus("200 OK");
    res.setHeader("Content-Type", "application/json; charset=UTF-8");
    res.setHeader("Content-Length", intToString(responseBody.size()));
    res.setBody(responseBody);
    return res;
}

Response handleMethodNotAllowed(const LocationConfig &location_config, const ServerConfig &server_config)
{
    Response res = Response::createErrorResponse(405, server_config);
    std::string allow_methods;
    for (size_t m = 0; m < location_config.methods.size(); ++m)
    {
        allow_methods += location_config.methods[m];
        if (m != location_config.methods.size() - 1)
        {
            allow_methods += ", ";
        }
    }
    res.setHeader("Allow", allow_methods);
    std::cout << "Method not allowed. Returning 405." << std::endl;
    return res;
}

} // namespace ResponseHandlers