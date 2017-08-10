#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <jsmn.h>

#include "config.h"
#include "file_io.h"
#include "http.h"
#include "crypt.h"

#define CONFIGFILE "/.oidc/config.conf"
#define RUNNINGCONFIG_FILE "/.oidc/conf"

struct oidc_provider {
  char* issuer;
  char* name;                           // mandatory in config file
  char* username;                       // optional in config file
  char* encrypted_password_hex;         // retrieved
  unsigned long encrypted_password_len; // retrivied    // this is len of the cipher not of hex encoded cipher
  char salt_hex[SALT_LEN*2+1];          // generated
  char nonce_hex[NONCE_LEN*2+1];        // generated
  char* client_id;                      // mandatory in config file
  char* client_secret;                  // mandatory in config file
  char* configuration_endpoint;         // mandatory in config file
  char* token_endpoint;                 // retrieved from configuration_endpoint
  char* refresh_token;                  // optional in config file
  char* access_token;                   // retrieved from token endpoint
  unsigned long token_expires_at;       // retrieved from token endpoint
  unsigned long min_valid_period;       // mandatory in config file
};

static struct {
  char* cert_path;                      // mandatory in config file
  char* wattson_url;                    // optional in config file
  int encryption_mode;                  // optional in config file
  char* cwd;                            // determined
  unsigned int provider_count;          // determined
  struct oidc_provider provider[];
} config;



// getter
const char* conf_getCertPath() {return config.cert_path;}
const char* conf_getWattsonUrl() {return config.wattson_url;}
int conf_getEncryptionMode() {return config.encryption_mode;}
int conf_hasEncryptedPassword(unsigned int provider) {return isValid(config.provider[provider].encrypted_password_hex);}
const char* conf_getcwd() {return config.cwd;}
unsigned int conf_getProviderCount() {return config.provider_count;}
unsigned int conf_getProviderIndex(char* providername) {
  unsigned int i;
  if(providername!=NULL)
    for(i=0; i<conf_getProviderCount(); i++) {
      if(strcmp(providername, conf_getProviderName(i))==0)
        return i;
    }
  return -1;
}
char* conf_getProviderName(unsigned int provider) {return config.provider[provider].name;}
char* conf_getUsername(unsigned int provider) {return config.provider[provider].username;}
const char* conf_getClientId(unsigned int provider) {return config.provider[provider].client_id;}
const char* conf_getClientSecret(unsigned int provider) {return config.provider[provider].client_secret;}
const char* conf_getConfigurationEndpoint(unsigned int provider) {return config.provider[provider].configuration_endpoint;}
const char* conf_getTokenEndpoint(unsigned int provider) {return config.provider[provider].token_endpoint;}
unsigned long conf_getMinValidPeriod(unsigned int provider) {return config.provider[provider].min_valid_period;}
char* conf_getRefreshToken(unsigned int provider) {return config.provider[provider].refresh_token;}
char* conf_getAccessToken(unsigned int provider) {return config.provider[provider].access_token;}
unsigned long conf_getTokenExpiresAt(unsigned int provider) {return config.provider[provider].token_expires_at;}
/** @fn char* conf_getDecryptedPassword(unsigned int provider, const char* encryption_password)
 * @brief returns the decrypted password
 * @param provider the provider whose password should be returned
 * @param encryption_password the password used for encryption
 * @return the decrypted password
 */
char* conf_getDecryptedPassword(unsigned int provider, const char* encryption_password) {
  return (char*)decrypt(config.provider[provider].encrypted_password_hex, config.provider[provider].encrypted_password_len, encryption_password, config.provider[provider].nonce_hex, config.provider[provider].salt_hex);
}

void conf_setcwd(char* cwd) {
  free(config.cwd);
  config.cwd = cwd;
}
void savecwd() {
  char* cwd = getcwd(NULL,0);
  if(!isValid(cwd)) {
    syslog(LOG_AUTHPRIV|LOG_ALERT, "Could not get cwd: %m\n");
    free(cwd);
    exit(EXIT_FAILURE);
  } else {
    conf_setcwd(cwd);
  }
}
void conf_setCertPath(char* cert_path) {
  free(config.cert_path);
  config.cert_path = cert_path;
}
void conf_setWattsonUrl(char* wattson_url) {
  free(config.wattson_url);
  config.wattson_url = wattson_url;
}
void conf_setTokenExpiresAt(unsigned int provider, unsigned long expires_at) {
  config.provider[provider].token_expires_at=expires_at;
}
void conf_setName(unsigned int provider, char* name) {
  free(config.provider[provider].name);
  config.provider[provider].name=name;
}
void conf_setUsername(unsigned int provider, char* username) {
  free(config.provider[provider].username);
  config.provider[provider].username=username;
}
void conf_setClientId(unsigned int provider, char* client_id) {
  free(config.provider[provider].client_id);
  config.provider[provider].client_id=client_id;
}
void conf_setClientSecret(unsigned int provider, char* client_secret) {
  free(config.provider[provider].client_secret);
  config.provider[provider].client_secret=client_secret;
}
/** @fn void conf_encryptAndSetPassword(unsigned int provider, const char* password, const char* encryption_password)
 * @brief encrypts a plain \p password and sets the encrypted_password_hex
 * @param provider the provider whose password should be set
 * @param password the plain password
 * @param encryption_password the password to be used for encryption
 */
void conf_encryptAndSetPassword(unsigned int provider, const char* password, const char* encryption_password) {
  free(config.provider[provider].encrypted_password_hex);
  config.provider[provider].encrypted_password_len = MAC_LEN + strlen(password);
  config.provider[provider].encrypted_password_hex = encrypt((unsigned char*)password, encryption_password, config.provider[provider].nonce_hex, config.provider[provider].salt_hex);
}
void conf_setAccessToken(unsigned int provider, char* access_token) {
  free(config.provider[provider].access_token);
  config.provider[provider].access_token=access_token;
}
void conf_setRefreshToken(unsigned int provider, char* refresh_token) {
  free(config.provider[provider].refresh_token);
  config.provider[provider].refresh_token=refresh_token;
}
void conf_setTokenEndpoint(unsigned int provider, char* token_endpoint) {
  free(config.provider[provider].token_endpoint);
  config.provider[provider].token_endpoint=token_endpoint;
}
void conf_setConfigurationEndpoint(unsigned int provider, char* configuration_endpoint) {
  free(config.provider[provider].configuration_endpoint);
  config.provider[provider].configuration_endpoint=configuration_endpoint;
}


// void printConfig() {
//   char* jsonConfig = configToJSON();
//   fprintf(stdout, "%s\n", jsonConfig);
//   free(jsonConfig);
// }
//
// void logConfig() {
//   char* jsonConfig = configToJSON();
//   syslog(LOG_AUTHPRIV|LOG_DEBUG, "%s", jsonConfig);
//   free(jsonConfig);
// }
//
//
// void readConfig() {
//   char* config_cont = readFile(CONFIGFILE);
//   struct key_value pairs[4];
//   pairs[0].key = "cert_path"; pairs[0].value=NULL;
//   pairs[1].key = "wattson_url"; pairs[1].value=NULL;
//   pairs[2].key = "encryption_mode"; pairs[2].value=NULL;
//   pairs[3].key = "provider"; pairs[3].value=NULL;
//   if(getJSONValues(config_cont, pairs, sizeof(pairs)/sizeof(*pairs))<0) {
//     syslog(LOG_AUTHPRIV|LOG_ALERT, "Could not parse config file. Please fix the configuration.\n");
//     exit(EXIT_FAILURE);
//   }
//   free(config_cont);
//   if (!isValid(pairs[0].value)) {
//     syslog(LOG_AUTHPRIV|LOG_ALERT,"No cert_path found in config file '%s'.\n",CONFIGFILE);
//     free(pairs[0].value);
//     free(config_cont);
//     exit(EXIT_FAILURE);
//   } else {
//     conf_setCertPath(pairs[0].value);
//   }
//   if (!isValid(pairs[1].value)) {
//     syslog(LOG_AUTHPRIV|LOG_NOTICE,"No wattson_url found in config file '%s'.\n",CONFIGFILE);
//     free(pairs[1].value);
//   } else {
//     conf_setWattsonUrl(pairs[1].value);
//   }
//   if (pairs[2].value!=NULL) {
//     config.encryption_mode = atoi(pairs[2].value);
//     free(pairs[2].value);
//   }
//   readProviderConfig(pairs[3].value);
//   free(pairs[3].value);
//   logConfig();
// }

void readProviderConfig(char* provider) {
  // int r;
  // jsmn_parser p;
  // jsmn_init(&p);
  // int token_needed = jsmn_parse(&p, provider, strlen(provider), NULL, 0);
  // jsmntok_t t[token_needed]; 
  // jsmn_init(&p);
  // r = jsmn_parse(&p, provider, strlen(provider), t, sizeof(t)/sizeof(t[0]));
  //
  // if (!checkParseResult(r, t[0]))
  //   exit(EXIT_FAILURE);
  // config.provider_count = t[0].size;
  // int t_index = 2;
  // unsigned int i;
  // for(i=0;i<config.provider_count;i++){
  //   char* name =  malloc(t[t_index-1].end-t[t_index-1].start+1);
  //   sprintf(name,"%.*s", t[t_index-1].end-t[t_index-1].start,
  //       provider + t[t_index-1].start);
  //   if(isValid(name)) conf_setName(i, name); else free(name);
  //   char* username =  getValuefromTokens(&t[t_index], t[t_index].size*2, "username", provider);
  //   if (isValid(username)) conf_setUsername(i,username); else free(username);
  //   char* client_id =  getValuefromTokens(&t[t_index], t[t_index].size*2, "client_id", provider);
  //   if (isValid(client_id)) conf_setClientId(i,client_id); else free(client_id);
  //   char* client_secret =  getValuefromTokens(&t[t_index], t[t_index].size*2, "client_secret", provider);
  //   if (isValid(client_secret)) conf_setClientSecret(i,client_secret); else free(client_secret);
  //   char* configuration_endpoint =  getValuefromTokens(&t[t_index], t[t_index].size*2, "configuration_endpoint", provider);
  //   if (isValid(configuration_endpoint)) conf_setConfigurationEndpoint(i,configuration_endpoint); else free(configuration_endpoint);
  //   char* refresh_token = getValuefromTokens(&t[t_index], t[t_index].size*2, "refresh_token", provider);
  //   if (isValid(refresh_token)) conf_setRefreshToken(i, refresh_token); else free(refresh_token);
  //   char* pov = getValuefromTokens(&t[t_index], t[t_index].size*2, "min_valid_period", provider);
  //   if(!isValid(config.provider[i].client_id)) {
  //     syslog(LOG_AUTHPRIV|LOG_ALERT, "No client_id found for provider %s in config file '%s'.\n",config.provider[i].name, CONFIGFILE);
  //     exit(EXIT_FAILURE);
  //   }
  //   if(!isValid(config.provider[i].client_secret)) {
  //     syslog(LOG_AUTHPRIV|LOG_ALERT, "No client_secret found for provider %s in config file '%s'.\n",config.provider[i].name, CONFIGFILE);
  //     exit(EXIT_FAILURE);
  //   }
  //   if(!isValid(config.provider[i].configuration_endpoint)) {
  //     syslog(LOG_AUTHPRIV|LOG_ALERT, "No configuration_endpoint found for provider %s in config file '%s'.\n",config.provider[i].name, CONFIGFILE);
  //     exit(EXIT_FAILURE);
  //   }
  //   if(!isValid(pov)) {
  //     syslog(LOG_AUTHPRIV|LOG_ALERT, "No min_valid_period found for provider %s in config file '%s'.\n",config.provider[i].name, CONFIGFILE);
  //     exit(EXIT_FAILURE);
  //   }
  //   config.provider[i].min_valid_period = atoi(pov);
  //   free(pov);
  //   if (!isValid(config.provider[i].refresh_token)) 
  //     syslog(LOG_AUTHPRIV|LOG_NOTICE, "No refresh_token found for provider %s in config file '%s'.\n",config.provider[i].name, CONFIGFILE);
  //   if (!isValid(config.provider[i].username)) 
  //     syslog(LOG_AUTHPRIV|LOG_NOTICE, "No username found for provider %s in config file '%s'.\n",config.provider[i].name, CONFIGFILE);
  //   t_index += t[t_index].size*2+2;
  //   getOIDCProviderConfig(i);
  // }
}


// void saveConfig() {
//   char* jsonConfig = configToJSON();
//   writeToFile(RUNNINGCONFIG_FILE, jsonConfig);
//   free(jsonConfig);
// }

void readSavedConfig() {
  // char* config_cont = readFile(RUNNINGCONFIG_FILE);
  // if(config_cont==NULL) {
  //   syslog(LOG_AUTHPRIV|LOG_NOTICE, "Could not read saved config from file '%s'", RUNNINGCONFIG_FILE);
  //   return;
  // }
  // struct key_value pairs[6];
  // pairs[0].key = "cert_path"; pairs[0].value=NULL;
  // pairs[1].key = "wattson_url"; pairs[1].value=NULL;
  // pairs[2].key = "encryption_mode"; pairs[2].value=NULL;
  // pairs[3].key = "cwd"; pairs[3].value=NULL;
  // pairs[4].key = "provider_count"; pairs[4].value=NULL;
  // pairs[5].key = "provider"; pairs[5].value=NULL;
  // if(getJSONValues(config_cont, pairs, sizeof(pairs)/sizeof(*pairs))<0) {
  //   syslog(LOG_AUTHPRIV|LOG_ALERT, "Could not parse saved config file.\n");
  //   free(config_cont);
  //   return;
  // }
  // free(config_cont);
  // config.cert_path = pairs[0].value;
  // if(!isValid(config.cert_path)) {free(config.cert_path); config.cert_path=NULL;}
  // config.wattson_url = pairs[1].value;
  // if(!isValid(config.wattson_url)) {free(config.wattson_url); config.wattson_url=NULL;}
  // if(isValid(pairs[2].value)) 
  //   config.encryption_mode = atoi(pairs[2].value);
  // free(pairs[2].value);
  // config.cwd = pairs[3].value;
  // if(!isValid(config.cwd)) {free(config.cwd); config.cwd=NULL;}
  // if(isValid(pairs[4].value)) 
  //   config.provider_count = atoi(pairs[4].value);
  // free(pairs[4].value);
  //
  // char* provider = pairs[5].value;
  // int r;
  // jsmn_parser p;
  // jsmn_init(&p);
  // int token_needed = jsmn_parse(&p, provider, strlen(provider), NULL, 0);
  // jsmntok_t t[token_needed]; 
  // jsmn_init(&p);
  // r = jsmn_parse(&p, provider, strlen(provider), t, sizeof(t)/sizeof(t[0]));
  //
  // if (!checkParseResult(r, t[0]))
  //   return;
  // int t_index = 2;
  // unsigned int i;
  // for(i=0;i<config.provider_count;i++){
  //   config.provider[i].name = malloc(t[t_index-1].end-t[t_index-1].start+1);
  //   sprintf(config.provider[i].name,"%.*s", t[t_index-1].end-t[t_index-1].start,
  //       provider + t[t_index-1].start);
  //   config.provider[i].username = getValuefromTokens(&t[t_index], t[t_index].size*2, "username", provider);
  //   config.provider[i].encrypted_password_hex = getValuefromTokens(&t[t_index], t[t_index].size*2, "encrypted_password_hex", provider);
  //   config.provider[i].client_id = getValuefromTokens(&t[t_index], t[t_index].size*2, "client_id", provider);
  //   config.provider[i].client_secret = getValuefromTokens(&t[t_index], t[t_index].size*2, "client_secret", provider);
  //   config.provider[i].configuration_endpoint = getValuefromTokens(&t[t_index], t[t_index].size*2, "configuration_endpoint", provider);
  //   config.provider[i].token_endpoint = getValuefromTokens(&t[t_index], t[t_index].size*2, "token_endpoint", provider);
  //   config.provider[i].refresh_token = getValuefromTokens(&t[t_index], t[t_index].size*2, "refresh_token", provider);
  //   config.provider[i].access_token = getValuefromTokens(&t[t_index], t[t_index].size*2, "access_token", provider);
  //   char* pov = getValuefromTokens(&t[t_index], t[t_index].size*2, "min_valid_period", provider);
  //   char* exp_at = getValuefromTokens(&t[t_index], t[t_index].size*2, "token_expires_at", provider);
  //   char* salt_hex = getValuefromTokens(&t[t_index], t[t_index].size*2, "salt_hex", provider);
  //   char* nonce_hex = getValuefromTokens(&t[t_index], t[t_index].size*2, "nonce_hex", provider);
  //   char* len = getValuefromTokens(&t[t_index], t[t_index].size*2, "encrypted_password_len", provider);
  //   syslog(LOG_AUTHPRIV|LOG_DEBUG, "char* len is %s", len);
  //   if(isValid(pov))
  //     config.provider[i].min_valid_period = atoi(pov);
  //   free(pov);
  //   if(isValid(exp_at))
  //     config.provider[i].token_expires_at = atoi(exp_at);
  //   free(exp_at);
  //   if(isValid(salt_hex))
  //     strcpy(config.provider[i].salt_hex, salt_hex);
  //   free(salt_hex);
  //   if(isValid(nonce_hex))
  //     strcpy(config.provider[i].nonce_hex, nonce_hex);
  //   free(nonce_hex);
  //   if(isValid(len))
  //     config.provider[i].encrypted_password_len = atoi(len);
  //   free(len);
  //   syslog(LOG_AUTHPRIV|LOG_DEBUG, "setted len is %lu", config.provider[i].encrypted_password_len);
  //   t_index += t[t_index].size*2+2;
  // }
  // free(pairs[5].value);
  // logConfig();
  //
}

