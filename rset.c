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

/* Resets the connection so that no messages are marked as deleted */

int
pop3_rset(const char *arg)
{
  if (strlen(arg) != 0)
    return ERR_BAD_ARGS;
  if (state != TRANSACTION)
    return ERR_WRONG_STATE;
  if (cursor > 0) {
    int i;

    cursor = 0;
    for (i = 0; i < num_messages; i++)
      messages[i].deleted = 0;
  }
  fprintf(ofile, "+OK\r\n");
  return OK;
}
