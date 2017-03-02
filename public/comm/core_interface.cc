/*
 * core_interface.cpp
 *
 *  Created on: 2013-6-26
 *      Author: huaiyu
 */

#include "core_interface.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdarg.h>
#include <fcntl.h>
#include <netdb.h>
#include <cassert>
#include <errno.h>
#include "buffer.h"
#include <sys/un.h>
#include <sys/ioctl.h>
#include "log/mig_log.h"

#include <iostream>
using namespace std;

#define ArrLen(arr) (sizeof(arr)/sizeof(arr[0]))

static const int INVALID_SOCKT = -1;

namespace {

struct {
  size_t addr_len;
  union {
    sockaddr saddr;
    sockaddr_in saddr_in;
    sockaddr_un saddr_un;
  };
} _svr_sock_addr = { 0 };

int _svr_socket = INVALID_SOCKT;

fixed_buf<> recv_buffer(8192);

void _set_sock(int sock) {
  assert(INVALID_SOCKT != sock);

  int so_ka = 1;
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &so_ka, sizeof(so_ka));

  linger so_linger = { 0 };
  so_linger.l_onoff = 1;
  so_linger.l_linger = 3;
  setsockopt(sock, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
}

bool _try_connect() {
  if (INVALID_SOCKT == _svr_socket) {
    MIG_WARN(USER_LEVEL, "套接字未创建!");
    return false;
  }

  if (-1
      == connect(_svr_socket, &_svr_sock_addr.saddr, _svr_sock_addr.addr_len)) {
    close(_svr_socket);
    _svr_socket = INVALID_SOCKT;
    MIG_WARN(USER_LEVEL, "socket连接失败:%s %d", strerror(errno), errno);
    return false;
  }

  return true;
}

bool _try_send(int sock, const void *buff, size_t len, int flags,
               int &ret_code) {
  ret_code = 0;

  static const int RETRY_COUNT = 1;
  for (int i = 0; i < 1 + RETRY_COUNT; ++i) {
    int ret = send(sock, buff, len, flags);
    if (ret > 0) {
      return true;
    } else {
      if (ECONNRESET != errno && ENOTCONN != errno) {
        ret_code = errno;
        MIG_WARN(USER_LEVEL, "socket发送数据失败:%s", strerror(errno));
        return false;
      }

      if (!_try_connect()) {
        return false;
      }
    }
  }
  return true;
}
}

namespace net {

bool core_connect_net(const char *addr, unsigned short port) {
  assert(addr && addr[0]);

  struct hostent *host = gethostbyname(addr);
  if (NULL == host) {
    MIG_WARN(USER_LEVEL, "无效的网络地址");
    return false;
  }

  _svr_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (INVALID_SOCKT == _svr_socket) {
    MIG_WARN(USER_LEVEL, "socket创建失败");
    return false;
  }

  sockaddr_in &sai = _svr_sock_addr.saddr_in;
  _svr_sock_addr.addr_len = sizeof(sockaddr_in);
  sai.sin_family = AF_INET;
  sai.sin_port = htons(port);
  sai.sin_addr = *((struct in_addr *) host->h_addr);

  _set_sock(_svr_socket);

  return _try_connect();
}

bool core_connect_ipc(const char *path) {
  assert(path && path[0]);

  _svr_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (INVALID_SOCKT == _svr_socket) {
    MIG_WARN(USER_LEVEL, "socket创建失败");
    return false;
  }

  sockaddr_un &sau = _svr_sock_addr.saddr_un;
  _svr_sock_addr.addr_len = sizeof(sockaddr_un);
  sau.sun_family = AF_UNIX;
  strncpy(sau.sun_path, path, ArrLen(sau.sun_path));

  _set_sock(_svr_socket);

  return _try_connect();
}

bool core_get(int /*socket*/, const char *key, size_t key_len,
              std::string& value, int &flags, int &return_code) {
  assert(key && key[0]);

  value.clear();
  flags = 0;
  return_code = 0;
  int ret = 0;

  if (!_try_send(_svr_socket, key, key_len, 0, ret)) {
    return_code = ret;
    return false;
  }

  size_t wlen = 1024;
  recv_buffer.clear();
  do {
    void *wbuf = recv_buffer.wlock(wlen);
    ret = recv(_svr_socket, wbuf, wlen, 0);
    if (ret <= 0) {
      // 接收数据失败或远端已关闭
      MIG_WARN(USER_LEVEL, "socket接收数据失败:%s", strerror(errno));
      recv_buffer.wunlock(wbuf, 0);
      return false;
    }
    recv_buffer.wunlock(wbuf, ret);

    int nread = 0;
    ret = ioctl(_svr_socket, FIONREAD, &nread);
    if (0!=ret || 0==nread) {
      break;
    }
    wlen = nread;
  } while (1);

  value.assign(recv_buffer,recv_buffer.size());

  return true;
}

void core_close() {
  close(_svr_socket);
  _svr_socket = INVALID_SOCKT;

  MIG_WARN(USER_LEVEL, "关闭套接字");
}

}
