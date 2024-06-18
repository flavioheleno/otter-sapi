#include "defines.h"
#include "sapi.h"
#include "server.h"

#include <unistd.h>
#include <fcntl.h>

#define PAGE "<html><head><title>otter php sapi</title>"\
             "</head><body>default page</body></html>"

// typedef struct otterServerContext {
//   uintptr_t current_request;
//   uintptr_t main_request;
//   bool worker_ready;
//   char *cookie_data;
//   bool finished;
// } otterServerContext;

static void requestCompleted (
  void *cls,
  struct MHD_Connection *connection,
  void **req_cls,
  enum MHD_RequestTerminationCode toe
) {
  fprintf (stderr,
           "%" PRIu64 " - RC: %d\n",
           (uint64_t) __rdtsc (),
           toe);
}

static void connectionCompleted (
  void *cls,
  struct MHD_Connection *connection,
  void **socket_context,
  enum MHD_ConnectionNotificationCode toe
) {
  fprintf (stderr,
           "%" PRIu64 " - CC: %d\n",
           (uint64_t) __rdtsc (),
           toe);
}

static enum MHD_Result requestHandler(
  void *basePath,
  struct MHD_Connection *connection,
  const char *url,
  const char *method,
  const char *version,
  const char *upload_data,
  size_t *upload_data_size,
  void **ptr
) {
  static int aptr;
  struct MHD_Response *response;
  int ret;

  if (&aptr != *ptr) {
    DEBUGF("New %s request for %s (%s)", method, url, version);
    /* The first time only the headers are valid,
       do not respond in the first round... */
    *ptr = &aptr;

    return MHD_YES;
  }

  if (
    (
      strcmp(method, MHD_HTTP_METHOD_GET) != 0 ||
      strcmp(method, MHD_HTTP_METHOD_HEAD) != 0 ||
      strcmp(method, MHD_HTTP_METHOD_OPTIONS) != 0
    ) &&
    *upload_data_size > 0
  ) {
    return MHD_NO; /* upload data in a GET/HEAD/OPTIONS!? */
  }

  otterResponseStruct *otterResponse = otterExec(
    basePath,
    "site/index.php",
    url,
    method,
    version,
    upload_data,
    upload_data_size
  );

  if (otterResponse == NULL) {
    // return default error page
    return MHD_NO;
  }

  *ptr = NULL; /* clear context pointer */

  if (otterResponse->contentLength == 0) {
    response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
  } else {
    response = MHD_create_response_from_pipe(otterResponse->bodyfd[0]);
    //response = MHD_create_response_from_fd_at_offset64(otterResponse->contentLength, otterResponse->bodyfd[0], 0);
  }

  for (size_t i = 0; i < otterResponse->headerCount; i++) {
    MHD_add_response_header(response, otterResponse->headers[i].name, otterResponse->headers[i].value);
  }

  ret = MHD_queue_response(
    connection,
    otterResponse->statusCode,
    response
  );

  MHD_destroy_response(response);

  // otterResponse->bodyfd[0] will be closed by MHD_destroy_response
  close(otterResponse->bodyfd[1]);

  free(otterResponse);

  return ret;
}

void serverInfo(void) {
  otterInit();
  otterInfo();
  otterShutdown();
}

serverPtr *serverInit(const char *basePath, int port) {
  #ifdef ZTS
    DEBUG("Initializing SAPI..");
    otterInit();
    DEBUG("Done!");
  #endif

  DEBUG("Starting HTTP Server..");

  return MHD_start_daemon(
    MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_DEBUG,
    // MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_EPOLL | MHD_USE_TURBO | MHD_USE_TCP_FASTOPEN,
    port,
    NULL,
    NULL,
    &requestHandler,
    (void *)basePath,
    // MHD_OPTION_NOTIFY_COMPLETED, &requestCompleted, NULL,
    // MHD_OPTION_NOTIFY_CONNECTION, &connectionCompleted, NULL,
    MHD_OPTION_END
  );
}

void serverShutdown(serverPtr *ptr) {
  if (ptr == NULL) {
    DEBUG("Failed!");

    return;
  }

  DEBUG("Stopping HTTP Server..");
  MHD_stop_daemon(ptr);
  DEBUG("Done!");

  #ifdef ZTS
    DEBUG("Finalizing SAPI..");
    otterShutdown();
    DEBUG("Done!");
  #endif
}
