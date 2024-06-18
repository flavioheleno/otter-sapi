#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "server.h"

int main(int argc, const char *argv[]) {
  const char *basePath = getcwd(NULL, 0);
  if (basePath == NULL) {
    return 1;
  }

  if (argc >= 2 && strcmp(argv[1], "-i") == 0) {
    serverInfo();
    return 0;
  }

  serverPtr *s = serverInit(basePath, 8080);

  (void)getc(stdin);

  serverShutdown(s);

  free((void *)basePath);

  return 0;
}
