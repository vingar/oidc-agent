#define _XOPEN_SOURCE
#include "httpserver.h"
#include "ipc.h"
#include "parse_oidp.h"
#include "oidc_utilities.h"

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <strings.h>
#include <signal.h>

const char* const HTML_SUCCESS =  
#include "static/success.html" 
;
const char* const HTML_NO_CODE =
#include "static/no_code.html"
;
const char* const HTML_WRONG_STATE = 
#include "static/wrong_state.html"
;
const char* const HTML_CODE_EXCHANGE_FAILED = 
#include "static/code_exchange_failed.html"
;
const char* const HTML_CODE_EXCHANGE_FAILED_WITH_ERROR = 
#include "static/code_exchange_failed_with_error.html"
;
const char* const HTML_ERROR =
#include "static/error.html"
;


char* communicateWithPath(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  static struct connection con;
  if(ipc_initWithPath(&con)!=OIDC_SUCCESS) { 
    return NULL; 
  }
  if(ipc_connect(con)<0) {
    return NULL;
  }
  ipc_vwrite(*(con.sock), fmt, args);
  char* response = ipc_read(*(con.sock));
  ipc_close(&con);
  if(NULL==response) {
    printError("An unexpected error occured. It seems that oidc-agent has stopped.\n%s\n", oidc_serror());
    syslog(LOG_AUTHPRIV|LOG_ERR, "An unexpected error occured. It seems that oidc-agent has stopped.\n%s\n", oidc_serror());
    exit(EXIT_FAILURE);
  }
  return response;
}

static int ahc_echo(void* cls,
    struct MHD_Connection * connection,
    const char * url,
    const char * method,
    const char * version,
    const char * upload_data __attribute__((unused)),
    size_t * upload_data_size,
    void ** ptr) {
  static int dummy;
  struct MHD_Response * response;
  int ret;
  char* res = NULL;

  syslog(LOG_AUTHPRIV|LOG_DEBUG, "HttpServer: New connection: %s %s %s", version, method, url);

  if (0 != strcmp(method, "GET")) {
    return MHD_NO; /* unexpected method */
  }
  if (&dummy != *ptr) {
    /* The first time only the headers are valid,
       do not respond in the first round... */
    *ptr = &dummy;
    return MHD_YES;
  }
  if (0 != *upload_data_size) {
    return MHD_NO; /* upload data in a GET!? */
  }
  *ptr = NULL; /* clear context pointer */
  const char* code = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "code"); 
  const char* state = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "state");
  if(code) {
    syslog(LOG_AUTHPRIV|LOG_DEBUG, "HttpServer: Code is %s", code);
    char** cr = (char**) cls;
    if(strcmp(cr[2], state)==0) {
      res = communicateWithPath(REQUEST_CODEEXCHANGE, cr[0], cr[1], code, state);
      char* oidcgen_call = oidc_sprintf(REQUEST_CODEEXCHANGE, cr[0], cr[1], code, state);
      if(res==NULL) {
        res = oidc_sprintf(HTML_CODE_EXCHANGE_FAILED, oidcgen_call);
        response = MHD_create_response_from_buffer (strlen(res), (void*) res, MHD_RESPMEM_MUST_FREE);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      } else { //res!=NULL
        syslog(LOG_AUTHPRIV|LOG_DEBUG, "Httpserver ipc response is: %s", res);
        char* error = parseForError(res);
        if(error) {
          res = oidc_sprintf(HTML_CODE_EXCHANGE_FAILED_WITH_ERROR, error, oidcgen_call);
          clearFreeString(error);
        } else {
          res = oidc_sprintf(HTML_SUCCESS, cr[2]);
        }
        response = MHD_create_response_from_buffer (strlen(res), (void*) res, MHD_RESPMEM_MUST_FREE); // Note that MHD just frees the data and does not use clearFree
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      }
      clearFreeString(cr[0]);
      clearFreeString(cr[1]);
      clearFreeString(cr[2]);
      clearFree(cr, sizeof(char*)*3);
      *ptr = "shutdown";
      clearFreeString(oidcgen_call);
    } else {
      response = MHD_create_response_from_buffer(strlen(HTML_WRONG_STATE), (void*) HTML_WRONG_STATE, MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    }
  } else {
    const char* error = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "error");
    const char* error_description = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "error_description");
    if(error) {
      char* err = combineError(error, error_description);
      char* res = oidc_sprintf(HTML_ERROR, err);
      clearFreeString(err);
      response = MHD_create_response_from_buffer(strlen(res), (void*) res, MHD_RESPMEM_MUST_FREE);
    } else {
      response = MHD_create_response_from_buffer(strlen(HTML_NO_CODE), (void*) HTML_NO_CODE, MHD_RESPMEM_PERSISTENT);
    }
    ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
  }
  MHD_destroy_response(response);
  return ret;
}

void stopHttpServer(struct MHD_Daemon** d_ptr) {
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "HttpServer: Stopping HttpServer");
  MHD_stop_daemon(*d_ptr);
  clearFree(d_ptr, sizeof(struct MHD_Daemon*));
}

/**
 * @param config a pointer to a json account config.
 * */
struct MHD_Daemon** startHttpServer(unsigned short port, char* config, char* state) {
  // MHD_set_panic_func(&panicCallback, NULL);
  struct MHD_Daemon** d_ptr = calloc(sizeof(struct MHD_Daemon*),1);
  char** cls = calloc(sizeof(char*), 3);
  cls[0] = oidc_sprintf("%s", config);
  cls[1] = portToUri(port);
  cls[2] = oidc_sprintf("%s", state);
  *d_ptr = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
      port,
      NULL,
      NULL,
      &ahc_echo,
      cls,
      // MHD_OPTION_NOTIFY_COMPLETED, &requestCompletedCallback, d_ptr,
      MHD_OPTION_END);
  if (*d_ptr == NULL) {
    syslog(LOG_AUTHPRIV|LOG_ERR, "Error starting the HttpServer");
    oidc_errno = OIDC_EHTTPD;
    clearFree(d_ptr, sizeof(struct MHD_Daemon*));
    clearFreeString(cls[0]);
    clearFreeString(cls[1]);
    clearFreeString(cls[2]);
    clearFree(cls, sizeof(char*)*3);
    return NULL;
  }
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "HttpServer: Started HttpServer");
  return d_ptr;
}


struct MHD_Daemon** d_ptr = NULL;
list_t* servers = NULL;

struct running_server {
  pid_t pid;
  char* state;
};

void http_sig_handler(int signo) {
  switch(signo) {
    case SIGTERM:
      sleep(5);
      stopHttpServer(d_ptr);
      break;
    default: 
      syslog(LOG_AUTHPRIV|LOG_EMERG, "HttpServer caught Signal %d", signo);
  }
  exit(signo);
}

void clearFreeRunningServer(struct running_server* s) {
  clearFreeString(s->state);
  clearFree(s, sizeof(struct running_server));
}

int matchRunningServer(char* state, struct running_server* s) {
  return strcmp(s->state, state) == 0 ? 1 : 0;
}

oidc_error_t fireHttpServer(unsigned short port, char* config, char* state) {
  pid_t pid = fork();
  if(pid == -1) {
    syslog(LOG_AUTHPRIV|LOG_ALERT, "fork %m");
    oidc_setErrnoError();
    return oidc_errno;
  }
  if(pid == 0) {
    d_ptr = startHttpServer(port, config, state);
    signal(SIGTERM, http_sig_handler);
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigsuspend(&sigset); 
    return OIDC_ENOPE;
  } else {
    if(servers == NULL) {
      servers = list_new();
      servers->free = (void(*) (void*)) &clearFreeRunningServer;
      servers->match = (int(*) (void*, void*)) &matchRunningServer;
    }
    struct running_server*  running_server= calloc(sizeof(struct running_server), 1);
    running_server->pid = pid;
    running_server->state = oidc_strcopy(state);
    list_rpush(servers, list_node_new(running_server));

    return OIDC_SUCCESS;
  }
}

void termHttpServer(char* state) {
  if(state==NULL) {
    return;
  }
  list_node_t* n = list_find(servers, state);  
  if(n==NULL) {
    return;
  }
  kill(((struct running_server*)n->val)->pid, SIGTERM);
  list_remove(servers, n);
}
