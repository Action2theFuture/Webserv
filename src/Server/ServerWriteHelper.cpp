#include "ServerWriteHelper.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include <cstring>
#include <errno.h>
#include <string>

static bool sendAllData(Poller *poller, int client_fd, std::string &buf)
{
    while (!buf.empty())
    {
        ssize_t sent = send(client_fd, buf.c_str(), buf.size(), 0);
        if (sent == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (!poller->modify(client_fd, POLLER_READ | POLLER_WRITE))
                {
                    LogConfig::reportInternalError("sendAllData: Failed to modify events for client_fd " +
                                                   intToString(client_fd));
                    return false;
                }
                return true;
            }
            else
            {
                LogConfig::reportInternalError("sendAllData: send() failed for client_fd " + intToString(client_fd) +
                                               ": " + std::string(strerror(errno)));
                return false;
            }
        }
        buf.erase(0, sent);
    }
    return true;
}

bool writePendingDataHelper(Poller *poller, int client_fd, std::string &buf)
{
    return sendAllData(poller, client_fd, buf);
}
