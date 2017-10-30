/* virtualmail-pop3d - a POP3 server with virtual domains support
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

/* Displays the size of message number arg or all messages (if no arg) */

int
pop3_list(const char *arg)
{
  int mesg = 0;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr(arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  if (strlen(arg) == 0) {
    fprintf(ofile, "+OK\r\n");
    for (mesg = 0; mesg < num_messages; mesg++)
      if (pop3_mesg_exist(mesg) == OK)
	fprintf(ofile, "%d %d\r\n", mesg + 1, messages[mesg].size);
    fprintf(ofile, ".\r\n");
  } else {
    mesg = atoi(arg) - 1;
    if (pop3_mesg_exist(mesg) != OK)
      return ERR_NO_MESG;
    fprintf(ofile, "+OK %d %d\r\n", mesg + 1, messages[mesg].size);
  }

  return OK;
}
