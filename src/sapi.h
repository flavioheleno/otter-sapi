#ifndef __OTTER_SAPI_H__
#define __OTTER_SAPI_H__

  #include <SAPI.h>

  typedef struct otterHeaderStruct {
    const char *name;
    size_t nameLen;
    const char *value;
    size_t valueLen;
  } otterHeaderStruct;

  typedef struct otterResponseStruct {
    unsigned int statusCode;
    char *statusText;
    otterHeaderStruct *headers;
    size_t headerCount;
    size_t contentLength;
    int bodyfd[2];
  } otterResponseStruct;

  extern sapi_module_struct otterSapi;
  extern otterResponseStruct otterResponse;

  extern void otterInfo(void);
  extern int otterInit(void);
  extern void otterShutdown(void);
  extern otterResponseStruct *otterExec(
    const char *documentRoot,
    const char *scriptFilename,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size
  );

#endif
