#include "account.h"
#include "crypt.h"
#include "file_io.h"
#include "oidc_array.h"

#include <syslog.h>

/** @fn struct oidc_account* addAccount(struct oidc_account* p, size_t* size, struct oidc_account account)   
 * @brief adds a account to an array 
 * @param p a pointer to the start of an array
 * @param size a pointer to the number of accounts in the array
 * @param account the account to be added. 
 * @return a pointer to the new array
 */
struct oidc_account* addAccount(struct oidc_account* p, size_t* size, struct oidc_account account) {
  void* tmp = realloc(p, sizeof(struct oidc_account) * (*size + 1));
  if(tmp==NULL) {
    syslog(LOG_AUTHPRIV|LOG_EMERG, "%s (%s:%d) realloc() failed: %m\n", __func__, __FILE__, __LINE__);
    oidc_errno = OIDC_EALLOC;
    return NULL;
  }
  p = tmp;
  memcpy(p + *size, &account, sizeof(struct oidc_account));
  (*size)++;
  // For some reason using the following function insted of the above same
  // statements doesn't work.
  // p= arr_addElement(p, size, sizeof(struct oidc_account), &account);    
  return p;
}

/** @fn int account_comparator(const void* v1, const void* v2)
 * @brief compares two accounts by their name. Can be used for sorting.
 * @param v1 pointer to the first element
 * @param v2 pointer to the second element
 * @return -1 if v1<v2; 1 if v1>v2; 0 if v1=v2
 */
int account_comparatorByName(const void *v1, const void *v2) {
  const struct oidc_account *p1 = (struct oidc_account *)v1;
  const struct oidc_account *p2 = (struct oidc_account *)v2;
  if(account_getName(*p1)==NULL && account_getName(*p2)==NULL) {
    return 0;
  }
  if(account_getName(*p1)==NULL) {
    return -1;
  }
  if(account_getName(*p2)==NULL) {
    return 1;
  }
  return strcmp(account_getName(*p1), account_getName(*p2));
}

/** @fn int account_comparator(const void* v1, const void* v2)
 * @brief compares two accounts by their name. Can be used for sorting.
 * @param v1 pointer to the first element
 * @param v2 pointer to the second element
 * @return -1 if v1<v2; 1 if v1>v2; 0 if v1=v2
 */
int account_comparatorByState(const void *v1, const void *v2) {
  const struct oidc_account *p1 = (struct oidc_account *)v1;
  const struct oidc_account *p2 = (struct oidc_account *)v2;
  if(account_getUsedState(*p1)==NULL && account_getUsedState(*p2)==NULL) {
    return 0;
  }
  if(account_getUsedState(*p1)==NULL) {
    return -1;
  }
  if(account_getUsedState(*p2)==NULL) {
    return 1;
  }
  return strcmp(account_getUsedState(*p1), account_getUsedState(*p2));
}

/** @fn void sortAccount()
 * @brief sorts accounts by their name using \f account_comparator 
 * @param p the array to be sorted
 * @param size the number of accounts in \p p
 * @return the sorted array
 */
struct oidc_account* sortAccount(struct oidc_account* p, size_t size) {
  return arr_sort(p, size, sizeof(struct oidc_account), account_comparatorByName);
}

/** @fn struct oidc_account* findAccount(struct oidc_account* p, size_t size, struct oidc_account key) 
 * @brief finds a account in an array.
 * @param p the array that should be searched
 * @param size the number of elements in arr
 * @param key the account to be found. 
 * @return a pointer to the found account. If no account could be found
 * NULL is returned.
 */
struct oidc_account* findAccountByName(struct oidc_account* p, size_t size, struct oidc_account key) {
  return arr_find(p, size, sizeof(struct oidc_account), &key, account_comparatorByName);
}

/** @fn struct oidc_account* findAccount(struct oidc_account* p, size_t size, struct oidc_account key) 
 * @brief finds a account in an array.
 * @param p the array that should be searched
 * @param size the number of elements in arr
 * @param key the account to be found. 
 * @return a pointer to the found account. If no account could be found
 * NULL is returned.
 */
struct oidc_account* findAccountByState(struct oidc_account* p, size_t size, struct oidc_account key) {
  return arr_find(p, size, sizeof(struct oidc_account), &key, account_comparatorByState);
}

/** @fn struct oidc_account* removeAccount(struct oidc_account* p, size_t* size, struct oidc_account account)   
 * @brief removes a account from an array 
 * @param p a pointer to the start of an array
 * @param size a pointer to the number of accounts in the array
 * @param account the account to be removed. 
 * @return a pointer to the new array
 */
struct oidc_account* removeAccount(struct oidc_account* p, size_t* size, struct oidc_account key) {
  void* pos = findAccountByName(p, *size,  key);
  if(NULL==pos) {
    return p;
  }
  freeAccountContent(pos);
  memmove(pos, p + *size - 1, sizeof(struct oidc_account));
  (*size)--;
  void* tmp = realloc(p, sizeof(struct oidc_account) * (*size));
  if(tmp==NULL && *size > 0) {
    syslog(LOG_AUTHPRIV|LOG_EMERG, "%s (%s:%d) realloc() failed: %m\n", __func__, __FILE__, __LINE__);
    oidc_errno = OIDC_EALLOC;
    return NULL;
  }
  p = tmp;
  return p;

}

/** @fn struct oidc_account* getAccountFromJSON(char* json)
 * @brief parses a json encoded account
 * @param json the json string
 * @return a pointer a the oidc_account. Has to be freed after usage. On
 * failure NULL is returned.
 */
struct oidc_account* getAccountFromJSON(char* json) {
  if(NULL==json) {
    oidc_setArgNullFuncError(__func__);
    return NULL;
  }
  struct oidc_account* p = calloc(sizeof(struct oidc_account), 1);
  struct key_value pairs[12];
  pairs[0].key = "issuer_url"; pairs[0].value = NULL;
  pairs[1].key = "issuer"; pairs[1].value = NULL;
  pairs[2].key = "name"; pairs[2].value = NULL;
  pairs[3].key = "client_id"; pairs[3].value = NULL;
  pairs[4].key = "client_secret"; pairs[4].value = NULL;
  pairs[5].key = "username"; pairs[5].value = NULL;
  pairs[6].key = "password"; pairs[6].value = NULL;
  pairs[7].key = "refresh_token"; pairs[7].value = NULL;
  pairs[8].key = "cert_path"; pairs[8].value = NULL;
  pairs[9].key = "redirect_uris"; pairs[9].value = NULL;
  pairs[10].key = "scope"; pairs[10].value = NULL;
  pairs[11].key = "device_authorization_endpoint"; pairs[11].value = NULL;
  if(getJSONValues(json, pairs, sizeof(pairs)/sizeof(*pairs))>0) {
    struct oidc_issuer* iss = calloc(sizeof(struct oidc_issuer), 1);
    if(pairs[0].value) {
      issuer_setIssuerUrl(iss, pairs[0].value);
      clearFreeString(pairs[1].value);
    } else {
      issuer_setIssuerUrl(iss, pairs[1].value);
    }
    issuer_setDeviceAuthorizationEndpoint(iss, pairs[11].value);
    account_setIssuer(p, iss);
    account_setName(p, pairs[2].value);
    account_setClientId(p, pairs[3].value);
    account_setClientSecret(p, pairs[4].value);
    account_setUsername(p, pairs[5].value);
    account_setPassword(p, pairs[6].value);
    account_setRefreshToken(p, pairs[7].value);
    account_setCertPath(p, pairs[8].value);
    account_setScope(p, pairs[10].value);
    int redirect_uri_count = JSONArrrayToArray(pairs[9].value, NULL); 
    if(redirect_uri_count>0){
      char** redirect_uri = calloc(sizeof(char*), redirect_uri_count);
      JSONArrrayToArray(pairs[9].value, redirect_uri);
      account_setRedirectUris(p, redirect_uri, redirect_uri_count);
    }
    clearFreeString(pairs[9].value);
    return p;
  } 
  freeAccount(p);
  return NULL;
}

/** @fn char* accountToJSON(struct oidc_rovider p)
 * @brief converts a account into a json string
 * @param p the oidc_account to be converted
 * @return a poitner to a json string representing the account. Has to be freed
 * after usage.
 */
char* accountToJSON(struct oidc_account p) {
  char* fmt = "{\n\"name\":\"%s\",\n\"issuer_url\":\"%s\",\n\"device_authorization_endpoint\":\"%s\",\n\"client_id\":\"%s\",\n\"client_secret\":\"%s\",\n\"refresh_token\":\"%s\",\n\"cert_path\":\"%s\",\n\"username\":\"%s\",\n\"password\":\"%s\",\n\"redirect_uris\":%s,\n\"scope\":\"%s\"\n}";
  char* redirect_uris = calloc(sizeof(char), 2+1);
  strcpy(redirect_uris, "[]");;
  unsigned int i;
  for(i=0; i<account_getRedirectUrisCount(p); i++) {
    redirect_uris = json_arrAdd(redirect_uris, account_getRedirectUris(p)[i]);
  }
  char* p_json = oidc_sprintf(fmt, 
      isValid(account_getName(p)) ? account_getName(p) : "", 
      isValid(account_getIssuerUrl(p)) ? account_getIssuerUrl(p) : "", 
      isValid(account_getDeviceAuthorizationEndpoint(p)) ? account_getDeviceAuthorizationEndpoint(p) : "", 
      isValid(account_getClientId(p)) ? account_getClientId(p) : "", 
      isValid(account_getClientSecret(p)) ? account_getClientSecret(p) : "", 
      isValid(account_getRefreshToken(p)) ? account_getRefreshToken(p) : "", 
      isValid(account_getCertPath(p)) ? account_getCertPath(p) : "",
      isValid(account_getUsername(p)) ? account_getUsername(p) : "", 
      isValid(account_getPassword(p)) ? account_getPassword(p) : "",
      redirect_uris,
      isValid(account_getScope(p)) ? account_getScope(p) : ""
      );
  clearFreeString(redirect_uris);
  return p_json;
}

/** void freeAccount(struct oidc_account* p)
 * @brief frees a account completly including all fields.
 * @param p a pointer to the account to be freed
 */
void freeAccount(struct oidc_account* p) {
  if(p==NULL) {
    return;
  }
  freeAccountContent(p);
  clearFree(p, sizeof(*p));
}

/** void freeAccountContent(struct oidc_account* p)
 * @brief frees a all fields of a account. Does not free the pointer it self
 * @param p a pointer to the account to be freed
 */
void freeAccountContent(struct oidc_account* p) {
  if(p==NULL) {
    return;
  }
  account_setName(p, NULL);
  account_setIssuer(p, NULL);
  account_setClientId(p, NULL);
  account_setClientSecret(p, NULL);
  account_setScope(p, NULL);
  account_setUsername(p, NULL);
  account_setPassword(p, NULL);
  account_setRefreshToken(p, NULL);
  account_setAccessToken(p, NULL);
  account_setCertPath(p, NULL);
  account_setRedirectUris(p, NULL, 0);
  account_setUsedState(p, NULL);
}

/** int accountconfigExists(const char* accountname)
 * @brief checks if a configuration for a given account exists
 * @param accountname the short name that should be checked
 * @return 1 if the configuration exists, 0 if not
 */
int accountConfigExists(const char* accountname) {
  return oidcFileDoesExist(accountname);
}

/** @fn struct oidc_account* decryptAccount(const char* accountname, const char* password) 
 * @brief reads the encrypted configuration for a given short name and decrypts
 * the configuration.
 * @param accountname the short name of the account that should be decrypted
 * @param password the encryption password
 * @return a pointer to an oidc_account. Has to be freed after usage. Null on
 * failure.
 */
struct oidc_account* decryptAccount(const char* accountname, const char* password) {
  char* fileText = readOidcFile(accountname);
  struct oidc_account* p = decryptAccountText(fileText, password);
  clearFreeString(fileText);
  return p;
}

struct oidc_account* decryptAccountText(char* fileContent, const char* password) {
  if(fileContent==NULL || password ==NULL) {
    oidc_setArgNullFuncError(__func__);
    return NULL;
  }
  unsigned char* decrypted = decryptFileContent(fileContent, password); 
  if(NULL==decrypted) {
    return NULL;
  }
  struct oidc_account* p = getAccountFromJSON((char*)decrypted);
  clearFreeString((char*)decrypted);
  return p;
}

/** @fn char* getAccountNameList(struct oidc_account* p, size_t size) 
 * @brief gets the account short names from an array of accounts
 * @param p a pointer to the first account
 * @param size the nubmer of accounts
 * @return a pointer to a JSON Array String containing all the account names.
 * Has to be freed after usage.
 */
char* getAccountNameList(struct oidc_account* p, size_t size) {
  if(NULL==p) {
    oidc_setArgNullFuncError(__func__);
    return NULL;
  }
  char* accountList = calloc(sizeof(char), 2+1);
  strcpy(accountList, "[]");
  if(0==size) {
    return accountList;
  }
  unsigned int i;
  for(i=0; i<size; i++) {
    accountList = json_arrAdd(accountList, account_getName(*(p+i)));
  }
  return accountList;
}

int hasRedirectUris(struct oidc_account account) {
  char* str = arrToListString(account_getRedirectUris(account), account_getRedirectUrisCount(account), ' ', 1);
  int ret = str != NULL ? 1 : 0;
  clearFreeString(str);
  return ret;
}

char* defineUsableScopes(struct oidc_account account) {
  char* supported = oidc_strcopy(account_getScopesSupported(account));
  char* wanted = account_getScope(account);
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "supported scope is '%s'", supported);
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "wanted scope is '%s'", wanted);
    if(!isValid(supported)) {
      clearFreeString(supported);
    return oidc_strcopy(wanted); //Do not return wanted directly, because it will be used in a call to setScope which will free and then set it
  }
  if(!isValid(wanted)) {
      clearFreeString(supported);
    return NULL;
  }
  
  //Adding mandatory scopes (for oidc-agent) to supported scopes
  if(strstr(supported, "openid")==NULL) {
    char* tmp = oidc_strcat(supported, " openid");
    clearFreeString(supported);
    supported = tmp;
  }
  if(strstr(supported, "offline_access")==NULL &&strcmp(account_getIssuerUrl(account), "https://accounts.google.com/")!=0) { //don't add offline_access for google, because theay don't accept it
    char* tmp = oidc_strcat(supported, " offline_access");
    clearFreeString(supported);
    supported = tmp;
  }
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "supported scope is now '%s'", supported);

  if(strcmp("max", wanted)==0) {
    return supported;
  }
  list_t* list = delimitedStringToList(wanted, ' ');
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(list, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    if(strstr(supported, node->val)==NULL) {
      list_remove(list, node);
    }
  }
  clearFreeString(supported);
  char* usable = listToDelimitedString(list, ' ');

  list_iterator_destroy(it);
  list_destroy(list);
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "usable scope is '%s'", usable);
  return usable;
}
