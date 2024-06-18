#ifndef __OTTER_SERVER_H__
#define __OTTER_SERVER_H__

  #include <microhttpd.h>

  typedef struct MHD_Daemon serverPtr;

  extern void serverInfo(void);
  extern serverPtr *serverInit(const char *basePath, int port);
  extern void serverShutdown(serverPtr *ptr);

#endif
