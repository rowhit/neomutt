/**
 * @file
 * Shared constants/structs that are private to IMAP
 *
 * @authors
 * Copyright (C) 1996-1999 Brandon Long <blong@fiction.net>
 * Copyright (C) 1999-2009 Brendan Cully <brendan@kublai.com>
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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

#ifndef MUTT_IMAP_IMAP_PRIVATE_H
#define MUTT_IMAP_IMAP_PRIVATE_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "mutt/mutt.h"
#include "conn/conn.h"
#ifdef USE_HCACHE
#include "hcache/hcache.h"
#endif

struct Context;
struct Email;
struct ImapEmailData;
struct ImapMbox;
struct Mailbox;
struct Message;
struct Progress;

/* -- symbols -- */
#define IMAP_PORT 143
#define IMAP_SSL_PORT 993

/* logging levels */
#define IMAP_LOG_CMD  2
#define IMAP_LOG_LTRL 4
#define IMAP_LOG_PASS 5

/* IMAP command responses. Used in ImapCommand.state too */
#define IMAP_CMD_OK       0  /**< `<tag> OK ...` */
#define IMAP_CMD_BAD      -1 /**< `<tag> BAD ...` */
#define IMAP_CMD_NO       -2 /**< `<tag> NO ...` */
#define IMAP_CMD_CONTINUE 1  /**< `* ...` */
#define IMAP_CMD_RESPOND  2  /**< `+` */
#define IMAP_CMD_NEW      3  /**< ImapCommand.state additions */

/* number of entries in the hash table */
#define IMAP_CACHE_LEN 10

#define SEQLEN 5
/* maximum length of command lines before they must be split (for
 * lazy servers) */
#define IMAP_MAX_CMDLEN 1024

#define IMAP_REOPEN_ALLOW     (1 << 0)
#define IMAP_EXPUNGE_EXPECTED (1 << 1)
#define IMAP_EXPUNGE_PENDING  (1 << 2)
#define IMAP_NEWMAIL_PENDING  (1 << 3)
#define IMAP_FLAGS_PENDING    (1 << 4)

/* imap_exec flags (see imap_exec) */
#define IMAP_CMD_FAIL_OK (1 << 0)
#define IMAP_CMD_PASS    (1 << 1)
#define IMAP_CMD_QUEUE   (1 << 2)
#define IMAP_CMD_POLL    (1 << 3)

/* length of "DD-MMM-YYYY HH:MM:SS +ZZzz" (null-terminated) */
#define IMAP_DATELEN 27

/**
 * enum ImapFlags - IMAP server responses
 */
enum ImapFlags
{
  IMAP_FATAL = 1,
  IMAP_BYE
};

/**
 * enum ImapState - IMAP connection state
 */
enum ImapState
{
  /* States */
  IMAP_DISCONNECTED = 0,
  IMAP_CONNECTED,
  IMAP_AUTHENTICATED,
  IMAP_SELECTED,

  /* and pseudo-states */
  IMAP_IDLE
};

/**
 * enum ImapNamespace - IMAP namespace types
 */
enum ImapNamespace
{
  IMAP_NS_PERSONAL = 0,
  IMAP_NS_OTHER,
  IMAP_NS_SHARED
};

/**
 * enum ImapCaps - Capabilities we are interested in
 *
 * @note This must be kept in the same order as Capabilities.
 */
enum ImapCaps
{
  IMAP4 = 0,
  IMAP4REV1,
  STATUS,
  ACL,                   /**< RFC2086: IMAP4 ACL extension */
  NAMESPACE,             /**< RFC2342: IMAP4 Namespace */
  ACRAM_MD5,             /**< RFC2195: CRAM-MD5 authentication */
  AGSSAPI,               /**< RFC1731: GSSAPI authentication */
  AUTH_ANON,             /**< AUTH=ANONYMOUS */
  AUTH_OAUTHBEARER,      /**< RFC7628: AUTH=OAUTHBEARER */
  STARTTLS,              /**< RFC2595: STARTTLS */
  LOGINDISABLED,         /**< RFC2595: LOGINDISABLED */
  IDLE,                  /**< RFC2177: IDLE */
  SASL_IR,               /**< SASL initial response draft */
  ENABLE,                /**< RFC5161 */
  CONDSTORE,             /**< RFC7162 */
  QRESYNC,               /**< RFC7162 */
  X_GM_EXT1,             /**< https://developers.google.com/gmail/imap/imap-extensions */
  X_GM_ALT1 = X_GM_EXT1, /**< Alternative capability string */

  CAPMAX
};

/* imap_conn_find flags */
#define MUTT_IMAP_CONN_NONEW    (1 << 0)
#define MUTT_IMAP_CONN_NOSELECT (1 << 1)

/**
 * struct ImapCache - IMAP-specific message cache
 */
struct ImapCache
{
  unsigned int uid;
  char *path;
};

/**
 * struct ImapStatus - Status of an IMAP mailbox
 */
struct ImapStatus
{
  char *name;

  unsigned int messages;
  unsigned int recent;
  unsigned int uidnext;
  unsigned int uidvalidity;
  unsigned int unseen;
  unsigned long long modseq;  /* Used by CONDSTORE. 1 <= modseq < 2^63 */
};

/**
 * struct ImapList - Items in an IMAP browser
 */
struct ImapList
{
  char *name;
  char delim;
  bool noselect;
  bool noinferiors;
};

/**
 * struct ImapCommand - IMAP command structure
 */
struct ImapCommand
{
  char seq[SEQLEN + 1]; ///< Command tag, e.g. 'a0001'
  int state;            ///< Command state, e.g. #IMAP_CMD_NEW
};

/**
 * enum ImapCommandType - IMAP command type
 */
enum ImapCommandType
{
  IMAP_CT_NONE = 0,
  IMAP_CT_LIST,
  IMAP_CT_STATUS
};

/**
 * struct ImapAccountData - IMAP-specific server data
 *
 * This data is specific to a Connection to an IMAP server
 */
struct ImapAccountData
{
  struct Connection *conn;
  struct ConnAccount conn_account;
  struct Account *account;
  bool recovering;
  unsigned char state;  ///< ImapState, e.g. #IMAP_AUTHENTICATED
  unsigned char status; ///< ImapFlags, e.g. #IMAP_FATAL
  /* let me explain capstr: SASL needs the capability string (not bits).
   * we have 3 options:
   *   1. rerun CAPABILITY inside SASL function.
   *   2. build appropriate CAPABILITY string by reverse-engineering from bits.
   *   3. keep a copy until after authentication.
   * I've chosen (3) for now. (2) might not be too bad, but it involves
   * tracking all possible capabilities. bah. (1) I don't like because
   * it's just no fun to get the same information twice */
  char *capstr;
  unsigned char capabilities[(CAPMAX + 7) / 8];
  unsigned int seqno; ///< tag sequence number, e.g. 'a0001'
  time_t lastread; /**< last time we read a command for the server */
  char *buf;
  size_t blen;

  bool unicode; /* If true, we can send UTF-8, and the server will use UTF8 rather than mUTF7 */
  bool qresync; /* true, if QRESYNC is successfully ENABLE'd */

  /* if set, the response parser will store results for complicated commands
   * here. */
  enum ImapCommandType cmdtype;
  void *cmddata;

  /* command queue */
  struct ImapCommand *cmds;
  int cmdslots;
  int nextcmd;
  int lastcmd;
  struct Buffer *cmdbuf;

  /* cache ImapStatus of visited mailboxes */
  struct ListHead mboxcache;

  /* The following data is all specific to the currently SELECTED mbox */
  char delim;
  struct Context *ctx;
  struct Mailbox *mailbox;
  char *mbox_name;
  unsigned short check_status; /**< Flags, e.g. #IMAP_NEWMAIL_PENDING */
  unsigned char reopen;        /**< Flags, e.g. #IMAP_REOPEN_ALLOW */
  unsigned int new_mail_count; /**< Set when EXISTS notifies of new mail */
  struct ImapCache cache[IMAP_CACHE_LEN];
  struct Hash *uid_hash;
  unsigned int uid_validity;
  unsigned int uidnext;
  unsigned long long modseq;
  struct Email **msn_index;   /**< look up headers by (MSN-1) */
  size_t msn_index_size;       /**< allocation size */
  unsigned int max_msn;        /**< the largest MSN fetched so far */
  struct BodyCache *bcache;

  /* all folder flags - system AND custom flags */
  struct ListHead flags;
#ifdef USE_HCACHE
  header_cache_t *hcache;
#endif
};

/**
 * struct SeqsetIterator - UID Sequence Set Iterator
 */
struct SeqsetIterator
{
  char *full_seqset;
  char *eostr;
  int in_range;
  int down;
  unsigned int range_cur;
  unsigned int range_end;
  char *substr_cur;
  char *substr_end;
};

/* -- private IMAP functions -- */
/* imap.c */
int imap_check(struct ImapAccountData *adata, bool force);
int imap_create_mailbox(struct ImapAccountData *adata, char *mailbox);
int imap_rename_mailbox(struct ImapAccountData *adata, struct ImapMbox *mx, const char *newname);
struct ImapStatus *imap_mboxcache_get(struct ImapAccountData *adata, const char *mbox, bool create);
void imap_mboxcache_free(struct ImapAccountData *adata);
int imap_exec_msgset(struct ImapAccountData *adata, const char *pre, const char *post,
                     int flag, bool changed, bool invert);
int imap_open_connection(struct ImapAccountData *adata);
void imap_close_connection(struct ImapAccountData *adata);
struct ImapAccountData *imap_conn_find(const struct ConnAccount *account, int flags);
int imap_read_literal(FILE *fp, struct ImapAccountData *adata, unsigned long bytes, struct Progress *pbar);
void imap_expunge_mailbox(struct ImapAccountData *adata);
void imap_logout(struct ImapAccountData **adata);
int imap_sync_message_for_copy(struct ImapAccountData *adata, struct Email *e, struct Buffer *cmd, int *err_continue);
bool imap_has_flag(struct ListHead *flag_list, const char *flag);

/* auth.c */
int imap_authenticate(struct ImapAccountData *adata);

/* command.c */
int imap_cmd_start(struct ImapAccountData *adata, const char *cmdstr);
int imap_cmd_step(struct ImapAccountData *adata);
void imap_cmd_finish(struct ImapAccountData *adata);
bool imap_code(const char *s);
const char *imap_cmd_trailer(struct ImapAccountData *adata);
int imap_exec(struct ImapAccountData *adata, const char *cmdstr, int flags);
int imap_cmd_idle(struct ImapAccountData *adata);

/* message.c */
void imap_edata_free(void **ptr);
struct ImapEmailData *imap_edata_get(struct Email *e);
int imap_read_headers(struct ImapAccountData *adata, unsigned int msn_begin, unsigned int msn_end, bool initial_download);
char *imap_set_flags(struct ImapAccountData *adata, struct Email *e, char *s, int *server_changes);
int imap_cache_del(struct ImapAccountData *adata, struct Email *e);
int imap_cache_clean(struct ImapAccountData *adata);
int imap_append_message(struct Context *ctx, struct Message *msg);

int imap_msg_open(struct Context *ctx, struct Message *msg, int msgno);
int imap_msg_close(struct Context *ctx, struct Message *msg);
int imap_msg_commit(struct Context *ctx, struct Message *msg);

/* util.c */
struct ImapAccountData *imap_adata_get(struct Mailbox *m);
#ifdef USE_HCACHE
header_cache_t *imap_hcache_open(struct ImapAccountData *adata, const char *path);
void imap_hcache_close(struct ImapAccountData *adata);
struct Email *imap_hcache_get(struct ImapAccountData *adata, unsigned int uid);
int imap_hcache_put(struct ImapAccountData *adata, struct Email *e);
int imap_hcache_del(struct ImapAccountData *adata, unsigned int uid);
int imap_hcache_store_uid_seqset(struct ImapAccountData *adata);
int imap_hcache_clear_uid_seqset(struct ImapAccountData *adata);
char *imap_hcache_get_uid_seqset(struct ImapAccountData *adata);
#endif

int imap_continue(const char *msg, const char *resp);
void imap_error(const char *where, const char *msg);
struct ImapAccountData *imap_adata_new(void);
void imap_adata_free(void **ptr);
char *imap_fix_path(struct ImapAccountData *adata, const char *mailbox, char *path, size_t plen);
void imap_cachepath(struct ImapAccountData *adata, const char *mailbox, char *dest, size_t dlen);
int imap_get_literal_count(const char *buf, unsigned int *bytes);
char *imap_get_qualifier(char *buf);
int imap_mxcmp(const char *mx1, const char *mx2);
char *imap_next_word(char *s);
void imap_qualify_path(char *buf, size_t buflen, struct ImapMbox *mx, char *path);
void imap_quote_string(char *dest, size_t dlen, const char *src, bool quote_backtick);
void imap_unquote_string(char *s);
void imap_munge_mbox_name(struct ImapAccountData *adata, char *dest, size_t dlen, const char *src);
void imap_unmunge_mbox_name(struct ImapAccountData *adata, char *s);
struct SeqsetIterator *mutt_seqset_iterator_new(const char *seqset);
int mutt_seqset_iterator_next(struct SeqsetIterator *iter, unsigned int *next);
void mutt_seqset_iterator_free(struct SeqsetIterator **p_iter);
bool imap_account_match(const struct ConnAccount *a1, const struct ConnAccount *a2);
void imap_get_parent(const char *mbox, char delim, char *buf, size_t buflen);

/* utf7.c */
void imap_utf_encode(struct ImapAccountData *adata, char **s);
void imap_utf_decode(struct ImapAccountData *adata, char **s);
void imap_allow_reopen(struct Mailbox *m);
void imap_disallow_reopen(struct Mailbox *m);

#ifdef USE_HCACHE
#define imap_hcache_keylen mutt_str_strlen
#endif /* USE_HCACHE */

#endif /* MUTT_IMAP_IMAP_PRIVATE_H */
