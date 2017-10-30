/* virtualmail-pop3d - a POP3 server with virtual domains support
   uidl.c
   Ideas from Pascal Bouchareine <pb@grolier.fr> and Jeffrey Stedfast.
   Since their contributions were for gnu-pop3d using ideas from
   gnu-pop3d, it is assumed (and partially due to the viral nature
   of GPL) that this code is also GPL'd. */

#include "vm-pop3d.h"

/* UIDL implementation based on SMTP Message-Id: header
   Even though Message-Id's are required to exist and be unique, it is
   probably wrong to assume that this unique-id should be based on it.
   Maybe later, when APOP is enabled, consider using md5digest[16] instead.
   Or do someother hashing to accomplish this.
   Please note that RFC 1939 says the UIDL unique-id consists of
   "one to 70 characters in the range 0x21 to 0x7E"; some Message-Id's
   are longer than that and some contain different characters (like ^M). */

#define UNIQUE_ID_LENGTH 70

int 
uidl(char unique_id[UNIQUE_ID_LENGTH], int mesg)
{
  char buffer[256];
  char *message_id;
  int has_message_id = 0;

  unique_id[0] = '\0';

  fsetpos(mbox, &(messages[mesg].header));

  while (fgets(buffer, 256, mbox)) {
    /* Using case-insensitive because Jason J. Horton <jason@intercom.com>
     * says that some SMTP servers use Message-ID, Message-Id, or
     * Message-id headers. */
    if (!strncasecmp(buffer, "Message-Id:", 11)) {
      has_message_id = 1;	/* appears to have a Message-Id */
      /* it shouldn't matter if it has a newline. */
      break;
    }
    if (strchr(buffer, '\n') == NULL)
      continue;
    if (buffer[0] == '\n')	/* end of mail headers */
      break;
  }

  if (has_message_id) {
    for (message_id = buffer + 11;
	 *message_id && (*message_id != '>'); message_id++);
    *message_id = '\0';
    for (message_id = buffer + 11;
	 *message_id && (*message_id != '<'); message_id++);
    if (*(message_id) == '<') {
      message_id++;
      strncpy(unique_id, message_id, UNIQUE_ID_LENGTH);
      unique_id[UNIQUE_ID_LENGTH] = '\0';
    }
  }
  if (unique_id[0] == '\0') {
    /* need to make a unique id */
    /* This is NOT unique! */
    memset(unique_id, '\0', UNIQUE_ID_LENGTH);
    snprintf(unique_id, UNIQUE_ID_LENGTH, "no-message-id-%u", mesg + 1);
    /* this should really be better than this. Maybe do a hash above. */
  }
  return OK;
}

int
pop3_uidl(const char *arg)
{
  int mesg;
  char unique_id[UNIQUE_ID_LENGTH];

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr(arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  if (strlen(arg) == 0) {
    fprintf(ofile, "+OK\r\n");
    for (mesg = 0; mesg < num_messages; mesg++)
      if (pop3_mesg_exist(mesg) == OK) {
	if (!uidl(unique_id, mesg))
	  fprintf(ofile, "%u %s\r\n", mesg + 1, unique_id);
	else
	  return ERR_FILE;
      }
    fprintf(ofile, ".\r\n");
  } else {			/* respond with only one unique id for
				 * specific message */
    mesg = atoi(arg) - 1;
    if (pop3_mesg_exist(mesg) != OK)
      return ERR_NO_MESG;

    if (!uidl(unique_id, mesg))
      fprintf(ofile, "+OK %u %s\r\n", mesg + 1, unique_id);
    else
      return ERR_FILE;

  }

  return OK;
}
