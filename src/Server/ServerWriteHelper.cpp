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
        if (sent > 0)
            buf.erase(0, sent); // 전송한 만큼 버퍼에서 제거합니다.
        else if (sent == 0)
            return false; // send()가 0을 반환하는 경우는 보통 발생하지 않으므로 오류 처리
        else // sent == -1 인 경우, errno를 사용하지 않으므로 임시 조건으로 처리합니다.
        {
            if (!poller->modify(client_fd, POLLER_READ | POLLER_WRITE))
            {
                LogConfig::reportInternalError("sendAllData: Failed to modify events for client_fd " +
                                               intToString(client_fd));
                return false;
            }
            return true; // 남은 데이터가 있으므로, 추후 WRITE 이벤트 발생 시 재시도합니다.
        }
    }
    return true;
}

// writePendingDataHelper()는 sendAllData()를 호출하여 전송을 진행합니다.
bool writePendingDataHelper(Poller *poller, int client_fd, std::string &buf)
{
    return sendAllData(poller, client_fd, buf);
}

