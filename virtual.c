/* read the README.virtual for more info on using the
*  virtual patch for vm-pop3d */

#include "vm-pop3d.h"

#ifdef USE_VIRTUAL

#ifndef VIRTUAL_PASSWORDS_PATH
#define VIRTUAL_PASSWORDS_PATH  "/etc/virtual"
#endif

#ifndef VIRTUAL_PASSWORD_FNAME
#define VIRTUAL_PASSWORD_FNAME  "passwd"
#endif

/* The following should probably try to use some system defaults. */
#define MAX_USERNAME_LENGTH 128
#define MAX_PASSWORD_LENGTH 128

/* the following is just used by C preprocessor to stringify numbers */
#define SNUMBER_(a) #a
#define SNUMBER(a) SNUMBER_(a)

struct passwd global_pwdentry;
char global_pwdentry_pw_name[MAX_USERNAME_LENGTH + 1];
char global_pwdentry_pw_passwd[MAX_PASSWORD_LENGTH + 1];

struct passwd *
getvirtualpwnam(const char *username, const char *domainname)
{
  FILE *passwdfd;
  char buffer[256];
  struct passwd *pwdentry = &global_pwdentry;
  int found = 0;

  /* return a pointer to an internal static object */
  pwdentry->pw_name = global_pwdentry_pw_name;
  pwdentry->pw_passwd = global_pwdentry_pw_passwd;
  
  snprintf(buffer, 256, "%s/%s/%s",
	   VIRTUAL_PASSWORDS_PATH, domainname, VIRTUAL_PASSWORD_FNAME);

  if ((passwdfd = fopen(buffer, "r")) == (FILE *) NULL) {
    /* fprintf (stderr, "Couldn't open password file: %s\n", buffer); */
    syslog(LOG_ERR, "Couldn't open password file: %s", buffer);
    return (0);
  }
  while (fgets(buffer, sizeof(buffer), passwdfd) != (char *) NULL) {

    /* skip comments, blank lines */
    if (*buffer == '#' || *buffer == '\n' || *buffer == '\0') {
      continue;
    }
    if (sscanf(buffer,
               "%" SNUMBER(MAX_USERNAME_LENGTH) "[^:]:"
               "%" SNUMBER(MAX_PASSWORD_LENGTH) "[^:\n]",
	       pwdentry->pw_name, pwdentry->pw_passwd) != 2) {
      /* fprintf (stderr, "%s: Trouble parsing passwd file\n", buffer); */
      continue;
    }
    if (strcmp(pwdentry->pw_name, username) == 0) {
      /* uid of zero is not allowed */
      if (uid) pwdentry->pw_uid = uid; /* as set via command-line */
      else pwdentry->pw_uid = (uid_t) VIRTUAL_UID;
      found = 1;
      break;
    }
  }

  (void) fclose(passwdfd);	/* what should I do? Log if it doesn't
				 * close? */

  if (found)
    return (pwdentry);
  else
    return (0);

}

#endif				/* USE_VIRTUAL */
