#ifndef SERVER_WRITE_HELPER_HPP
#define SERVER_WRITE_HELPER_HPP

#include "Poller.hpp"
#include <set>
#include <string>

bool writePendingDataHelper(Poller *poller, int client_fd, std::string &buf);

#endif // SERVER_WRITE_HELPER_HPP
