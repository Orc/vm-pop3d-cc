/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* If defined, use this location relative to user's home directory for their
 * mailbox before system spool. Must have a leading / and match mailer
 * configuration. Off by default. If enabled but no path is given, defaults
 * to "/Mail/mailbox" */
/* #undef MAILSPOOLHOME */

/* Define if you have the crypt function.  */
/* #undef HAVE_CRYPT */

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the <paths.h> header file.  */
#define HAVE_PATHS_H 1

/* Define if you have the <shadow.h> header file.  */
#define HAVE_SHADOW_H 1

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <syslog.h> header file.  */
#define HAVE_SYSLOG_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the crypt library (-lcrypt).  */
#define HAVE_LIBCRYPT 1

/* Define if you have the pam library (-lpam).  */
#define USE_PAM 1

/* Using local getopt.h */
/* #undef USE_LOCAL_GETOPT_H */

/* Define if you want virtual domains support. */
#define USE_VIRTUAL 1

/* Define if you want to use IP-based virtual domains support. */
/* #undef IP_BASED_VIRTUAL */

/* Define if you want extra logging and messages. */
#define DEBUG 1
