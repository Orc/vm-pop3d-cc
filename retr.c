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

/* Prints out the specified message */

int
pop3_retr(const char *arg)
{
  int mesg, empty_line = 1, new_line = 1;
  int done = 0;
  char *buf;

  if ((strlen(arg) == 0) || (strchr(arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesg = atoi(arg) - 1;

  if (pop3_mesg_exist(mesg) != OK)
    return ERR_NO_MESG;

  fsetpos(mbox, &(messages[mesg].header));
  buf = malloc(sizeof(char) * 80);
  if (buf == NULL)
    pop3_abquit(ERR_NO_MEM);

  fprintf(ofile, "+OK\r\n");

  while (!done && fgets(buf, 80, mbox)) {
    if (empty_line && !strncmp(buf, "From ", 5))
      done = 1;
    else {
      if (new_line) {
        if (buf[0] == '.') /* only matters at beginning of line */
          fprintf(ofile, ".");
        else if (buf[0] == '\n') empty_line = 1;
          else empty_line = 0;
      }
      if (strchr(buf, '\n')) {
        buf[strlen(buf) - 1] = '\0';
        fprintf(ofile, "%s\r\n", buf);
        new_line = 1;
      }
      else {
        fprintf(ofile, "%s", buf);
        new_line = 0;
        empty_line = 0;
      }
    }
  }

  free(buf);
  fprintf(ofile, ".\r\n");
  return OK;
}
