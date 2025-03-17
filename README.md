# Webserv

### Commit Rules 📜

---

- **Force push and push to main branch are prohibited**
- Create and push a sub-branch instead of the main branch
- Sub-branch should have a title and a comment
- Each commit have to be pushed with a brief comment of what you've written code
- Use **labeling and questions** for peer review

### Process 🧶

---

#### 1. Project Overview

This web server is written in C++ and leverages OS-specific asynchronous, event-driven I/O models (epoll on Linux or kqueue on macOS) to achieve high-performance network processing.

- Handle static and dynamic content requests according to the HTTP protocol
- Provide file management features such as upload, deletion, and listing
- Execute CGI scripts for dynamic content generation
- Manage server settings (port, root directory, error pages, max request body size, allowed methods, etc.) through a configuration file
- Support logging and debugging using ANSI color codes

#### 2. Project structure

```
├── cgi-bin
│   ├── index.pl
│   ├── index.py
│   └── index.sh
├── include
├── src
│   ├── Log.cpp
│   ├── Parsing
│   │   ├── ConfigurationCore.cpp
│   │   ├── ConfigurationParse.cpp
│   │   ├── HttpMultipartParser.cpp
│   │   ├── HttpParserUtils.cpp
│   │   └── HttpRequestParser.cpp
│   ├── Poller
│   │   ├── EpollPoller.cpp
│   │   └── KqueuePoller.cpp
│   ├── Request
│   │   └── Request.cpp
│   ├── Response
│   │   ├── CGIHandler.cpp
│   │   ├── Response.cpp
│   │   ├── ResponseHandlers.cpp
│   │   └── ResponseUtils.cpp
│   ├── Server
│   │   ├── ServerCore.cpp
│   │   ├── ServerEvents.cpp
│   │   ├── ServerMatchLocation.cpp
│   │   ├── ServerUtils.cpp
│   │   ├── ServerWrite.cpp
│   │   ├── ServerWriteHelper.cpp
│   │   └── SocketManager.cpp
│   ├── Utils.cpp
│   └── main.cpp
└── www
    └── html
        ├── delete.html
        ├── error_pages
        │   ├── 400.html
        │   ├── 404.html
        │   ├── 405.html
        │   ├── 413.html
        │   └── 500.html
        ├── index.html
        ├── new.html
        ├── query.html
        └── upload.html
```

#### 3. Asynchronous Event-Based I/O

##### 3.1 Polling Abstraction and Platform-Specific Implementations

- **Abstraction Layer**
  - A common Poller interface is defined so different event monitoring mechanisms (epoll, kqueue, etc.) can be used uniformly.
- **Platform-Specific Event Loops**
  - On Linux, epoll (epoll_create, epoll_ctl, epoll_wait) is used to monitor read/write events for multiple sockets.
  - On macOS/BSD systems, kqueue (kqueue, kevent) is employed.
- **Advantages**
  - Efficiently handles large numbers of concurrent connections with near O(1) or O(log n) event monitoring performance.
  - By operating all sockets in non-blocking mode, the server can process client I/O work more efficiently based on event notifications.

##### 3.2 Socket Management and Connection Handling

- **Socket Creation and Binding**
  - Creates sockets based on port/host (IP) settings and prepares the server socket with bind() and listen().
  - Throws exceptions and logs errors for easier debugging if problems occur.
- **Non-Blocking Mode**
  - Uses fcntl() to configure sockets as non-blocking, ensuring accept/read/write calls never block.
  - The event loop monitors when sockets become “readable” or “writable,” and data is processed accordingly.
- **Event Loop Connection Flow**
  - **New Connections**: Accepts client sockets via accept() and immediately registers them for read events.
  - **Read Events**: Receives client requests → parses HTTP requests → generates appropriate responses
  - **Write Events**: Continues sending remaining data via send(); once all data is sent, write monitoring is disabled

#### 4. HTTP Request Parsing and Response Generation

##### 4.1 Parsing HTTP Requests

- **Parsing Logic**
  - Extracts the request line (method, URL, protocol), headers (key-value pairs), and body (multipart/form data, regular form data, etc.) in stages.
  - Uses the Content-Length header to determine body length, and handles multipart/form-data boundaries if needed.
- **Multipart Handling**
  - For requests that include file uploads, it extracts filename, Content-Type, etc., temporarily storing body data in memory before writing it to disk.
  - Enhances security through filename sanitization, extension checks, and size limits.

##### 4.2 Response Generation and Routing

- **Response Object**
  - Stores status codes (e.g., 200 OK, 404 Not Found), headers, and the body, ultimately converting them into an HTTP-compliant string like “HTTP/1.1 200 OK\r\n…”.
  - Uses headers such as “Connection: keep-alive” to decide whether to keep the socket open after a request is completed.
- **Routing via LocationConfig**
  - Matches the request path against multiple location blocks (“/upload”, “/cgi-bin”, “/images/,” etc.) defined in the configuration file, selecting the one with the longest match.
  - Each location can specify allowed methods, root directory, upload settings, CGI options, etc.
- **Error Handling and Custom Error Pages**
  - Uses predefined error pages from the server configuration; if unavailable, returns a default error message.
  - Common statuses like 404 Not Found, 400 Bad Request, etc., are handled via separate routines or error pages.

#### 5. CGI and Dynamic Content

##### 5.1 CGI Execution Method

- **Environment Variables**
  - Sets request data as CGI-standard environment variables (REQUEST_METHOD, CONTENT_LENGTH, CONTENT_TYPE, etc.).
- **Fork/Execve with Pipes**
  - Spawns a child process with fork(), then executes a Python, Bash, or Perl script via execve().
  - The server captures the script’s stdout through a pipe and assembles it into the response body.
- **Advantages**
  - More extensible than serving only static files, allowing easy integration of PHP, Python scripts, etc.
  - Each script runs in a separate process, enhancing overall server stability.

#### 6. File Upload/Deletion and Additional Features

##### 6.1 File Upload

- **Multipart Processing**
  - Identifies the boundary in “Content-Type: multipart/form-data” and separates each part (files/form fields).
  - Saves uploaded files in a designated directory, enforcing limits on file size and file extension checks.
- **Additional Capabilities**
  - Returns either JSON or HTML responses for uploads
  - Can provide a file listing in JSON if needed

##### 6.2 File Deletion

- **DELETE Method**
  - Offers an API to delete either a single file (specified via query parameters) or all files in a directory.
  - Returns errors if files are missing or if there are permission issues, logging them accordingly.

##### 6.3 Cookie/Session Handling

- **Cookie Support**
  - For example, reading a mode parameter from a route like /setmode, setting the mode in a Set-Cookie header.
  - Basic session logic serves as a sample implementation that can be expanded as necessary.

#### 7. Configuration File Parsing and Server Setup

##### 7.1 Structure of the Configuration File

- **server { … }**
  - Global settings such as port, server name, root directory, error pages, and client_max_body_size.
- **location { … }**
  - Allowed methods (GET, POST, etc.), cgi_extension, cgi_path, upload directories, and redirection settings.
  - Used for path-based routing of incoming requests.

##### 7.2 Parsing Implementation

- **Step-by-Step Parsing**
  - Reads the file line-by-line, detecting server blocks and location blocks. It then populates the respective ServerConfig or LocationConfig structures based on directives (e.g., “listen,” “root,” “error_page,” etc.).
- **Default Values**
  - If unspecified, defaults like a 1 MB limit for request bodies and certain allowed file extensions are applied automatically.
- **Logging**
  - After parsing, the server and location configurations can be logged in detail, especially when in debug mode, for easy verification.

#### 8. Utility Functions and Logging

##### 8.1 Utilities

- **Path Normalization**
  - Handles slashes (/), dots (.), and double-dot (..) sequences to produce safe file paths.
- **Determining MIME Types**
  - Maps file extensions to appropriate Content-Type values (text/html, image/png, text/css, etc.).
- **URL Decoding**
  - Decodes %xx-encoded strings, converts + to spaces, trims whitespace, and more.

##### 8.2 Logging System

- **Colored Logging**
  - Uses ANSI escape codes to distinguish standard messages, errors, internal errors, and success statuses clearly.
- **Log File Management**
  - Separates normal logs, error logs, and internal error logs into different files for better debugging.
  - Different directories may be used for development vs. production modes.

#### 9. Server Execution and Shutdown

##### 9.1 Main Function Flow

1. **Check Configuration File Path**
   - Accepts the config file path as a command-line argument.
2. **Initialize the Server**
   - Parses the configuration file, creates sockets, and registers them with the event poller.
3. **Start the Event Loop**
   - Runs an infinite loop that waits for events (epoll/kqueue) until a termination signal (SIGINT, SIGTERM) is received.
4. **Cleanup and Resource Release**
   - Closes all sockets and frees dynamically allocated objects before exiting.

##### 9.2 Safe Shutdown

- **Signal Handlers**
  - Captures signals like SIGINT (Ctrl+C) or SIGTERM, stops the server loop, and cleanly shuts down all connections before exiting.

#### 10. Notable and Advanced Techniques and Bonus Parts

1. **Separation of Platform Dependencies**
   - Encapsulates the epoll and kqueue differences behind a Poller abstraction so that higher-level logic remains the same regardless of underlying OS-specific APIs.
2. **Multipart Parsing and File Upload Security**
   - Goes beyond simple text processing, parsing multipart file uploads and validating file size and extensions to minimize attack surface.
3. **Process-Based CGI Implementation**
   - Improves safety and flexibility by running each CGI script in a separate process rather than in the server’s memory space.
   - Designed for extensibility to handle multiple scripting languages (Python, Bash, Perl).
4. **Flexible Configuration File Structure**
   - Supports multiple server blocks, each containing multiple location blocks, allowing for highly adaptable setups in real-world deployment.
5. **Robust Logging and Debug Modes**
   - Color-coded output and file-based logs provide fast problem resolution.
   - Internal errors are logged without halting the server, ensuring continued service operation whenever possible.

### Version

- [C++ 98](https://cplusplus.com/doc/oldtutorial/)
