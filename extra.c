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

/* Takes a string as input and returns either the remainder of the string
   after the first space, or a zero length string if no space */

char *
pop3_args(const char *cmd)
{
  int space = -1, i = 0, len;
  char *buf;

  len = strlen(cmd) + 1;
  buf = malloc(len * sizeof(char));
  if (buf == NULL)
    pop3_abquit(ERR_NO_MEM);

  while (space < 0 && i < len) {
    if (cmd[i] == ' ')
      space = i + 1;
    else if (cmd[i] == '\0' || cmd[i] == '\r' || cmd[i] == '\n')
      len = i;
    i++;
  }

  if (space < 0)
    buf[0] = '\0';
  else {
    for (i = space; i < len; i++)
      if (cmd[i] == '\0' || cmd[i] == '\r' || cmd[i] == '\n')
	buf[i - space] = '\0';
      else
	buf[i - space] = cmd[i];
  }

  return buf;
}

/* This takes a string and returns the string up to the first space or end of
   the string, whichever occurs first */

char *
pop3_cmd(const char *cmd)
{
  char *buf;
  int i = 0, len;

  len = strlen(cmd) + 1;
  buf = malloc(len * sizeof(char));
  if (buf == NULL)
    pop3_abquit(ERR_NO_MEM);

  for (i = 0; i < len; i++) {
    if (cmd[i] == ' ' || cmd[i] == '\0' || cmd[i] == '\r' || cmd[i] == '\n')
      len = i;
    else
      buf[i] = cmd[i];
  }
  buf[i - 1] = '\0';
  return buf;
}

/* Returns OK if a message actually exists and has not been marked by dele(),
   otherwise ERR_NO_MESG */

int
pop3_mesg_exist(int mesg)
{
  if ((mesg < 0) || (mesg >= num_messages) || (messages[mesg].deleted == 1))
    return ERR_NO_MESG;

  return OK;
}

/* This is called if it needs to quit without going to the UPDATE stage.
   This is used for conditions such as out of memory, a broken socket, or
   being killed on a signal */

int
pop3_abquit(int reason)
{
  pop3_unlock();
  if (mbox != NULL)
    fclose(mbox);
  free(messages);

/* Jonathan Chin <jonathan.chin@ox.compsoc.net>
   Ensure that ofile is non-null so following routine will
   not attempt to write an error message through the socket.
*/

  switch (reason) {
  case ERR_NO_MEM:
    if (NULL != ofile)
      fprintf(ofile, "-ERR Out of memory, quitting\r\n");
    syslog(LOG_ERR, "Out of memory");
    break;
  case ERR_DEAD_SOCK:
    if (NULL != ofile)  /* should this even print to socket? */
      fprintf(ofile, "-ERR Socket closed, quitting\r\n");
    syslog(LOG_ERR, "Socket closed");
    break;
  case ERR_SIGNAL:
    if (NULL != ofile)
      fprintf(ofile, "-ERR Quitting on signal\r\n");
    break;
  case ERR_TIMEOUT:
    if (NULL != ofile)
      fprintf(ofile, "-ERR Session timed out\r\n");
    if (state == TRANSACTION)
      syslog(LOG_INFO, "Session timed out for user: %s", username);
    else
      syslog(LOG_INFO, "Session timed out for no user");
    break;
#ifdef IP_BASED_VIRTUAL
  case ERR_IP_BASED:
    if (NULL != ofile)
      fprintf(ofile, "-ERR Quitting - host name not found\r\n");
    syslog(LOG_ERR, "Quitting - host name not found");
    break;
#endif				/* IP_BASED_VIRTUAL */
  case ERR_NO_OFILE:
    syslog(LOG_ERR, "Quitting - couldn't open stream");
    break;
  default:
    if (NULL != ofile)
      fprintf(ofile, "-ERR Quitting (reason unknown)\r\n");
    syslog(LOG_ERR, "Unknown quit");
    break;
  }
  fflush(ofile);
  exit(1);
}

/* This locks the mailbox file, works with procmail locking */

int
pop3_lock(void)
{
  struct flock fl;

  if ( !mbox_writable )
      return OK;

  fl.l_type = F_RDLCK;
  fl.l_whence = SEEK_CUR;
  fl.l_start = 0;
  fl.l_len = 0;
  lockfile = malloc(strlen(mailbox) + strlen(".lock") + 1);
  if (lockfile == NULL)
    pop3_abquit(ERR_NO_MEM);
  strcpy(lockfile, mailbox);
  strcat(lockfile, ".lock");
  lock = fopen(lockfile, "r");
  if (lock != NULL) {
    free(lockfile);
    lockfile = NULL;
    return ERR_MBOX_LOCK;
  }
  lock = fopen(lockfile, "w");
  if (lock == NULL) {
    syslog(LOG_ERR, "Failed to create lockfile: \"%s\"\n", lockfile);
    free(lockfile);
    lockfile = NULL;
    return ERR_LOCK_FAILED;
  }
  if (fcntl(fileno(mbox), F_SETLK, &fl) == -1) {
    syslog(LOG_ERR, "%s\n", strerror(errno));
    return ERR_FILE;
  }
  return OK;
}

/* Unlocks the mailbox */

int
pop3_unlock(void)
{
  struct flock fl;

  if ( !mbox_writable )
      return OK;
      
  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_CUR;
  if (mbox != NULL)
    fcntl(fileno(mbox), F_SETLK, &fl);
  if (lockfile != NULL) {
    fclose(lock);
    if (unlink(lockfile) == -1) {
      syslog(LOG_ERR, "Removing lockfile %s: %s\n", lockfile, strerror(errno));
      free(lockfile);
      return ERR_FILE;
    }
    free(lockfile);
  }
  return OK;
}

/* This parses the mailbox to get the number of messages, the size of each
   message, and the read offset of the first character of each message. This
   is where the real speed magic is done */

int
pop3_getsizes(void)
{
  char buf[80];
  unsigned int max_count = 0;
  /* start with 1 because first line of mbox is From */
  int new_line = 1, empty_line = 1;

  num_messages = 0;

  if (messages == NULL) {
    messages = malloc(sizeof(message) * 10);
    if (messages == NULL)
      pop3_abquit(ERR_NO_MEM);
    max_count = 10;
  }
  while (fgets(buf, 80, mbox)) {
    /* Only count a message if "From " is a real mbox delimiter.
       It depends on the delivery agent; some escape all "From " lines;
       some don't. */
    if (empty_line && !strncmp(buf, "From ", 5)) {
      while (strchr(buf, '\n') == NULL)
        fgets(buf, 80, mbox); /* get all of From line */
 
      num_messages++;

      if (num_messages > max_count) {
	max_count = num_messages * 2;
	messages = realloc(messages, 2 * max_count * sizeof(message));
	if (messages == NULL)
	  pop3_abquit(ERR_NO_MEM);
      }
      fgetpos(mbox, &(messages[num_messages - 1].header));
      messages[num_messages - 1].size = 0;
      messages[num_messages - 1].deleted = 0;
    } else
      messages[num_messages - 1].size += strlen(buf) + 1;

    if (new_line && buf[0] == '\n') empty_line = 1;
    else empty_line = 0;

    if (strchr(buf, '\n')) new_line = 1;
    else new_line = 0;

  }

  return OK;
}

/* Default signal handler to call the pop3_abquit() function */

void
pop3_signal(int signal)
{
  /* See note about SIGCHLD in vm-pop3d.c */

  syslog(LOG_ERR, "Quitting on signal: %d", signal);
  pop3_abquit(ERR_SIGNAL);
}

/* Gets a line of input from the client */

char *
pop3_readline(int fd)
{
  fd_set rfds;
  struct timeval tv;
  char buf[1024], *ret = NULL;
  int available;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  memset(buf, '\0', 1024);

  while (strchr(buf, '\n') == NULL) {
    if (timeout > 0) {
      available = select(fd + 1, &rfds, NULL, NULL, &tv);
      if (!available)
	pop3_abquit(ERR_TIMEOUT);
    }
    if (read(fd, buf, 1024) < 1)
      pop3_abquit(ERR_DEAD_SOCK);

    if (ret == NULL) {
      ret = malloc((strlen(buf) + 1) * sizeof(char));
      strcpy(ret, buf);
    } else {
      ret = realloc(ret, (strlen(ret) + strlen(buf) + 1) * sizeof(char));
      strcat(ret, buf);
    }
  }
  return ret;
}
