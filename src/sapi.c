#include "defines.h"
#include "sapi.h"
#include "module.h"

#include <php.h>
#include <php_main.h>
#include <php_variables.h>
#include <signal.h>
#include "ext/standard/php_standard.h"
#include <fcntl.h>

typedef struct requestContext {} requestContext;

static const char HARDCODED_INI[] =
  "html_errors=0\n"
  "register_argc_argv=0\n"
  "implicit_flush=1\n"
  "output_buffering=0\n"
  "max_execution_time=0\n"
  "max_input_time=-1\n\0";

typedef struct otterRequestStruct {
  const char *documentRoot;
  const char *scriptFilename;
  const char *url;
  const char *method;
  const char *version;
} otterRequestStruct;

void otterInfo(void) {
  if (php_request_startup() == FAILURE) {
    return;
  }

  //zend_is_auto_global(ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_SERVER));
  php_print_info(0xFFFFFFFF);
  php_request_shutdown(NULL);
}

int otterInit(void) {
DEBUG("otterInit");
  /**
   * MHD does not install a signal handler for SIGPIPE. On platforms where this is possible (such as GNU/Linux), it
   * disables SIGPIPE for its I/O operations (by passing MSG_NOSIGNAL or similar). On other platforms, SIGPIPE signals
   * may be generated from network operations by MHD and will cause the process to die unless the developer explicitly
   * installs a signal handler for SIGPIPE. Hence portable code using MHD must install a SIGPIPE handler or explicitly
   * block the SIGPIPE signal. MHD does not do so in order to avoid messing with other parts of the application that
   * may need to handle SIGPIPE in a particular way.
   */
  #if defined(SIGPIPE) && defined(SIG_IGN)
    signal(SIGPIPE, SIG_IGN);
  #endif

  #ifdef ZTS
    php_tsrm_startup();
  #endif

  zend_signal_startup();

  sapi_startup(&otterSapi);

  otterSapi.ini_entries = HARDCODED_INI;
  otterSapi.additional_functions = NULL;

  otterSapi.phpinfo_as_text = 1;
  otterSapi.php_ini_path_override = NULL;
  otterSapi.php_ini_ignore_cwd = 1;

  #ifdef ZTS
    SG(request_info).path_translated = NULL;
  #endif

  // Module initialization (MINIT)
  if (otterSapi.startup(&otterSapi) == FAILURE) {
    DEBUG("Failed to initialize module");
    #ifdef ZTS
      tsrm_shutdown();
    #endif

    return 1;
  }

  return 0;
}

void otterShutdown(void) {
DEBUG("otterShutdown");
  // Module shutdown (MSHUTDOWN)
  php_module_shutdown();

  // SAPI shutdown (SSHUTDOWN)
  sapi_shutdown();

  #ifdef ZTS
    tsrm_shutdown();
  #endif
}

otterResponseStruct *otterExec(
  const char *documentRoot,
  const char *scriptFilename,
  const char *url,
  const char *method,
  const char *version,
  const char *upload_data,
  size_t *upload_data_size
) {
DEBUG("otterExec");
  char *scriptPath;
  size_t scriptPathLen;

  #ifndef ZTS
    otterInit();
  #endif

  /* initialize the defaults */
  SG(request_info).path_translated = NULL;
  SG(request_info).request_method = method;
  SG(request_info).proto_num = 1000;
  SG(request_info).query_string = NULL;
  SG(request_info).request_uri = url;
  SG(request_info).content_type = NULL;
  SG(request_info).content_length = *upload_data_size;
  SG(sapi_headers).http_response_code = 200;

  // Request initialization (RINIT)
  if (php_request_startup() == FAILURE) {
    DEBUG("Failed to initialize PHP Request");

    return NULL;
  }

  zend_is_auto_global(ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_SERVER));
  //zend_is_auto_global(ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_ENV));
  //zend_is_auto_global(ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_REQUEST));

  // SG(server_context) = (otterRequestStruct *)malloc(sizeof(otterRequestStruct));
  // memset(SG(server_context), 0, sizeof(otterRequestStruct));

  scriptPathLen = strlen(documentRoot) + strlen(scriptFilename) + 2;
  scriptPath = (char *)malloc(scriptPathLen);
  if (scriptPath == NULL) {
    // failed to malloc
    return NULL;
  }

  memset(scriptPath, 0, scriptPathLen);
  strcpy(scriptPath, documentRoot);
  strcat(scriptPath, "/");
  strcat(scriptPath, scriptFilename);

  DEBUGF("documentRoot: %s", documentRoot);
  DEBUGF("scriptFilename: %s", scriptFilename);
  DEBUGF("scriptPath: %s", scriptPath);

  otterResponseStruct *otterResponse = (otterResponseStruct *)malloc(sizeof(otterResponseStruct));
  memset(otterResponse, 0, sizeof(otterResponseStruct));

  if (pipe(otterResponse->bodyfd) == -1) {
    free(otterResponse);
    DEBUG("Failed to open pipe");

    return NULL;
  }

  if (fcntl(otterResponse->bodyfd[1], F_SETFD, FD_CLOEXEC) == -1) {
    free(otterResponse);
    DEBUG("Failed to setup pipe");

    return NULL;
  }

  SG(server_context) = otterResponse;

  zend_file_handle routerScript;
  zend_stream_init_filename(&routerScript, scriptPath);

  routerScript.primary_script = 1;

  if (php_execute_script(&routerScript) == FAILURE) {
    DEBUG("Failed to execute PHP script");
  }

  zend_destroy_file_handle(&routerScript);

  otterResponse->statusCode = (unsigned int)SG(sapi_headers).http_response_code;

  // Request shutdown (RSHUTDOWN)
  php_request_shutdown(NULL);

  #ifndef ZTS
    otterShutdown();
  #endif

  return otterResponse;
}

// Module initialization (MINIT)
static int otterStartup(sapi_module_struct *sapiModule) {
DEBUG("otterStartup");
  return php_module_startup(sapiModule, NULL);
}

static int otterActivate(void) {
DEBUG("otterActivate");
  return SUCCESS;
}

static int otterDeactivate(void) {
DEBUG("otterDeactivate");
  return SUCCESS;
}

static size_t otterUnbufferedWrite(const char *str, size_t len) {
//DEBUGF("otterUnbufferedWrite (%s)", str);
  otterResponseStruct *otterResponse = (otterResponseStruct *)SG(server_context);

  size_t wln = write(otterResponse->bodyfd[1], str, len);

  otterResponse->contentLength += wln;

  return wln;
}

static void otterSapiFlush(void *context) {
//DEBUG("otterSapiFlush");
  if (SG(headers_sent) == 0) {
    sapi_send_headers();
    SG(headers_sent) = 1;
  }
}

static int otterSendHeaders(sapi_headers_struct *headers) {
DEBUG("otterSendHeaders");
  if (SG(request_info).no_headers == 1) {
    return  SAPI_HEADER_SENT_SUCCESSFULLY;
  }

  otterResponseStruct *otterResponse = (otterResponseStruct *)SG(server_context);


  return SAPI_HEADER_SENT_SUCCESSFULLY;
  // SAPI_HEADER_SEND_FAILED
}

static void otterSendHeader(sapi_header_struct *header, void *context) {
  // otterResponseStruct *otterResponse = SG(context);

}

static size_t otterReadPost(char *buffer, size_t bytes) {
DEBUG("otterReadPost");
  return 0;
}

static char *otterReadCookies(void) {
DEBUG("otterReadCookies");
  return NULL;
}

static void otterRegisterVariables(zval *vars) {
DEBUG("otterRegisterVariables");

  // php_register_variable(vars, "DOCUMENT_ROOT", strlen("DOCUMENT_ROOT"), documentRoot, strlen(documentRoot))
  // php_register_variable(vars, "REQUEST_URI", strlen("REQUEST_URI"), scriptFilename, strlen(scriptFilename))
  // php_register_variable(vars, "REQUEST_METHOD", strlen("REQUEST_METHOD"), scriptFilename, strlen(scriptFilename))

  /*
  char *phpSelf = SG(request_info).request_uri ? SG(request_info).request_uri : "";
  size_t phpSelfLen = strlen(phpSelf);
  if (otterSapi.input_filter(PARSE_SERVER, "PHP_SELF", &phpSelf, phpSelfLen, &phpSelfLen)) {
    php_register_variable_safe("PHP_SELF", phpSelf, phpSelfLen, vars);
  }*/

  php_import_environment_variables(vars);
}

static void otterLogMessage(const char *message, int type) {
  DEBUG("otterLogMessage");
  DEBUG(message);
  // fprintf(stderr, "%s\n", message);
}

sapi_module_struct otterSapi = {
  "otter",                     /* name */
  "Otter",                     /* pretty name */
  otterStartup,                /* startup */
  php_module_shutdown_wrapper, /* shutdown */
  otterActivate,               /* activate */
  otterDeactivate,             /* deactivate */
  otterUnbufferedWrite,        /* unbuffered write */
  otterSapiFlush,              /* flush */
  NULL,                        /* get uid */
  NULL,                        /* getenv */
  php_error,                   /* error handler */
  NULL,                        /* header handler */
  otterSendHeaders,            /* send headers handler */
  otterSendHeader,             /* send header handler */
  otterReadPost,               /* read POST data */
  otterReadCookies,            /* read Cookies */
  otterRegisterVariables,      /* register server variables */
  otterLogMessage,             /* Log message */
  NULL,                        /* Get request time */
  NULL,                        /* Child terminate */
  STANDARD_SAPI_MODULE_PROPERTIES
};
