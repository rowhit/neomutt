/**
 * @file
 * Parse and identify different URL schemes
 *
 * @authors
 * Copyright (C) 2000-2002,2004 Thomas Roessler <roessler@does-not-exist.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * A simple URL parser.
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "mutt.h"
#include "url.h"
#include "envelope.h"
#include "globals.h"
#include "lib/lib.h"
#include "mapping.h"
#include "mime.h"
#include "protos.h"
#include "rfc2047.h"
#include "rfc822.h"

static const struct Mapping UrlMap[] = {
  { "file", U_FILE },       { "imap", U_IMAP },     { "imaps", U_IMAPS },
  { "pop", U_POP },         { "pops", U_POPS },     { "news", U_NNTP },
  { "snews", U_NNTPS },     { "mailto", U_MAILTO },
#ifdef USE_NOTMUCH
  { "notmuch", U_NOTMUCH },
#endif
  { "smtp", U_SMTP },       { "smtps", U_SMTPS },   { NULL, U_UNKNOWN },
};

int url_pct_decode(char *s)
{
  char *d = NULL;

  if (!s)
    return -1;

  for (d = s; *s; s++)
  {
    if (*s == '%')
    {
      if (s[1] && s[2] && isxdigit((unsigned char) s[1]) &&
          isxdigit((unsigned char) s[2]) && hexval(s[1]) >= 0 && hexval(s[2]) >= 0)
      {
        *d++ = (hexval(s[1]) << 4) | (hexval(s[2]));
        s += 2;
      }
      else
        return -1;
    }
    else
      *d++ = *s;
  }
  *d = '\0';
  return 0;
}

enum UrlScheme url_check_scheme(const char *s)
{
  char sbuf[STRING];
  char *t = NULL;
  int i;

  if (!s || !(t = strchr(s, ':')))
    return U_UNKNOWN;
  if ((size_t)(t - s) >= sizeof(sbuf) - 1)
    return U_UNKNOWN;

  strfcpy(sbuf, s, t - s + 1);
  for (t = sbuf; *t; t++)
    *t = tolower(*t);

  if ((i = mutt_getvaluebyname(sbuf, UrlMap)) == -1)
    return U_UNKNOWN;
  else
    return (enum UrlScheme) i;
}

/**
 * parse_userhost - fill in components of Url with info from src
 *
 * Note: These are pointers into src, which is altered with '\0's.
 *       Port of 0 means no port given.
 */
static int parse_userhost(struct Url *u, char *src)
{
  char *t = NULL, *p = NULL;

  u->user = NULL;
  u->pass = NULL;
  u->host = NULL;
  u->port = 0;

  if (strncmp(src, "//", 2) != 0)
  {
    u->path = src;
    return url_pct_decode(u->path);
  }

  src += 2;

  if ((u->path = strchr(src, '/')))
    *u->path++ = '\0';

  if ((t = strrchr(src, '@')))
  {
    *t = '\0';
    if ((p = strchr(src, ':')))
    {
      *p = '\0';
      u->pass = p + 1;
      if (url_pct_decode(u->pass) < 0)
        return -1;
    }
    u->user = src;
    if (url_pct_decode(u->user) < 0)
      return -1;
    src = t + 1;
  }

  /* IPv6 literal address.  It may contain colons, so set t to start
   * the port scan after it.
   */
  if ((*src == '[') && (t = strchr(src, ']')))
  {
    src++;
    *t++ = '\0';
  }
  else
    t = src;

  if ((p = strchr(t, ':')))
  {
    int num;
    *p++ = '\0';
    if (mutt_atoi(p, &num) < 0 || num < 0 || num > 0xffff)
      return -1;
    u->port = (unsigned short) num;
  }
  else
    u->port = 0;

  u->host = src;
  return url_pct_decode(u->host) >= 0 && (!u->path || url_pct_decode(u->path) >= 0) ? 0 : -1;
}

/**
 * url_parse - Fill in Url
 *
 * char* elements are pointers into src, which is modified by this call
 * (duplicate it first if you need to).
 */
int url_parse(struct Url *u, char *src)
{
  char *tmp = NULL;

  if ((u->scheme = url_check_scheme(src)) == U_UNKNOWN)
    return -1;

  tmp = strchr(src, ':') + 1;

  return parse_userhost(u, tmp);
}

void url_pct_encode(char *dst, size_t l, const char *src)
{
  static const char *alph = "0123456789ABCDEF";

  *dst = 0;
  l--;
  while (src && *src && l)
  {
    if (strchr("/:&%", *src) && l > 3)
    {
      *dst++ = '%';
      *dst++ = alph[(*src >> 4) & 0xf];
      *dst++ = alph[*src & 0xf];
      src++;
      continue;
    }
    *dst++ = *src++;
  }
  *dst = 0;
}

/**
 * url_tostring - output the URL string for a given Url object
 */
int url_tostring(struct Url *u, char *dest, size_t len, int flags)
{
  long l;

  if (u->scheme == U_UNKNOWN)
    return -1;

  snprintf(dest, len, "%s:", mutt_getnamebyvalue(u->scheme, UrlMap));

  if (u->host)
  {
    if (!(flags & U_PATH))
      safe_strcat(dest, len, "//");
    len -= (l = strlen(dest));
    dest += l;

    if (u->user && (u->user[0] || !(flags & U_PATH)))
    {
      char str[STRING];
      url_pct_encode(str, sizeof(str), u->user);

      if (flags & U_DECODE_PASSWD && u->pass)
      {
        char p[STRING];
        url_pct_encode(p, sizeof(p), u->pass);
        snprintf(dest, len, "%s:%s@", str, p);
      }
      else
        snprintf(dest, len, "%s@", str);

      len -= (l = strlen(dest));
      dest += l;
    }

    if (strchr(u->host, ':'))
      snprintf(dest, len, "[%s]", u->host);
    else
      snprintf(dest, len, "%s", u->host);

    len -= (l = strlen(dest));
    dest += l;

    if (u->port)
      snprintf(dest, len, ":%hu/", u->port);
    else
      snprintf(dest, len, "/");
  }

  if (u->path)
    safe_strcat(dest, len, u->path);

  return 0;
}

int url_parse_mailto(struct Envelope *e, char **body, const char *src)
{
  char *t = NULL, *p = NULL;
  char *tmp = NULL;
  char *headers = NULL;
  char *tag = NULL, *value = NULL;

  int rc = -1;

  if (!(t = strchr(src, ':')))
    return -1;

  /* copy string for safe use of strtok() */
  if ((tmp = safe_strdup(t + 1)) == NULL)
    return -1;

  if ((headers = strchr(tmp, '?')))
    *headers++ = '\0';

  if (url_pct_decode(tmp) < 0)
    goto out;

  e->to = rfc822_parse_adrlist(e->to, tmp);

  tag = headers ? strtok_r(headers, "&", &p) : NULL;

  for (; tag; tag = strtok_r(NULL, "&", &p))
  {
    if ((value = strchr(tag, '=')))
      *value++ = '\0';
    if (!value || !*value)
      continue;

    if (url_pct_decode(tag) < 0)
      goto out;
    if (url_pct_decode(value) < 0)
      goto out;

    /* Determine if this header field is on the allowed list.  Since Mutt
     * interprets some header fields specially (such as
     * "Attach: ~/.gnupg/secring.gpg"), care must be taken to ensure that
     * only safe fields are allowed.
     *
     * RFC2368, "4. Unsafe headers"
     * The user agent interpreting a mailto URL SHOULD choose not to create
     * a message if any of the headers are considered dangerous; it may also
     * choose to create a message with only a subset of the headers given in
     * the URL.
     */
    if (mutt_list_match(tag, &MailToAllow))
    {
      if (mutt_strcasecmp(tag, "body") == 0)
      {
        if (body)
          mutt_str_replace(body, value);
      }
      else
      {
        char *scratch = NULL;
        size_t taglen = mutt_strlen(tag);

        safe_asprintf(&scratch, "%s: %s", tag, value);
        scratch[taglen] = 0; /* overwrite the colon as mutt_parse_rfc822_line expects */
        value = skip_email_wsp(&scratch[taglen + 1]);
        mutt_parse_rfc822_line(e, NULL, scratch, value, 1, 0, 1);
        FREE(&scratch);
      }
    }
  }

  /* RFC2047 decode after the RFC822 parsing */
  rfc2047_decode_adrlist(e->from);
  rfc2047_decode_adrlist(e->to);
  rfc2047_decode_adrlist(e->cc);
  rfc2047_decode_adrlist(e->bcc);
  rfc2047_decode_adrlist(e->reply_to);
  rfc2047_decode_adrlist(e->mail_followup_to);
  rfc2047_decode_adrlist(e->return_path);
  rfc2047_decode_adrlist(e->sender);
  rfc2047_decode(&e->x_label);
  rfc2047_decode(&e->subject);

  rc = 0;

out:
  FREE(&tmp);
  return rc;
}
