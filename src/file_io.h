#ifndef FILE_IO_H
#define FILE_IO_H

#include "oidc_error.h"
#include "../lib/list/src/list.h"

char* getOidcDir() ;
oidc_error_t writeOidcFile(const char* filename, const char* text) ;
oidc_error_t writeFile(const char* filepath, const char* text) ;
char* readOidcFile(const char* filename) ;
char* readFile(const char* path);
int fileDoesExist(const char* path);
int oidcFileDoesExist(const char* filename) ;
int removeOidcFile(const char* filename) ;
char* concatToOidcDir(const char* filename) ;
list_t* getAccountConfigFileList() ;
list_t* getClientConfigFileList() ;

#endif // FILE_IO_H 
