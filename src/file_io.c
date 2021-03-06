#include "file_io.h"
#include "oidc_utilities.h"
#include "../lib/list/src/list.h"

#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <syslog.h>
#include <unistd.h>

char* possibleLocations[] = {"~/.config/oidc-agent/", "~/.oidc-agent/"};


/** @fn char* readFile(const char* path)
 * @brief reads a file and returns a pointer to the content
 * @param path the file to be read
 * @return a pointer to the file content. Has to be freed after usage. On
 * failure NULL is returned and oidc_errno is set.
 */
char* readFile(const char* path) {
  syslog(LOG_AUTHPRIV|LOG_DEBUG, "Reading file: %s", path);
  FILE *fp;
  long lSize;
  char *buffer;

  fp = fopen ( path, "rb" );
  if( !fp ) {
    syslog(LOG_AUTHPRIV|LOG_NOTICE, "%m\n");
    oidc_errno = OIDC_EFOPEN;
    return NULL;
  }

  fseek( fp , 0L , SEEK_END);
  lSize = ftell( fp );
  rewind( fp );

  buffer = calloc( 1, lSize+1 );
  if( !buffer ) {
    fclose(fp);
    syslog(LOG_AUTHPRIV|LOG_ALERT, "memory alloc failed in function readFile '%s': %m\n", path);
    oidc_errno = OIDC_EALLOC;
    return NULL;
  }

  if( 1!=fread( buffer , lSize, 1 , fp) ) {
    if(feof(fp)) {
      oidc_errno = OIDC_EEOF;
    } else {
      oidc_errno = OIDC_EFREAD;
    }
    fclose(fp);
    clearFree(buffer, lSize);
    syslog(LOG_AUTHPRIV|LOG_ALERT, "entire read failed in function readFile '%s': %m\n", path);
    return NULL;
  }
  fclose(fp);
  return buffer;
}

/** @fn char* readOidcFile(const char* filename)
 * @brief reads a file located in the oidc dir and returns a pointer to the content
 * @param filename the filename of the file
 * @return a pointer to the file content. Has to be freed after usage.
 */
char* readOidcFile(const char* filename) {
  char* path = concatToOidcDir(filename);
  char* c = readFile(path);
  clearFreeString(path);
  return c;
}

/** @fn void writeFile(const char* path, const char* text)
 * @brief writes text to a file
 * @note \p text has to be nullterminated and must not contain nullbytes. 
 * @param path the file to be written
 * @param text the nullterminated text to be written
 * @return OIDC_OK on success, OID_EFILE if an error occured. The system sets
 * errno.
 */
oidc_error_t writeFile(const char* path, const char* text) {
  FILE *f = fopen(path, "w");
  if (f == NULL) {
    syslog(LOG_AUTHPRIV|LOG_ALERT, "Error opening file '%s' in function writeToFile().\n", path);
    return OIDC_EFOPEN;
  }
  fprintf(f, "%s", text);
  fclose(f);
  return OIDC_SUCCESS;
}

/** @fn void writeOidcFile(const char* filename, const char* text)
 * @brief writes text to a file located in the oidc directory
 * @note \p text has to be nullterminated and must not contain nullbytes. 
 * @param filename the file to be written
 * @param text the nullterminated text to be written
 * @return OIDC_OK on success, OID_EFILE if an error occured. The system sets
 * errno.
 */
oidc_error_t writeOidcFile(const char* filename, const char* text) {
  char* path = concatToOidcDir(filename);
  oidc_error_t er = writeFile(path, text);
  clearFreeString(path);
  return er;
}

/** @fn int fileDoesExist(const char* path)
 * @brief checks if a file exists
 * @param path the path to the file to be checked
 * @return 1 if the file does exist, 0 if not
 */
int fileDoesExist(const char* path) {
  return access(path, F_OK)==0 ? 1 : 0;
}

/** @fn int oidcFileDoesExist(const char* filename)
 * @brief checks if a file exists in the oidc dir
 * @param filename the file to be checked
 * @return 1 if the file does exist, 0 if not
 */
int oidcFileDoesExist(const char* filename) {
  char* path = concatToOidcDir(filename);
  int b = fileDoesExist(path);
  clearFreeString(path);
  return b;
}

/** @fn int dirExists(const char* path)
 * @brief checks if a directory exists
 * @param path the path to the directory to be checked
 * @return 1 if the directory does exist, 0 if not, -1 if an error occured
 */
int dirExists(const char* path) {
  DIR* dir = opendir(path);
  if(dir) {/* Directory exists. */
    closedir(dir);
    return 1;
  } else if (ENOENT == errno) { /* Directory does not exist. */
    return 0;
  } else { /* opendir() failed for some other reason. */
    syslog(LOG_AUTHPRIV|LOG_ALERT, "opendir: %m");
    exit(EXIT_FAILURE);
    return -1;
  }
}

/** @fn char* getOidcDir()
 * @brief get the oidc directory path
 * @return a pointer to the oidc directory path. Has to be freed after usage. If
 * no oidc dir is found, NULL is returned
 */
char* getOidcDir() {
  char* home = getenv("HOME");
  unsigned int i;
  for(i=0; i<sizeof(possibleLocations)/sizeof(*possibleLocations); i++) {
    char* path = oidc_strcat(home, possibleLocations[i]+1);
    syslog(LOG_AUTHPRIV|LOG_DEBUG, "Checking if dir '%s' exists.", path);
    if(dirExists(path)>0) {
      return path;
    }
    clearFreeString(path);
  }
  return NULL;
}

/** @fn int removeFile(const char* path)
 * @brief removes a file
 * @param path the path to the file to be removed
 * @return On success, 0 is returned.  On error, -1 is returned, and errno is set appropriately.
 */
int removeFile(const char* path) {
  return unlink(path);
}

/** @fn int removeOidcFile(const char* filename)
 * @brief removes a file located in the oidc dir
 * @param filename the filename of the file to be removed
 * @return On success, 0 is returned.  On error, -1 is returned, and errno is set appropriately.
 */
int removeOidcFile(const char* filename) {
  char* path = concatToOidcDir(filename);
  int r = removeFile(path);
  clearFreeString(path);
  return r;
}

char* concatToOidcDir(const char* filename) {
  char* oidc_dir = getOidcDir();
  char* path = oidc_strcat(oidc_dir, filename);
  clearFreeString(oidc_dir);
  return path;
}

list_t* getFileListForDirIf(const char* dirname, int (match(const char*, const char*)), const char* arg) {
  DIR* dir;
  struct dirent* ent;
  if ((dir = opendir (dirname)) != NULL) {
    list_t* list = list_new();
    list->free = (void(*) (void*)) &clearFreeString;
    list->match = (int(*) (void*, void*)) &strequal;
    while ((ent = readdir (dir)) != NULL) {
      if(strcmp(ent->d_name, ".")!=0 && strcmp(ent->d_name, "..")!=0) {
#ifdef _DIRENT_HAVE_DTYPE
        if(ent->d_type==DT_REG) {
          if(match(ent->d_name, arg)) {
            list_rpush(list, list_node_new(oidc_strcopy(ent->d_name)));
          }
        }
#else
        if(match(ent->d_name, arg)) {
          list_rpush(list, list_node_new(oidc_strcopy(ent->d_name)));
        }
#endif
      }
    }
    closedir (dir);
    return list;
  } else {
    oidc_seterror(strerror(errno));
    oidc_errno = OIDC_EERROR;
    return NULL;
  }
}

int alwaysOne(const char* a __attribute__((unused)), const char* b __attribute__((unused))) {
  return 1;
}

list_t* getFileListForDir(const char* dirname) {
  return getFileListForDirIf(dirname, &alwaysOne, NULL);
}

int isClientConfigFile(const char* filename, const char* a __attribute__((unused))) {
  const char* const suffix = ".clientconfig";
  if(strEnds(filename, suffix)) {
    return 1;
  }
  char* pos = NULL;
  if((pos = strstr(filename, suffix))) {
    pos += strlen(suffix);
    while(*pos!='\0') {
      if(!isdigit(*pos)) {
        return 0;
      }
      pos++;
    }
    return 1;
  }
  return 0;
}

int isAccountConfigFile(const char* filename, const char* a __attribute__((unused))) {
  if(isClientConfigFile(filename, a)) {
    return 0;
  }
  if(strEnds(filename, ".config")) {
    return 0;
  }
  return 1;
}

list_t* getAccountConfigFileList() {
  char* oidc_dir = getOidcDir();
  list_t* list = getFileListForDirIf(oidc_dir, &isAccountConfigFile, ".clientconfig");
  clearFreeString(oidc_dir);
  return list;
}

list_t* getClientConfigFileList() {
  char* oidc_dir = getOidcDir();
  list_t* list = getFileListForDirIf(oidc_dir, &isClientConfigFile, ".clientconfig");
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(list, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    char* old = node->val;
    node->val = oidc_strcat(oidc_dir, old);
    clearFreeString(old);
  }
  clearFreeString(oidc_dir);
  return list;
}

