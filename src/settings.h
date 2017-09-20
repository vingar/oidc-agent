#ifndef OIDC_SETTINGS_H
#define OIDC_SETTINGS_H


// env var names
#define OIDC_SOCK_ENV_NAME "OIDC_SOCK"
#define OIDC_PID_ENV_NAME "OIDCD_PID"


// file names
#define ISSUER_CONFIG_FILENAME "issuer.config"

#define MAX_PASS_TRIES 3

#define CONF_ENDPOINT_SUFFIX ".well-known/openid-configuration"

static char* possibleCertFiles[] = {
  "/etc/ssl/certs/ca-certificates.crt", // Debian/Ubuntu/Gentoo etc.
  "/etc/pki/tls/certs/ca-bundle.crt",   // Fedora/RHEL
  "/etc/ssl/ca-bundle.pem",             // OpenSUSE
  "/etc/pki/tls/cacert.pem"             // OpenELEC
};


#endif // OIDC_SETTINGS_H