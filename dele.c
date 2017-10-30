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

/* DELE adds a message number to the list of messages to be deleted on QUIT */

int
pop3_dele(const char *arg)
{
  int num = 0;

  if ((arg == NULL) || (strchr(arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION || !mbox_writable)
    return ERR_WRONG_STATE;

  num = atoi(arg) - 1;
  if (pop3_mesg_exist(num) == OK) {
    cursor++;
    messages[num].deleted = 1;
  } else
    return ERR_NO_MESG;

  fprintf(ofile, "+OK Message %d marked\r\n", num + 1);
  return OK;
}
