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

#ifdef USE_PAM
#define COPY_STRING(s) (s) ? strdup(s) : NULL

static char *_pwd;
static char *_user;
static int _perr = 0;

static int
PAM_vmpop3d_conv(int num_msg, const struct pam_message ** msg, struct pam_response ** resp, void *appdata_ptr)
{
  int replies = 0;
  struct pam_response *reply = NULL;

  reply = (struct pam_response *) malloc(sizeof(struct pam_response) * num_msg);
  if (!reply)
    return PAM_CONV_ERR;

  for (replies = 0; replies < num_msg; replies++) {
    switch (msg[replies]->msg_style) {
    case PAM_PROMPT_ECHO_ON:
      reply[replies].resp_retcode = PAM_SUCCESS;
      reply[replies].resp = COPY_STRING(_user);
      /* PAM frees resp */
      break;
    case PAM_PROMPT_ECHO_OFF:
      reply[replies].resp_retcode = PAM_SUCCESS;
      reply[replies].resp = COPY_STRING(_pwd);
      /* PAM frees resp */
      break;
    case PAM_TEXT_INFO:
    case PAM_ERROR_MSG:
      reply[replies].resp_retcode = PAM_SUCCESS;
      reply[replies].resp = NULL;
      break;
    default:
      free(reply);
      _perr = 1;
      return PAM_CONV_ERR;
    }
  }
  *resp = reply;
  return PAM_SUCCESS;
}

static struct pam_conv PAM_conversation =
{&PAM_vmpop3d_conv, NULL};

#endif				/* USE_PAM */

/* Basic user authentication. This also takes the PASS command and verifies
   the user name and password. Calls setuid() upon successful verification,
   otherwise it will (likely) return ERR_BAD_LOGIN */


/* TODO: pop3_user() is too long -- break it up */

#ifdef USE_VIRTUAL
int
pop3_user(const char *arg, const char *domainname)
#else
int
pop3_user(const char *arg)
#endif
{
  char *buf, pass[POP_MAXCMDLEN], *tmp, *cmd;
  int rc;

#ifdef USE_PAM
  pam_handle_t *pamh;
  int pamerror;

#endif				/* !USE_PAM */
  struct passwd *pw = NULL;

  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if ((strlen(arg) == 0) || (strchr(arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  fprintf(ofile, "+OK\r\n");
  fflush(ofile);

  buf = pop3_readline(ifile);
  cmd = pop3_cmd(buf);
  tmp = pop3_args(buf);
  free(buf);

  if (strlen(tmp) > POP_MAXCMDLEN) {
    free(tmp);
    return ERR_TOO_LONG;
  } else {
    strncpy(pass, tmp, POP_MAXCMDLEN);
    free(tmp);
  }

  if (strlen(cmd) > 4) {
    free(cmd);
    return ERR_BAD_CMD;
  }
  if ((strcasecmp(cmd, "PASS") == 0)) {
    free(cmd);

#if 0
    /* Check to see if they have an APOP password. If so, refuse USER/PASS */
    tmp = pop3_apopuser(arg);
    if (tmp != NULL) {
      syslog(LOG_INFO, "APOP user %s tried to log in with USER", arg);
      free(tmp);
      return ERR_BAD_LOGIN;
    }
    free(tmp);
#endif				/* APOP support */

#ifdef USE_VIRTUAL
/* uses regular crypt and not PAM for virtual domains authentication */

/* syslog (LOG_INFO, "user %s AT %s trying to log in", arg, domainname); */

    if (domainname) {
      pw = getvirtualpwnam(arg, domainname);

      if (pw == NULL) {
#ifdef DEBUG
       if (debug > 3)
         syslog (LOG_INFO, "Non-existent user: %s (%s)", arg, domainname);
#endif
	return ERR_BAD_LOGIN;
      }
      if (pw->pw_uid < 1)
	return ERR_BAD_LOGIN;
      if (strcmp(pw->pw_passwd, crypt(pass, pw->pw_passwd)))
	return ERR_BAD_LOGIN;
    } else {			/* no domain part so revert to normal */
#endif				/* USE_VIRTUAL */
      pw = getpwnam(arg);

#ifndef USE_PAM
      if (pw == NULL) {
#ifdef DEBUG
       if (debug > 3)
         syslog (LOG_INFO, "Non-existent user: %s", arg);
#endif
	return ERR_BAD_LOGIN;
      }
      if (pw->pw_uid < 1)
	return ERR_BAD_LOGIN;
      if (strcmp(pw->pw_passwd, crypt(pass, pw->pw_passwd))) {
#ifdef HAVE_SHADOW_H
	struct spwd *spw;

	spw = getspnam(arg);
	if (spw == NULL)
	  return ERR_BAD_LOGIN;
	if (strcmp(spw->sp_pwdp, crypt(pass, spw->sp_pwdp)))
#endif				/* HAVE_SHADOW_H */
	  return ERR_BAD_LOGIN;
      }
#else				/* USE_PAM */
      _user = (char *) arg;
      _pwd = pass;
      /* libpam doesn't log to LOG_MAIL */
      closelog();
      pamerror = pam_start("passwd", arg, &PAM_conversation, &pamh);
      if (!_perr && (pamerror == PAM_SUCCESS)) {
        pamerror = pam_authenticate(pamh, 0);
        if (!_perr && (pamerror == PAM_SUCCESS)) {
          pamerror = pam_acct_mgmt(pamh, 0);
          if (!_perr && (pamerror == PAM_SUCCESS))
            pamerror = pam_setcred(pamh, PAM_ESTABLISH_CRED);
        }
      }
      pam_end(pamh, PAM_SUCCESS);
      openlog("vm-pop3d", LOG_PID, LOG_MAIL);
      if (_perr || (pamerror != PAM_SUCCESS)) {
#ifdef DEBUG
        if (debug > 3)
          syslog (LOG_INFO, "pam: %s", pam_strerror(pamh, pamerror));
#endif
        return ERR_BAD_LOGIN; 
      }
#endif				/* USE_PAM */

#ifdef USE_VIRTUAL
    }				/* has no domain part */
#endif				/* USE_VIRTUAL */

    if (pw != NULL && pw->pw_uid > 1)
      if (setuid(pw->pw_uid) == -1) {
        syslog (LOG_ERR, "setuid not set: uid %d", pw->pw_uid);
        return ERR_BAD_LOGIN;
      }

#ifdef DEBUG
    if (debug > 5)
      syslog (LOG_INFO, "uid %d, gid %d", getuid(), getgid());
#endif

    mbox = NULL;

#ifdef MAILSPOOLHOME
#ifdef USE_VIRTUAL
    /* at this time no virtual users can use MAILSPOOLHOME */
    if (!domainname) {
#endif				/* USE_VIRTUAL */

      /* TODO: need to check for pw_dir before even wanting to use it */
      mailbox = malloc((strlen(pw->pw_dir) + strlen(MAILSPOOLHOME) + 1)
		       * sizeof(char));
      if (mailbox == NULL)
	pop3_abquit(ERR_NO_MEM);
      strcpy(mailbox, pw->pw_dir);
      strcat(mailbox, MAILSPOOLHOME);

      mbox = fopen(mailbox, "r+");
      if (mbox == NULL) {
	chdir(_PATH_MAILDIR);
	free(mailbox);
      }
#ifdef USE_VIRTUAL
    }
#endif				/* USE_VIRTUAL */
#endif				/* MAILSPOOLHOME */

#ifdef USE_VIRTUAL
    /* if it has a domainname, it also can't be using MAILSPOOLHOME */
    if (domainname) {
      mailbox = malloc((strlen(arg) +
       strlen(VIRTUAL_MAILPATH) + strlen(domainname) + 2) * sizeof(char));
      if (mailbox == NULL)
	pop3_abquit(ERR_NO_MEM);
      strcpy(mailbox, VIRTUAL_MAILPATH);
      strcat(mailbox, "/");
      strcat(mailbox, domainname);
      strcat(mailbox, "/");
      chdir(mailbox);
      free(mailbox);
    }
#endif
    if (mbox == NULL) {
      mailbox = strdup(arg);	/* arg is the username */
      if (mailbox == NULL)
	pop3_abquit(ERR_NO_MEM);

      mbox = fopen(mailbox, "r+");
    }
    if (mbox == NULL) {
      mailbox = malloc(strlen("/dev/null") + 1);
      if (mailbox == NULL)
	pop3_abquit(ERR_NO_MEM);
      strcpy(mailbox, "/dev/null");
      mbox = fopen(mailbox, "r+");
      /* TODO: should this assume /dev/null works? */
      syslog(LOG_ERR, "Unable to use %s's mailbox; using %s for testing",
	     arg, mailbox);
    } else if ( (rc = pop3_lock()) != OK ) {
	pop3_unlock();
	state = AUTHORIZATION;
	return rc;
    }
    username = strdup(arg);
    if (username == NULL)
      pop3_abquit(ERR_NO_MEM);

    state = TRANSACTION;
    messages = NULL;
    pop3_getsizes();

    fprintf(ofile, "+OK opened %smailbox for %s\r\n",
	mbox_writable ? "" : "shared ", username);
#ifdef USE_VIRTUAL
    if (domainname)
      syslog(LOG_INFO, "User '%s' of '%s' logged in", username, domainname);
    else
#endif
      syslog(LOG_INFO, "User '%s' logged in", username);

    cursor = 0;
    return OK;
  } else if (strcasecmp(cmd, "QUIT") == 0) {
    free(cmd);
    return pop3_quit(pass);
  }
  free(cmd);
  return ERR_BAD_LOGIN;
}
