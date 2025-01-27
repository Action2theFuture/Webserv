#include "Response.hpp"

namespace ResponseHelpers
{

bool isMethodAllowed(const std::string &method, const LocationConfig &location_config)
{
    std::string trimmed_method = trim(method);

    std::cout << "Path : " << location_config.path << std::endl;
    std::cout << "Request Method: [" << trimmed_method << "]" << std::endl;
    std::cout << "Allowed Methods: [";
    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        std::cout << location_config.methods[i];
        if (i != location_config.methods.size() - 1)
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    for (size_t i = 0; i < location_config.methods.size(); ++i)
    {
        if (iequals(trimmed_method, location_config.methods[i]))
        {
            std::cout << "Method [" << trimmed_method << "] is allowed." << std::endl;
            return true;
        }
    }
    std::cout << "Method [" << trimmed_method << "] is NOT allowed." << std::endl;
    return false;
}

bool createSingleDir(const std::string &path)
{
    // 이미 존재하면 true, 에러면 false
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0)
    {
        // 존재
        if (!S_ISDIR(sb.st_mode))
        {
            std::cerr << "Path exists but is not a directory: " << path << std::endl;
            return false;
        }
        // 디렉토리면 OK
        return true;
    }
    else
    {
        // 존재 안함 -> mkdir 시도
        if (mkdir(path.c_str(), 0755) < 0)
        {
            std::cerr << "mkdir() failed for " << path << ": " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
}

bool ensureDirectoryExists(const std::string &fullPath)
{
    if (fullPath.empty())
        return false;

    // 구분자 '/' 기준으로 토큰화 (유닉스 경로 가정)
    std::vector<std::string> tokens;
    {
        std::string::size_type start = 0, pos;
        while ((pos = fullPath.find('/', start)) != std::string::npos)
        {
            if (pos > start)
                tokens.push_back(fullPath.substr(start, pos - start));
            start = pos + 1;
        }
        // 마지막 토큰
        if (start < fullPath.size())
            tokens.push_back(fullPath.substr(start));
    }

    // '.' 혹은 '' 처리가 필요할 수도 있음 (상대 경로)
    // 예: "./uploads" -> 첫 토큰이 '.'이 될 수도
    // 여기서는 간단히 무시 처리
    std::string pathSoFar;
    if (fullPath[0] == '/')
    {
        // 절대 경로면 시작 '/'
        pathSoFar = "/";
    }

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (!pathSoFar.empty() && pathSoFar[pathSoFar.size() - 1] != '/')
            pathSoFar += "/";
        pathSoFar += tokens[i];

        if (!createSingleDir(pathSoFar))
        {
            return false;
        }
    }
    return true;
}

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
        return createErrorResponse(500, server_config);
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
        return createErrorResponse(404, server_config);
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
        return createErrorResponse(500, server_config);
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

// What To do (1 / 23)
// To ensure that the path is only called with the get method when fetching and executing a static file, you can use
// the
std::string buildRequestedPath(const std::string &path, const LocationConfig &location_config,
                               const ServerConfig &server_config)
{
    std::string requested_path;

    if (path == "/")
    {
        // root + index
        if (!location_config.root.empty())
        {
            if (location_config.root.back() == '/')
                requested_path = location_config.root + location_config.index;
            else
                requested_path = location_config.root + "/" + location_config.index;
        }
        else if (!server_config.root.empty())
        {
            if (server_config.root.back() == '/')
                requested_path = server_config.root + location_config.index;
            else
                requested_path = server_config.root + "/" + location_config.index;
        }
        else
        {
            requested_path = location_config.index;
        }
    }
    else
    {
        // root + path
        std::cerr << "path : " << path << std::endl;
        // Check if the requested path exactly matches the location path
        if (path == location_config.path && !location_config.index.empty())
        {
            // root + index (for exact location match)
            if (!location_config.root.empty())
            {
                if (location_config.root.back() == '/')
                    requested_path = location_config.root + location_config.index;
                else
                    requested_path = location_config.root + "/" + location_config.index;
            }
            else if (!server_config.root.empty())
            {
                if (server_config.root.back() == '/')
                    requested_path = server_config.root + location_config.index;
                else
                    requested_path = server_config.root + "/" + location_config.index;
            }
            else
            {
                requested_path = location_config.index;
            }
        }
        else
        {
            // root + path
            if (!location_config.root.empty())
            {
                if (location_config.root.back() == '/')
                    requested_path = location_config.root + path.substr(location_config.path.length());
                else
                    requested_path = location_config.root + "/" + path.substr(location_config.path.length());
            }
            else if (!server_config.root.empty())
            {
                if (server_config.root.back() == '/')
                    requested_path = server_config.root + path;
                else
                    requested_path = server_config.root + "/" + path;
            }
            else
            {
                requested_path = path;
            }
        }
    }

    // 경로 정규화 (필요 시)
    requested_path = normalizePath(requested_path);
    std::cerr << "requeted path : " << requested_path << std::endl;

    return requested_path;
}

Response handleUpload(const std::string &real_path, const Request &request, const LocationConfig &location_config,
                      const ServerConfig &server_config)
{
    Response res;

    // 파일 업로드 파싱
    const std::vector<UploadedFile> &files = request.getUploadedFiles();
    const std::map<std::string, std::string> &form_fields = request.getFormFields();

    if (files.empty())
    {
        std::cerr << "uploaded files are empty" << std::endl;
        res = createErrorResponse(400, server_config); // Bad Request
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
        return createErrorResponse(500, server_config); // Internal Server Error
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
            return createErrorResponse(400, server_config); // Bad Request
        }

        std::string file_path = upload_dir + "/" + sanitized_filename;

        // 파일 저장
        std::ofstream ofs(file_path.c_str(), std::ios::binary);
        if (!ofs.is_open())
        {
            std::cerr << "Failed to open file for writing: " << file_path << std::endl;
            return createErrorResponse(500, server_config); // Internal Server Error
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

} // namespace ResponseHelpers