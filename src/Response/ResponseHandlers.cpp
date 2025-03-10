#include "ResponseHandlers.hpp"
#include "CGIHandler.hpp"
#include "Log.hpp"
#include "Response.hpp"
#include "ResponseUtils.hpp" // ResponseUtil 클래스 포함
#include "Utils.hpp"
#include <cstring>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Response ResponseHandler::handleRedirection(const LocationConfig &location_config)
{
    LogConfig::reportSuccess(302, "Moved Permanently");

    Response res;
    res.setStatus("301 Moved Permanently");
    res.setHeader("Location", location_config.redirect);
    res.setHeader("Cache-Control", "max-age=20, public");
    std::string body = "<h1>301 Moved Permanently</h1>";
    res.setBody(body);
    std::stringstream ss;
    ss << body.size();
    res.setHeader("Content-Length", ss.str());
    res.setHeader("Content-Type", "text/html");
    return res;
}


Response ResponseHandler::handleCGI(const Request &request, const std::string &real_path,
                                    const ServerConfig &server_config)
{
    // Create a copy of real_path
    std::string real_path_copy = real_path;

    // Trim the path to exclude everything after the first slash after the last .
    size_t dot_pos = real_path_copy.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        size_t slash_pos = real_path_copy.find('/', dot_pos);
        if (slash_pos != std::string::npos)
        {
            real_path_copy = real_path_copy.substr(0, slash_pos);
        }
    }

    // Check if the resource exists
    struct stat buffer;
    if (stat(real_path_copy.c_str(), &buffer) != 0)
    {
        LogConfig::reportInternalError("Resource not available: " + std::string(strerror(errno)));
        return Response::createErrorResponse(404, server_config);
    }

    Response res;
    CGIHandler cgi_handler;
    std::string cgi_output, cgi_content_type;

    if (!cgi_handler.execute(request, real_path_copy, cgi_output, cgi_content_type))
        return Response::createErrorResponse(500, server_config);

    // Parse CGI output
    size_t pos = cgi_output.find("\r\n\r\n");
    
    if (pos != std::string::npos)
    {
        std::string headers_part = cgi_output.substr(0, pos);
        std::string body_part = cgi_output.substr(pos + 4);
        std::istringstream iss(headers_part);
        std::string line;
        while (std::getline(iss, line))
        {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.resize(line.size() - 1);
            size_t colon = line.find(':');

            if (colon != std::string::npos)
            {
                std::string key = trim(line.substr(0, colon));
                std::string value = trim(line.substr(colon + 1));
                res.setHeader(key, value);
            }
        }

        res.setStatus("200 SUCCESS");
        res.setBody(body_part);

        std::stringstream ss;
        ss << body_part.size();
        res.setHeader("Content-Length", ss.str());

        if (!cgi_content_type.empty())
            res.setHeader("Content-Type", cgi_content_type);

    }
    else
    {
        res.setStatus("200 OK");
        res.setBody(cgi_output);

        std::stringstream ss;
        ss << cgi_output.size();

        res.setHeader("Content-Length", ss.str());
        res.setHeader("Content-Type", "text/html");
    }

    LogConfig::reportSuccess(200, "SUCCESS");

    return res;

}

Response ResponseHandler::handleStaticFile(const std::string &real_path, const ServerConfig &server_config)
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
        file_content.append(buffer, bytes_read);
    close(fd);
    if (bytes_read == -1)
    {
        perror("read");
        return Response::createErrorResponse(500, server_config);
    }
    std::string content_type = getMimeType(real_path);
    res.setStatus("200 OK");
    res.setBody(file_content);
    std::stringstream ss;
    ss << file_content.size();
    res.setHeader("Content-Length", ss.str());
    res.setHeader("Content-Type", content_type);
    LogConfig::reportSuccess(200, "SUCCESS");
    return res;
}

Response ResponseHandler::handleUpload(const std::string &real_path, const Request &request,
                                       const LocationConfig &location_config, const ServerConfig &server_config)
{
    const std::vector<UploadedFile> &files = request.getUploadedFiles();
    const std::map<std::string, std::string> &form_fields = request.getFormFields();

    // Validate that there are uploaded files
    if (!ResponseUtil::validateUploadedFiles(files))
    {
        LogConfig::reportInternalError("Uploaded files are empty");
        return Response::createErrorResponse(400, server_config);
    }

    // Get the upload directory
    std::string upload_dir;
    if (!ResponseUtil::getUploadDirectory(location_config, server_config, upload_dir))
        return Response::createErrorResponse(500, server_config);

    // Extract description if provided
    std::string description;
    std::map<std::string, std::string>::const_iterator desc_it = form_fields.find("description");
    if (desc_it != form_fields.end())
    {
        description = desc_it->second;
    }

    // Allowed file extensions
    const std::vector<std::string> &allowed_extensions = location_config.allowed_extensions;
    std::vector<std::string> uploaded_filenames;

    // Process each uploaded file
    for (std::vector<UploadedFile>::const_iterator it = files.begin(); it != files.end(); ++it)
    {
        // Sanitize the filename
        std::string sanitized_filename = sanitizeFilename(it->filename);

        // Check if the file extension is allowed
        if (!ResponseUtil::isFileExtensionAllowed(sanitized_filename, allowed_extensions))
        {
            std::string errorMsg = "Disallowed extension: File Name: " + sanitized_filename;
            LogConfig::reportInternalError(errorMsg);
            return Response::createErrorResponse(400, server_config);
        }

        // Save the file if the extension is valid
        if (!ResponseUtil::saveUploadedFile(upload_dir, *it, sanitized_filename))
        {
            LogConfig::reportInternalError("Failed to save file: " + sanitized_filename);
            return Response::createErrorResponse(500, server_config);
        }

        // Add to the list of successfully uploaded filenames
        uploaded_filenames.push_back(sanitized_filename);
    }

    // Handle static file response (or other post-upload logic)
    return handleStaticFile(real_path, server_config);
}


Response ResponseHandler::handleFileList(const Request &request, const LocationConfig &location_config,
                                         const ServerConfig &server_config)
{
    std::string method = request.getMethod();
    if (iequals(method, "GET"))
        return handleGetFileList(location_config, server_config);
    else if (iequals(method, "DELETE"))
        return handleDeleteFile(request, location_config, server_config);
    else
    {
        LogConfig::reportInternalError("Unsupported HTTP method: " + method);
        return Response::createErrorResponse(400, server_config);
    }
}

Response ResponseHandler::handleDeleteFile(const Request &request, const LocationConfig &location_config,
                                           const ServerConfig &server_config)
{
    std::map<std::string, std::string> queryParams = request.getQueryParams();
    if (queryParams.find("filename") == queryParams.end())
    {
        LogConfig::reportInternalError("filename parameter in DELETE is required.");
        return Response::createErrorResponse(400, server_config);
    }
    std::string filename = queryParams["filename"];
    std::string sanitized_filename = sanitizeFilename(filename);
    if (!isValidFilename(sanitized_filename))
    {
        LogConfig::reportInternalError("Invalid filename: " + sanitized_filename);
        return Response::createErrorResponse(400, server_config);
    }
    if (!ResponseUtil::deleteUploadedFile(location_config.upload_directory, sanitized_filename))
        return Response::createErrorResponse(500, server_config);
    std::string responseBody = "{ \"message\": \"File successfully deleted.\" }";
    Response res;
    res.setStatus("200 OK");
    res.setHeader("Content-Type", "application/json; charset=UTF-8");
    res.setHeader("Content-Length", intToString(responseBody.size()));
    res.setBody(responseBody);
    return res;
}

Response ResponseHandler::handleDeleteAllFiles(const LocationConfig &location_config, const ServerConfig &server_config)
{
    if (!ResponseUtil::deleteAllUploadedFiles(location_config.upload_directory))
        return Response::createErrorResponse(500, server_config);
    std::string responseBody = "{ \"message\": \"All files successfully deleted.\" }";
    Response res;
    res.setStatus("200 OK");
    res.setHeader("Content-Type", "application/json; charset=UTF-8");
    res.setHeader("Content-Length", intToString(responseBody.size()));
    res.setBody(responseBody);
    return res;
}

Response ResponseHandler::handleGetFileList(const LocationConfig &location_config, const ServerConfig &server_config)
{
    std::vector<std::string> files;
    if (!ResponseUtil::listUploadedFiles(location_config.upload_directory, files))
        return Response::createErrorResponse(500, server_config);
    std::string jsonContent = ResponseUtil::generateFileListJSON(files);
    Response res;
    res.setStatus("200 OK");
    res.setHeader("Content-Type", "application/json; charset=UTF-8");
    res.setHeader("Content-Length", intToString(jsonContent.size()));
    res.setBody(jsonContent);
    return res;
}

Response ResponseHandler::handleMethodNotAllowed(const LocationConfig &location_config,
                                                 const ServerConfig &server_config)
{
    Response res = Response::createErrorResponse(405, server_config);
    std::string allow_methods;
    for (size_t m = 0; m < location_config.methods.size(); ++m)
    {
        allow_methods += location_config.methods[m];
        if (m != location_config.methods.size() - 1)
            allow_methods += ", ";
    }
    res.setHeader("Allow", allow_methods);
    LogConfig::reportError(405, "Method Not Allowed");
    return res;
}

// 추가: validateMethod
bool ResponseHandler::validateMethod(const Request &request, const LocationConfig &location_config)
{
    std::string method = request.getMethod();
    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        if (iequals(method, location_config.methods[i]))
            return true;
    }
    LogConfig::reportInternalError("Method not allowed: " + method);
    return false;
}

// 추가: isCGIRequest
bool ResponseHandler::isCGIRequest(const std::string &real_path, const LocationConfig &location_config)
{

    if (!location_config.cgi_extension.empty())
    {
        std::string ext = real_path.substr(real_path.find_last_of('.') + 1);

        for (std::vector<std::string>::const_iterator it = location_config.cgi_extension.begin(); it != location_config.cgi_extension.end(); ++it)
        {
            std::string cgi_ext = *it;
            if (cgi_ext[0] == '.') // Remove the dot if present
                cgi_ext = cgi_ext.substr(1);
            if (iequals(ext, cgi_ext))
                return true;
        }

    }
    return false;
}

Response ResponseHandler::handleCatQuery(const std::string &real_path, const Request &request, const ServerConfig &server_config)
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
        file_content.append(buffer, bytes_read);
    close(fd);
    if (bytes_read == -1)
    {
        perror("read");
        return Response::createErrorResponse(500, server_config);
    }

    // 쿼리 파라미터를 가져옵니다.
    // 여기서는 첫 번째 쿼리 파라미터만 사용한다고 가정합니다.
    std::map<std::string, std::string> params = request.getQueryParams();
    std::string key = "";
    std::string value = "";
    if (!params.empty())
    {
        key = params.begin()->first;
        value = params.begin()->second;
    }

    // 템플릿 파일 내 플레이스홀더 "{{key}}"와 "{{value}}"를 실제 값으로 치환합니다.
    size_t pos = file_content.find("{{key}}");
    while (pos != std::string::npos)
    {
        file_content.replace(pos, 7, key);
        pos = file_content.find("{{key}}", pos + key.size());
    }
    pos = file_content.find("{{value}}");
    while (pos != std::string::npos)
    {
        file_content.replace(pos, 9, value);
        pos = file_content.find("{{value}}", pos + value.size());
    }

    // MIME 타입 설정 (HTML)
    std::string content_type = "text/html; charset=UTF-8";
    res.setStatus("200 OK");
    res.setBody(file_content);

    std::stringstream ss;
    ss << file_content.size();
    res.setHeader("Content-Length", ss.str());
    res.setHeader("Content-Type", content_type);

    LogConfig::reportSuccess(200, "SUCCESS");
    return res;
}
