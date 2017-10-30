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

/* Enters the UPDATE phase and deletes marked messages */

int
pop3_quit(const char *arg)
{
  int i = 0, size = 0, write = 0;
  int new_line = 1, empty_line = 1;
  char buf[80];
  fpos_t *readpos = NULL, *writepos = NULL;

  if (strlen(arg) != 0)
    return ERR_BAD_ARGS;

  if (state == TRANSACTION) {
    readpos = malloc(sizeof(fpos_t));
    writepos = malloc(sizeof(fpos_t));
    if (readpos == NULL || writepos == NULL)
      pop3_abquit(ERR_NO_MEM);
    if (fseek(mbox, 0L, SEEK_SET)) return ERR_FILE;
    fgetpos(mbox, writepos);

#ifdef DEBUG
        if (debug > 3)
          syslog (LOG_INFO, "Total messages = %d Messages to delete = %d",
                  num_messages, cursor);
#endif

    if ((cursor > 0) && (num_messages > 0)) {
      if (num_messages == cursor) {
        ftruncate(fileno(mbox), 0); /* quickly delete everything */
#ifdef DEBUG
        if (debug > 3)
          syslog (LOG_INFO, "Deleted all messages");
#endif
      }
      else {
        while (fgets(buf, 80, mbox)) {
	  if (empty_line && !strncmp(buf, "From ", 5)) {
	    if (pop3_mesg_exist(i) == OK)
	      write = 1;
  	    else
	      write = 0;
	    i++;
          }
	  if (write) {
            fgetpos(mbox, readpos);
	    fsetpos(mbox, writepos);
	    fprintf(mbox, "%s", buf);
	    size += strlen(buf);
	    fgetpos(mbox, writepos);
	    fsetpos(mbox, readpos);
	  }
          /* "From " only separates messages if followed by blank line */
          if (new_line && buf[0] == '\n') empty_line = 1;
          else empty_line = 0;
          if (strchr(buf, '\n')) new_line = 1;
          else new_line = 0;
        }
        ftruncate(fileno(mbox), size);
      }
    }
    pop3_unlock();
    fclose(mbox);
    free(messages);
    free(readpos);
    free(writepos);
    syslog(LOG_INFO, "Session ended for user: %s", username);
  } else
    syslog(LOG_INFO, "Session ended for no user");

  state = UPDATE;
  free(username);
  free(mailbox);
  free(md5shared);
  fprintf(ofile, "+OK\r\n");
  return OK;
}
