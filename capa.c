/* virtualmail-pop3d - a POP3 server with virtual domains support
   Copyright (C) 1999, 2000, 2001 Jeremy C. Reed
   This code is licensed under the GPL; it has several authors.
   vm-pop3d is based on:
   GNU POP3 - a small, fast, and efficient POP3 daemon

 * Code for capa support by Sean 'Shaleh' Perry <shaleh@debian.org>
 * added 4/2/1999
 *
 */
#include "vm-pop3d.h"

int
pop3_capa(const char *arg)
{
  if (strlen(arg) != 0)
    return ERR_BAD_ARGS;

  if (state != AUTHORIZATION && state != TRANSACTION)
    return ERR_WRONG_STATE;

  fprintf(ofile, "+OK Capability list follows\r\n");
  fprintf(ofile, "TOP\r\n");
  fprintf(ofile, "USER\r\n");
  fprintf(ofile, "RESP-CODES\r\n");
  fprintf(ofile, "UIDL\r\n");
  if (state == TRANSACTION)	/* let's not advertise to just anyone */
    fprintf(ofile, "IMPLEMENTATION %s %s\r\n", IMPL, VERSION);
  fprintf(ofile, ".\r\n");
  return OK;
}
