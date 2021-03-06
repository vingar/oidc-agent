#ifndef FLOW_HANDLER_H
#define FLOW_HANDLER_H

#include "account.h"
#include "oidc_error.h"

#include "../lib/list/src/list.h"

#include <time.h>

char* getAccessTokenUsingRefreshFlow(struct oidc_account* account, time_t min_valid_period, const char* scope) ;
oidc_error_t getAccessTokenUsingPasswordFlow(struct oidc_account* account) ;
oidc_error_t getAccessTokenUsingAuthCodeFlow(struct oidc_account* account, const char* code, const char* used_redirect_uri) ;
oidc_error_t getAccessTokenUsingDeviceFlow(struct oidc_account* account, const char* device_code) ;

list_t* parseFlow(const char* flow) ;
#endif //FLOW_HANDLER_H
