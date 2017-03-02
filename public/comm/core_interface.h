#ifndef _core_interface_
#define _core_interface_

#include <string>

namespace net {
bool core_connect_net(const char *addr, unsigned short port);

bool core_connect_ipc(const char *path);

bool core_get(int socket, const char *key, size_t key_len, std::string& value,
              int &flags, int &return_code);


void core_close();

}
#endif // _core_interface_
