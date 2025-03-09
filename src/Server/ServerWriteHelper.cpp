#include "ServerWriteHelper.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include <cstring>
#include <errno.h>
#include <string>

// 데이터를 모두 보내는 함수
static bool sendAllData(Poller *poller, int client_fd, std::string &buf)
{
    while (!buf.empty())
    {
        ssize_t sent = send(client_fd, buf.c_str(), buf.size(), 0);
        if (sent == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 더 이상 전송할 수 없는 상황이면, FD가 다시 쓰기 가능할 때까지
                // READ와 WRITE 이벤트를 모두 감시하도록 변경합니다.
                if (!poller->modify(client_fd, POLLER_READ | POLLER_WRITE))
                {
                    LogConfig::reportInternalError("sendAllData: Failed to modify events for client_fd " +
                                                   intToString(client_fd));
                    return false;
                }
                return true;  // 아직 전송할 데이터가 남아 있으므로, 추후 WRITE 이벤트에서 재시도
            }
            else
            {
                LogConfig::reportInternalError("sendAllData: send() failed for client_fd " + intToString(client_fd) +
                                               ": " + std::string(strerror(errno)));
                return false;
            }
        }
        // 일부 또는 전체 전송 성공 → 전송된 만큼 버퍼에서 제거
        buf.erase(0, sent);
    }
    return true;
}

// writePendingDataHelper()는 sendAllData()를 호출하여 전송을 진행합니다.
bool writePendingDataHelper(Poller *poller, int client_fd, std::string &buf)
{
    return sendAllData(poller, client_fd, buf);
}

