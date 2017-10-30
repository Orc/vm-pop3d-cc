/* virtualmail-pop3d - a POP3 server with virtual domains support
   This code is licensed under the GPL; it has several authors.
   vm-pop3d is based on:
   GNU POP3 - a small, fast, and efficient POP3 daemon
   Copyright (C) 1999 Jakob 'sparky' Kaivo <jkaivo@nodomainname.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "vm-pop3d.h"

static struct x_option opts[] = {
#ifdef DEBUG
  {'D', 'D', "debug", "?LEVEL", "set debugging level" },
#endif
  {'d', 'd', "daemon", "?JOBS", "run in daemon mode with JOB (10) threads" },
  {'s', 's', "service","?JOBS", "run as a systemd service with JOB (10) threads" },
  {'g', 'g', "group",   "NAME", "run in group NAME" },
  {'u', 'u', "user",    "NAME", "run as user NAME" },
  {'h', 'h', "help",         0, "show the usage, then quit" },
  {'i', 'i', "inetd",        0, "run as an inetd client" },
  {'p', 'p', "port",  "NUMBER", "attach to port NUMBER instead of the pop3 port" },
  {'t', 't', "timeout", "SECS", "time out after being idle for SECS" },
  {'v', 'v', "version",      0, "tell what version (" VERSION ") we are, then quit" },
  {'o', 'o', "traditional",  0, "traditional pop3 mode (editable mailboxes, no concurrency)" },
};
#define NROPTS (sizeof opts / sizeof opts[0])

int
main(int argc, char **argv)
{
  struct group *gr;
  static int mode = UNSET;
  int maxchildren = 10;
  int c = 0;
  char *group_name = NULL;
  char *uid_name = NULL;
  gid_t gid;
  struct passwd *temp_pw;

#ifdef DEBUG
  debug = -1; /* no debugging by default */
#endif
  port = 110;			/* Default POP3 port */
  timeout = 0;			/* Timeout turned off */

  uid = 0;			/* Not superuser -- just unset */
  mbox_writable = 0;		/* concurrency; can't edit the mbox */

  /* Set the signal handlers */
  signal(SIGINT, pop3_signal);
  signal(SIGQUIT, pop3_signal);
  signal(SIGILL, pop3_signal);
  signal(SIGBUS, pop3_signal);
  signal(SIGFPE, pop3_signal);
  signal(SIGSEGV, pop3_signal);
  signal(SIGTERM, pop3_signal);

/* Don't die when a process goes away unexpectedly.
   Ignore write on a pipe with no reader.  */
  signal(SIGPIPE, SIG_IGN);

  while ( (c = x_getopt(argc, argv, NROPTS, opts)) != EOF ) {
    switch (c) {
    case 'D':
#ifdef DEBUG
      debug = optarg ? atoi(x_optarg) : 1; 
#endif
      break;
    case 's':
    case 'd':
      mode = (c == 'd') ? DAEMON : SERVICE;
      maxchildren = x_optarg ? atoi(x_optarg) : 10;
      break;
    case 'g':
      if ( !(group_name = strdup(x_optarg)) ) {
        fprintf(stderr, "Exiting: can't allocate memory");
        exit(1);
      }
      break;
    case 'i':
      mode = INTERACTIVE;
      break;
    case 'p':
      if ( mode == UNSET) mode = DAEMON;
      port = atoi(x_optarg);
      break;
    case 't':
      timeout = atoi(x_optarg);
      /* RFC 1939 says no less than 10 minutes, but we'll let them use any */
      break;
    case 'u':
      if ( !(uid_name = strdup(x_optarg)) ) {
        fprintf(stderr, "Exiting: can't allocate memory");
        exit(1);
      }
      break;
    case 'o':
	/* traditional mode: can delete, can't be concurrent */
	mbox_writable = 1;
	break;
	
    case 'v':
      printf("vm-pop3d POP3 Server Version " VERSION "\n");
      exit(0);
    
    case 'h':
    default:
	printf("Usage: %s [OPTIONS]\n", argv[0]);
	if ( c == 'h' )
	    printf("Runs the vm-pop3d POP3 daemon.\n\n");    
	showopts(stdout, NROPTS, opts);
	exit((c=='h')?0:1);
    }
  }

  /* Use command-line group option or use "mail" group */
  if ( !(group_name || (group_name = strdup("mail"))) ) {
      fprintf(stderr, "Exiting: can't allocate memory");
      exit(1);
  }

  if ((gr = getgrnam(group_name)) == NULL) {
    /* then see if it is a GID */
    gid = (gid_t)atoi(group_name);
    /* zero is also error */
    if ((gid_t)gid == 0) {
      fprintf(stderr, "Group %s not found\n", group_name);
      exit(1);
    }
    if ((gr = getgrgid((gid_t)gid)) == NULL) {
      fprintf(stderr, "GID %s not found\n", group_name);
      exit(1);
    }
  }

  if (setgid(gr->gr_gid) == -1) {
    fprintf(stderr, "Error setting group: %s (%d)\n", group_name, gr->gr_gid);
    exit(1);
  }
  if (group_name) free(group_name);

  if (uid_name) {
    if ((temp_pw = getpwnam(uid_name)) == NULL) {
      /* then see if it is a GID */
      uid = (uid_t)atoi(uid_name);
      /* zero is also error */
      if ((uid_t)uid == 0) {
        fprintf(stderr, "User %s not found\n", uid_name);
        exit(1);
      }
      if ((temp_pw = getpwuid((uid_t)uid)) == NULL) {
        fprintf(stderr, "UID %s not found\n", uid_name);
        exit(1);
      }
    }
    uid = temp_pw->pw_uid;
    free(uid_name);
  }
#ifdef USE_VIRTUAL
  else {
    if (getpwuid((uid_t)VIRTUAL_UID) == NULL) {
      fprintf(stderr, "VIRTUAL UID %d not found\n", VIRTUAL_UID);
      exit(1);
    }
  }
#endif

  if (mode == DAEMON)
    pop3_daemon_init();

  /* change directories */
#ifdef MAILSPOOLHOME
  chdir("/");
#else
  chdir(_PATH_MAILDIR);
#endif

  /* Set up for syslog */
  openlog("vm-pop3d", LOG_PID, LOG_MAIL);

  umask(S_IROTH | S_IWOTH | S_IXOTH);	/* 007 */

  /* Actually run the daemon */
  switch (mode) {
  case SERVICE:
  case DAEMON:
    pop3_daemon(maxchildren);
    break;
  case INTERACTIVE:
  default:
    pop3_mainloop(fileno(stdin), fileno(stdout));
    break;
  }

  /* Close the syslog connection and exit */
  closelog();
  return OK;
}

/* Sets things up for daemon mode */

void
pop3_daemon_init(void)
{
  if (fork())
    exit(0);			/* parent exits */
  setsid();			/* become session leader */

  if (fork())
    exit(0);			/* new parent exits */

  /* close inherited file descriptors */
  close(0);
  close(1);
  close(2); /* errors need to go syslog now */

  signal(SIGHUP, SIG_IGN);	/* ignore SIGHUP */

  signal(SIGCHLD, SIG_DFL);    /* for forking */
}

/* The main part of the daemon. This function reads input from the client and
   executes the proper functions. Also handles the bulk of error reporting. */

int
pop3_mainloop(int infile, int outfile)
{
  int status = OK;
  char *buf, *arg, *cmd;
  struct hostent *htbuf;
  char *local_hostname;
  struct sockaddr_in name;
  int namelen = sizeof(name);
  char *temp_domain;
#ifdef USE_VIRTUAL
  const char domain_delimiters[] = ";!:@";
  int i;
#endif

#ifdef IP_BASED_VIRTUAL
  struct hostent *hostent_info;
#endif

  ifile = infile;
  ofile = fdopen(outfile, "w+");
  if (ofile == NULL)
    pop3_abquit(ERR_NO_OFILE);
  state = AUTHORIZATION;
  lockfile = NULL;
  curr_time = time(NULL);

  if (getpeername(infile,
        (struct sockaddr *) & name, (socklen_t *) & namelen) == 0) {
    if ((temp_domain = (char *) inet_ntoa(name.sin_addr)))
      syslog(LOG_INFO,"Connect from %s", temp_domain);
    else syslog(LOG_INFO, "Connection opened");
  }
  else syslog(LOG_INFO, "Incoming connection opened");

  /* Prepare the shared secret for APOP */
  local_hostname = malloc(MAXHOSTNAMELEN + 1);
  if (local_hostname == NULL)
    pop3_abquit(ERR_NO_MEM);

  gethostname(local_hostname, MAXHOSTNAMELEN);
  htbuf = gethostbyname(local_hostname);
  if (htbuf) {
    free(local_hostname);
    local_hostname = strdup(htbuf->h_name);
  }
  md5shared = malloc(strlen(local_hostname) + 51);
  if (md5shared == NULL)
    pop3_abquit(ERR_NO_MEM);

  snprintf(md5shared, strlen(local_hostname) + 50, "<%d.%ld@%s>", getpid(),
	   time(NULL), local_hostname);
  free(local_hostname);

  fflush(ofile);
  fprintf(ofile, "+OK POP3 " WELCOME " %s\r\n", md5shared);

/* vm-pop3d can run as a different user; for example, you can use
 * inetd's user field setting to run it as a different user.
 */
#ifdef DEBUG
    if (debug > 5)
      syslog (LOG_INFO, "uid %d, gid %d", getuid(), getgid());
#endif

#ifdef USE_VIRTUAL
  virtualdomain = NULL;
#endif

  while (state != UPDATE) {
    fflush(ofile);
    status = OK;
    buf = pop3_readline(ifile);
    cmd = pop3_cmd(buf);
    arg = pop3_args(buf);

#ifdef DEBUG
    if (debug > 5)
      syslog (LOG_ERR, "cmd: %s %s\n", cmd, arg);
#endif

    if (strlen(arg) > POP_MAXCMDLEN || strlen(cmd) > POP_MAXCMDLEN)
      status = ERR_TOO_LONG;
    else if (strlen(cmd) > 4)
      status = ERR_BAD_CMD;
    else if (strncasecmp(cmd, "USER", 4) == 0) {

#ifdef USE_VIRTUAL
      /* Some mail clients pre-parse out the username before sending */
      /* so allow colon or at-sign to separate username and domain name */

      for (i = strlen(domain_delimiters) - 1; i >= 0; i--) {
	temp_domain = strchr(arg, domain_delimiters[i]);
	if (temp_domain) {
	  temp_domain++;
/*                virtualdomain = malloc (MAXHOSTNAMELEN + 1); */
/*                strncpy (virtualdomain, (temp_domain + 1), MAXHOSTNAMELEN); */
	  virtualdomain = strdup(temp_domain);
/*                virtualdomain++; */
	  arg = strtok(arg, domain_delimiters);
	  break;
	}
      }

/* placing domain name in USER command overrides IP_BASED_VIRTUAL */

#ifdef IP_BASED_VIRTUAL
      if (virtualdomain == NULL) {
	if (getsockname(infile,
	      (struct sockaddr *) & name, (socklen_t *) & namelen) < 0) {
#ifdef DEBUG
          if (debug > 3)
            syslog(LOG_ERR,"getsockname: %m");
#endif
	  pop3_abquit(ERR_IP_BASED);
	}
	if (!(temp_domain = (char *) inet_ntoa(name.sin_addr))) {
#ifdef DEBUG
          if (debug > 3)
            syslog(LOG_ERR,"inet_ntoa failed: %m");
#endif
	  pop3_abquit(ERR_IP_BASED);
	}
#ifdef DEBUG
        if (debug > 6)
          syslog(LOG_INFO,"Connection to %s", temp_domain);
#endif

	if ((hostent_info = gethostbyaddr((char *) &name.sin_addr, 4, AF_INET))
	    != NULL) {
	  virtualdomain = malloc(MAXHOSTNAMELEN + 1);
	  if (virtualdomain == NULL)
	    pop3_abquit(ERR_NO_MEM);
	  strcpy(virtualdomain, hostent_info->h_name);
#ifdef DEBUG
          if (debug > 6)
            syslog (LOG_INFO, "hostname: %s", virtualdomain);
#endif
	} else {
#ifdef DEBUG
          if (debug > 3)
            syslog(LOG_ERR, "gethostbyaddr failure %d for %s\n",
                   h_errno, temp_domain);
#endif
	  pop3_abquit(ERR_IP_BASED);
	}
      }
#endif				/* IP_BASED_VIRTUAL */
      status = pop3_user(arg, virtualdomain);
      if (virtualdomain) {
	free(virtualdomain);
	virtualdomain = NULL;
      }
#else
syslog(LOG_ERR, "pop3 user: %s\n", arg);
      status = pop3_user(arg);
#endif				/* USE_VIRTUAL */

    } else if (strncasecmp(cmd, "QUIT", 4) == 0)
      status = pop3_quit(arg);
    else if (strncasecmp(cmd, "APOP", 4) == 0)
      status = pop3_apop(arg);
    else if (strncasecmp(cmd, "AUTH", 4) == 0)
      status = pop3_auth(arg);
    else if (strncasecmp(cmd, "STAT", 4) == 0)
      status = pop3_stat(arg);
    else if (strncasecmp(cmd, "LIST", 4) == 0)
      status = pop3_list(arg);
    else if (strncasecmp(cmd, "RETR", 4) == 0)
      status = pop3_retr(arg);
    else if (strncasecmp(cmd, "DELE", 4) == 0)
      status = pop3_dele(arg);
    else if (strncasecmp(cmd, "NOOP", 4) == 0)
      status = pop3_noop(arg);
    else if (strncasecmp(cmd, "RSET", 4) == 0)
      status = pop3_rset(arg);
    else if ((strncasecmp(cmd, "TOP", 3) == 0) && (strlen(cmd) == 3))
      status = pop3_top(arg);
    else if (strncasecmp(cmd, "UIDL", 4) == 0)
      status = pop3_uidl(arg);
    else if (strncasecmp(cmd, "CAPA", 4) == 0)
      status = pop3_capa(arg);
    else
      status = ERR_BAD_CMD;

    if (status == OK)
      fflush(ofile);
    else if (status == ERR_WRONG_STATE)
      fprintf(ofile, "-ERR " BAD_STATE "\r\n");
    else if (status == ERR_BAD_ARGS)
      fprintf(ofile, "-ERR " BAD_ARGS "\r\n");
    else if (status == ERR_NO_MESG)
      fprintf(ofile, "-ERR " NO_MESG "\r\n");
    else if (status == ERR_NOT_IMPL)
      fprintf(ofile, "-ERR " NOT_IMPL "\r\n");
    else if (status == ERR_BAD_CMD)
      fprintf(ofile, "-ERR " BAD_COMMAND "\r\n");
    else if (status == ERR_BAD_LOGIN)
      fprintf(ofile, "-ERR " BAD_LOGIN "\r\n");
    else if (status == ERR_MBOX_LOCK)
      fprintf(ofile, "-ERR [IN-USE] " MBOX_LOCK "\r\n");
    else if (status == ERR_TOO_LONG)
      fprintf(ofile, "-ERR " TOO_LONG "\r\n");
    else if (status == ERR_LOCK_FAILED)
      fprintf(ofile, "-ERR " LOCK_FAILED "\r\n");

    free(buf);
    free(cmd);
    free(arg);
  }

  fflush(ofile);
  return OK;
}

/* Runs in standalone daemon mode. This opens and binds to a port
   (default 110) then executes a pop3_mainloop() upon accepting a connection.
   It starts maxchildren child processes to listen to and accept socket
   connections */

int
pop3_daemon(int maxchildren)
{
  int children = 0;
  struct sockaddr_in client;
  int wait_status, sock, sock2;
  unsigned int socksize;
  pid_t pid;

  sock = socket(PF_INET, SOCK_STREAM, (getprotobyname("tcp"))->p_proto);
  if (sock < 0) {
    syslog(LOG_ERR, "%s\n", strerror(errno));
    exit(1);
  }
  memset(&client, 0, sizeof(struct sockaddr_in));
  client.sin_family = AF_INET;
  client.sin_port = htons(port);
  client.sin_addr.s_addr = htonl(INADDR_ANY);
  socksize = sizeof(client);
  if (bind(sock, (struct sockaddr *) & client, socksize)) {
    syslog(LOG_ERR, "Couldn't bind to socket: %s", strerror(errno));
    exit(1);
  }
  listen(sock, 128);
  while (1) {
    if (children < maxchildren) {
      if ((pid = fork()) < 0) { /* can't fork */
#ifdef DEBUG
       if (debug > 2)
         syslog(LOG_ERR, "Can't start server %d: %s", children, strerror(errno));
#else
      /* what should this do? */
#endif
      }
      else if (pid == 0) { /* following code executes in child process */
        if ((sock2 = accept(sock, (struct sockaddr *) & client,
		       (socklen_t *) & socksize)) < 1) {
          syslog(LOG_ERR, "Couldn't accept a connection on a socket: %s",
                strerror(errno));
          _exit(127);
        }
        pop3_mainloop(sock2, sock2);
	close(sock2);
	_exit(OK);
      } else { /* this is the parent */
	children++;
#ifdef DEBUG
       if (debug > 2)
         syslog(LOG_INFO, "Started server %d (%d)", children, pid);
#endif
      }
    } else {
      if ((pid = waitpid (0, &wait_status, 0)) > 0) {
        children--;
#ifdef DEBUG
        if (debug > 2)
          syslog(LOG_INFO, "Server exited (%d)", pid);
#endif
      }
#ifdef DEBUG
      else {
        if (WIFEXITED(wait_status)) {
          syslog(LOG_WARNING, "waitpid pid=%d, status=%d",
                 pid, WEXITSTATUS(wait_status));
        } else if (WIFSIGNALED(wait_status)) {
          syslog(LOG_WARNING, "waitpid pid=%d, signal=%d",
                 pid, WTERMSIG(wait_status));
        } else if (WIFSTOPPED(wait_status)) {
          syslog(LOG_WARNING, "waitpid pid=%d, stopped=%d",
                 pid, WSTOPSIG(wait_status));
        } else syslog(LOG_WARNING, "waitpid pid=%d, status=%d",
                      pid, wait_status);
      }
#endif
    }
  }
  return OK;
}
