/**
 * Copyright (C) 1996-2002,2010,2013 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2004 g10 Code GmbH
 *
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

#ifndef _MUTT_H
#define _MUTT_H 1

#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "charset.h"
#include "hash.h"
#include "rfc822.h"
#ifndef _POSIX_PATH_MAX
#include <limits.h>
#endif

/* On OS X 10.5.x, wide char functions are inlined by default breaking
 * --without-wc-funcs compilation
 */
#ifdef __APPLE_CC__
#define _DONT_USE_CTYPE_INLINE_
#endif

#include <wchar.h>
#ifdef HAVE_WC_FUNCS
#include <wctype.h>
#endif

/* PATH_MAX is undefined on the hurd */
#if !defined(PATH_MAX) && defined(_POSIX_PATH_MAX)
#define PATH_MAX _POSIX_PATH_MAX
#endif

#ifndef HAVE_WC_FUNCS
#ifdef MB_LEN_MAX
#undef MB_LEN_MAX
#endif
#define MB_LEN_MAX 16
#endif

#ifdef HAVE_FGETS_UNLOCKED
#define fgets fgets_unlocked
#endif

#ifdef HAVE_FGETC_UNLOCKED
#define fgetc fgetc_unlocked
#endif

/* nifty trick I stole from ELM 2.5alpha. */
#ifdef MAIN_C
#define WHERE
#define INITVAL(x) = x
#else
#define WHERE extern
#define INITVAL(x)
#endif

#include "buffer.h"
#include "mutt_regex.h"

/* flags for mutt_enter_string() */
#define MUTT_ALIAS 1            /* do alias "completion" by calling up the alias-menu */
#define MUTT_FILE     (1 << 1)  /* do file completion */
#define MUTT_EFILE    (1 << 2)  /* do file completion, plus incoming folders */
#define MUTT_CMD      (1 << 3)  /* do completion on previous word */
#define MUTT_PASS     (1 << 4)  /* password mode (no echo) */
#define MUTT_CLEAR    (1 << 5)  /* clear input if printable character is pressed */
#define MUTT_COMMAND  (1 << 6)  /* do command completion */
#define MUTT_PATTERN  (1 << 7)  /* pattern mode - only used for history classes */
#define MUTT_LABEL    (1 << 8)  /* do label completion */
#ifdef USE_NOTMUCH
#define MUTT_NM_QUERY (1 << 9)  /* Notmuch query mode. */
#define MUTT_NM_TAG   (1 << 10) /* Notmuch tag +/- mode. */
#endif

/* flags for _mutt_system() */
#define MUTT_DETACH_PROCESS 1 /* detach subprocess from group */

/* flags for mutt_FormatString() */
typedef enum {
  MUTT_FORMAT_FORCESUBJ   = (1 << 0), /* print the subject even if unchanged */
  MUTT_FORMAT_TREE        = (1 << 1), /* draw the thread tree */
  MUTT_FORMAT_MAKEPRINT   = (1 << 2), /* make sure that all chars are printable */
  MUTT_FORMAT_OPTIONAL    = (1 << 3),
  MUTT_FORMAT_STAT_FILE   = (1 << 4), /* used by mutt_attach_fmt */
  MUTT_FORMAT_ARROWCURSOR = (1 << 5), /* reserve space for arrow_cursor */
  MUTT_FORMAT_INDEX       = (1 << 6), /* this is a main index entry */
  MUTT_FORMAT_NOFILTER    = (1 << 7)  /* do not allow filtering on this pass */
} format_flag;

/* types for mutt_add_hook() */
#define MUTT_FOLDERHOOK   (1 << 0)
#define MUTT_MBOXHOOK     (1 << 1)
#define MUTT_SENDHOOK     (1 << 2)
#define MUTT_FCCHOOK      (1 << 3)
#define MUTT_SAVEHOOK     (1 << 4)
#define MUTT_CHARSETHOOK  (1 << 5)
#define MUTT_ICONVHOOK    (1 << 6)
#define MUTT_MESSAGEHOOK  (1 << 7)
#define MUTT_CRYPTHOOK    (1 << 8)
#define MUTT_ACCOUNTHOOK  (1 << 9)
#define MUTT_REPLYHOOK    (1 << 10)
#define MUTT_SEND2HOOK    (1 << 11)
#ifdef USE_COMPRESSED
#define MUTT_OPENHOOK     (1 << 12)
#define MUTT_APPENDHOOK   (1 << 13)
#define MUTT_CLOSEHOOK    (1 << 14)
#endif
#define MUTT_TIMEOUTHOOK  (1 << 15)
#define MUTT_STARTUPHOOK  (1 << 16)
#define MUTT_SHUTDOWNHOOK (1 << 17)
#define MUTT_GLOBALHOOK   (1 << 18)

/* tree characters for linearize_tree and print_enriched_string */
#define MUTT_TREE_LLCORNER 1
#define MUTT_TREE_ULCORNER 2
#define MUTT_TREE_LTEE     3
#define MUTT_TREE_HLINE    4
#define MUTT_TREE_VLINE    5
#define MUTT_TREE_SPACE    6
#define MUTT_TREE_RARROW   7
#define MUTT_TREE_STAR     8
#define MUTT_TREE_HIDDEN   9
#define MUTT_TREE_EQUALS   10
#define MUTT_TREE_TTEE     11
#define MUTT_TREE_BTEE     12
#define MUTT_TREE_MISSING  13
#define MUTT_TREE_MAX      14

#define MUTT_SPECIAL_INDEX MUTT_TREE_MAX

#define MUTT_THREAD_COLLAPSE    (1 << 0)
#define MUTT_THREAD_UNCOLLAPSE  (1 << 1)
#define MUTT_THREAD_GET_HIDDEN  (1 << 2)
#define MUTT_THREAD_UNREAD      (1 << 3)
#define MUTT_THREAD_NEXT_UNREAD (1 << 4)

enum
{
  /* modes for mutt_view_attachment() */
  MUTT_REGULAR = 1,
  MUTT_MAILCAP,
  MUTT_AS_TEXT,

  /* action codes used by mutt_set_flag() and mutt_pattern_function() */
  MUTT_ALL,
  MUTT_NONE,
  MUTT_NEW,
  MUTT_OLD,
  MUTT_REPLIED,
  MUTT_READ,
  MUTT_UNREAD,
  MUTT_DELETE,
  MUTT_UNDELETE,
  MUTT_PURGE,
  MUTT_DELETED,
  MUTT_FLAG,
  MUTT_TAG,
  MUTT_UNTAG,
  MUTT_LIMIT,
  MUTT_EXPIRED,
  MUTT_SUPERSEDED,
  MUTT_TRASH,

  /* actions for mutt_pattern_comp/mutt_pattern_exec */
  MUTT_AND,
  MUTT_OR,
  MUTT_THREAD,
  MUTT_TO,
  MUTT_CC,
  MUTT_COLLAPSED,
  MUTT_SUBJECT,
  MUTT_FROM,
  MUTT_DATE,
  MUTT_DATE_RECEIVED,
  MUTT_DUPLICATED,
  MUTT_UNREFERENCED,
  MUTT_ID,
  MUTT_BODY,
  MUTT_HEADER,
  MUTT_HORMEL,
  MUTT_WHOLE_MSG,
  MUTT_SENDER,
  MUTT_MESSAGE,
  MUTT_SCORE,
  MUTT_SIZE,
  MUTT_REFERENCE,
  MUTT_RECIPIENT,
  MUTT_LIST,
  MUTT_SUBSCRIBED_LIST,
  MUTT_PERSONAL_RECIP,
  MUTT_PERSONAL_FROM,
  MUTT_ADDRESS,
  MUTT_CRYPT_SIGN,
  MUTT_CRYPT_VERIFIED,
  MUTT_CRYPT_ENCRYPT,
  MUTT_PGP_KEY,
  MUTT_XLABEL,
#ifdef USE_NOTMUCH
  MUTT_NOTMUCH_LABEL,
#endif
  MUTT_MIMEATTACH,
#ifdef USE_NNTP
  MUTT_NEWSGROUPS,
#endif

  /* Options for Mailcap lookup */
  MUTT_EDIT,
  MUTT_COMPOSE,
  MUTT_PRINT,
  MUTT_AUTOVIEW,

  /* options for socket code */
  MUTT_NEW_SOCKET,
#ifdef USE_SSL_OPENSSL
  MUTT_NEW_SSL_SOCKET,
#endif

  /* Options for mutt_save_attachment */
  MUTT_SAVE_APPEND,
  MUTT_SAVE_OVERWRITE
};

/* possible arguments to set_quadoption() */
enum
{
  MUTT_ABORT = -1,
  MUTT_NO,
  MUTT_YES,
  MUTT_ASKNO,
  MUTT_ASKYES
};

/* quad-option vars */
enum
{
  OPT_ABORT,
  OPT_BOUNCE,
  OPT_COPY,
  OPT_DELETE,
  OPT_FORWEDIT,
  OPT_FCCATTACH,
  OPT_INCLUDE,
  OPT_MFUPTO,
  OPT_MIMEFWD,
  OPT_MIMEFWDREST,
  OPT_MOVE,
  OPT_PGPMIMEAUTO, /* ask to revert to PGP/MIME when inline fails */
  OPT_SMIMEENCRYPTSELF,
  OPT_PGPENCRYPTSELF,
#ifdef USE_POP
  OPT_POPDELETE,
  OPT_POPRECONNECT,
#endif
  OPT_POSTPONE,
  OPT_PRINT,
  OPT_QUIT,
  OPT_REPLYTO,
  OPT_RECALL,
#ifdef USE_SSL
  OPT_SSLSTARTTLS,
#endif
  OPT_SUBJECT,
  OPT_VERIFYSIG, /* verify PGP signatures */
#ifdef USE_NNTP
  OPT_TOMODERATED,
  OPT_CATCHUP,
  OPT_FOLLOWUPTOPOSTER,
#endif
  OPT_ATTACH, /* forgotten attachment detector */
  /* THIS MUST BE THE LAST VALUE. */
  OPT_MAX
};

/* flags to ci_send_message() */
#define SENDREPLY        (1 << 0)
#define SENDGROUPREPLY   (1 << 1)
#define SENDLISTREPLY    (1 << 2)
#define SENDFORWARD      (1 << 3)
#define SENDPOSTPONED    (1 << 4)
#define SENDBATCH        (1 << 5)
#define SENDMAILX        (1 << 6)
#define SENDKEY          (1 << 7)
#define SENDRESEND       (1 << 8)
#define SENDPOSTPONEDFCC (1 << 9)  /* used by mutt_get_postponed() to signal that the x-mutt-fcc header field was present */
#define SENDNOFREEHEADER (1 << 10) /* Used by the -E flag */
#define SENDDRAFTFILE    (1 << 11) /* Used by the -H flag */
#define SENDNEWS         (1 << 12)

/* flags for mutt_compose_menu() */
#define MUTT_COMPOSE_NOFREEHEADER (1 << 0)

/* flags to _mutt_select_file() */
#define MUTT_SEL_BUFFY   (1 << 0)
#define MUTT_SEL_MULTI   (1 << 1)
#define MUTT_SEL_FOLDER  (1 << 2)
#define MUTT_SEL_VFOLDER (1 << 3)

/* flags for parse_spam_list */
#define MUTT_SPAM   1
#define MUTT_NOSPAM 2

/* flags for keywords headers */
#define MUTT_X_LABEL        (1 << 0) /* introduced to mutt in 2000 */
#define MUTT_X_KEYWORDS     (1 << 1) /* used in c-client, dovecot */
#define MUTT_X_MOZILLA_KEYS (1 << 2) /* tbird */
#define MUTT_KEYWORDS       (1 << 3) /* rfc2822 */

/* boolean vars */
enum
{
  OPTALLOW8BIT,
  OPTALLOWANSI,
  OPTARROWCURSOR,
  OPTASCIICHARS,
  OPTASKBCC,
  OPTASKCC,
  OPTASKFOLLOWUP,
  OPTASKXCOMMENTTO,
  OPTATTACHSPLIT,
  OPTAUTOEDIT,
  OPTAUTOTAG,
  OPTBEEP,
  OPTBEEPNEW,
  OPTBOUNCEDELIVERED,
  OPTBRAILLEFRIENDLY,
  OPTCHECKMBOXSIZE,
  OPTCHECKNEW,
  OPTCOLLAPSEALL,
  OPTCOLLAPSEUNREAD,
  OPTCONFIRMAPPEND,
  OPTCONFIRMCREATE,
  OPTDELETEUNTAG,
  OPTDIGESTCOLLAPSE,
  OPTDUPTHREADS,
  OPTEDITHDRS,
  OPTENCODEFROM,
  OPTENVFROM,
  OPTFASTREPLY,
  OPTFCCCLEAR,
  OPTFLAGSAFE,
  OPTFOLLOWUPTO,
  OPTFORCENAME,
  OPTFORWDECODE,
  OPTFORWQUOTE,
  OPTFORWREF,
#ifdef USE_HCACHE
  OPTHCACHEVERIFY,
#if defined(HAVE_QDBM) || defined(HAVE_TC) || defined(HAVE_KC)
  OPTHCACHECOMPRESS,
#endif /* HAVE_QDBM */
#endif
  OPTHDRS,
  OPTHEADER,
  OPTHEADERCOLORPARTIAL,
  OPTHELP,
  OPTHIDDENHOST,
  OPTHIDELIMITED,
  OPTHIDEMISSING,
  OPTHIDETHREADSUBJECT,
  OPTHIDETOPLIMITED,
  OPTHIDETOPMISSING,
  OPTHONORDISP,
  OPTIGNORELWS,
  OPTIGNORELISTREPLYTO,
#ifdef USE_IMAP
  OPTIMAPCHECKSUBSCRIBED,
  OPTIMAPIDLE,
  OPTIMAPLSUB,
  OPTIMAPPASSIVE,
  OPTIMAPPEEK,
  OPTIMAPSERVERNOISE,
#endif
#ifdef USE_SSL
#ifndef USE_SSL_GNUTLS
  OPTSSLSYSTEMCERTS,
  OPTSSLV2,
#endif /* USE_SSL_GNUTLS */
  OPTSSLV3,
  OPTTLSV1,
  OPTTLSV1_1,
  OPTTLSV1_2,
  OPTSSLFORCETLS,
  OPTSSLVERIFYDATES,
  OPTSSLVERIFYHOST,
#if defined(USE_SSL_OPENSSL) && defined(HAVE_SSL_PARTIAL_CHAIN)
  OPTSSLVERIFYPARTIAL,
#endif /* USE_SSL_OPENSSL */
#endif /* defined(USE_SSL) */
  OPTIMPLICITAUTOVIEW,
  OPTINCLUDEONLYFIRST,
  OPTKEEPFLAGGED,
  OPTKEYWORDSLEGACY,
  OPTKEYWORDSSTANDARD,
  OPTMAILCAPSANITIZE,
  OPTMAILCHECKRECENT,
  OPTMAILCHECKSTATS,
  OPTMAILDIRTRASH,
  OPTMAILDIRCHECKCUR,
  OPTMARKERS,
  OPTMARKOLD,
  OPTMENUSCROLL,  /* scroll menu instead of implicit next-page */
  OPTMENUMOVEOFF, /* allow menu to scroll past last entry */
#if defined(USE_IMAP) || defined(USE_POP)
  OPTMESSAGECACHECLEAN,
#endif
  OPTMETAKEY, /* interpret ALT-x as ESC-x */
  OPTMETOO,
  OPTMHPURGE,
  OPTMIMEFORWDECODE,
#ifdef USE_NNTP
  OPTMIMESUBJECT, /* encode subject line with RFC2047 */
#endif
  OPTNARROWTREE,
  OPTPAGERSTOP,
  OPTPIPEDECODE,
  OPTPIPESPLIT,
#ifdef USE_POP
  OPTPOPAUTHTRYALL,
  OPTPOPLAST,
#endif
  OPTPOSTPONEENCRYPT,
  OPTPRINTDECODE,
  OPTPRINTSPLIT,
  OPTPROMPTAFTER,
  OPTREADONLY,
  OPTREFLOWSPACEQUOTES,
  OPTREFLOWTEXT,
  OPTREPLYSELF,
  OPTREPLYWITHXORIG,
  OPTRESOLVE,
  OPTRESUMEDRAFTFILES,
  OPTRESUMEEDITEDDRAFTFILES,
  OPTREVALIAS,
  OPTREVNAME,
  OPTREVREAL,
  OPTRFC2047PARAMS,
  OPTSAVEADDRESS,
  OPTSAVEEMPTY,
  OPTSAVENAME,
  OPTSCORE,
#ifdef USE_SIDEBAR
  OPTSIDEBAR,
  OPTSIDEBARFOLDERINDENT,
  OPTSIDEBARNEWMAILONLY,
  OPTSIDEBARNEXTNEWWRAP,
  OPTSIDEBARSHORTPATH,
  OPTSIDEBARONRIGHT,
#endif
  OPTSIGDASHES,
  OPTSIGONTOP,
  OPTSORTRE,
  OPTSPAMSEP,
  OPTSTATUSONTOP,
  OPTSTRICTTHREADS,
  OPTSUSPEND,
  OPTTEXTFLOWED,
  OPTTHOROUGHSRC,
  OPTTHREADRECEIVED,
  OPTTILDE,
  OPTTSENABLED,
  OPTUNCOLLAPSEJUMP,
  OPTUNCOLLAPSENEW,
  OPTUSE8BITMIME,
  OPTUSEDOMAIN,
  OPTUSEFROM,
  OPTUSEGPGAGENT,
#ifdef HAVE_LIBIDN
  OPTIDNDECODE,
  OPTIDNENCODE,
#endif
#ifdef HAVE_GETADDRINFO
  OPTUSEIPV6,
#endif
  OPTWAITKEY,
  OPTWEED,
  OPTWRAP,
  OPTWRAPSEARCH,
  OPTWRITEBCC, /* write out a bcc header? */
  OPTXMAILER,

  OPTCRYPTUSEGPGME,
  OPTCRYPTUSEPKA,

  /* PGP options */

  OPTCRYPTAUTOSIGN,
  OPTCRYPTAUTOENCRYPT,
  OPTCRYPTAUTOPGP,
  OPTCRYPTAUTOSMIME,
  OPTCRYPTCONFIRMHOOK,
  OPTCRYPTOPPORTUNISTICENCRYPT,
  OPTCRYPTREPLYENCRYPT,
  OPTCRYPTREPLYSIGN,
  OPTCRYPTREPLYSIGNENCRYPTED,
  OPTCRYPTTIMESTAMP,
  OPTSMIMEISDEFAULT,
  OPTASKCERTLABEL,
  OPTSDEFAULTDECRYPTKEY,
  OPTPGPIGNORESUB,
  OPTPGPCHECKEXIT,
  OPTPGPLONGIDS,
  OPTPGPAUTODEC,
  OPTPGPRETAINABLESIG,
  OPTPGPSTRICTENC,
  OPTFORWDECRYPT,
  OPTPGPSHOWUNUSABLE,
  OPTPGPAUTOINLINE,
  OPTPGPREPLYINLINE,

/* news options */

#ifdef USE_NNTP
  OPTSHOWNEWNEWS,
  OPTSHOWONLYUNREAD,
  OPTSAVEUNSUB,
  OPTLISTGROUP,
  OPTLOADDESC,
  OPTXCOMMENTTO,
#endif

  /* pseudo options */

  OPTAUXSORT,           /* (pseudo) using auxiliary sort function */
  OPTFORCEREFRESH,      /* (pseudo) refresh even during macros */
  OPTLOCALES,           /* (pseudo) set if user has valid locale definition */
  OPTNOCURSES,          /* (pseudo) when sending in batch mode */
  OPTSEARCHREVERSE,     /* (pseudo) used by ci_search_command */
  OPTMSGERR,            /* (pseudo) used by mutt_error/mutt_message */
  OPTSEARCHINVALID,     /* (pseudo) used to invalidate the search pat */
  OPTSIGNALSBLOCKED,    /* (pseudo) using by mutt_block_signals () */
  OPTSYSSIGNALSBLOCKED, /* (pseudo) using by mutt_block_signals_system () */
  OPTNEEDRESORT,        /* (pseudo) used to force a re-sort */
  OPTRESORTINIT,        /* (pseudo) used to force the next resort to be from scratch */
  OPTVIEWATTACH,        /* (pseudo) signals that we are viewing attachments */
  OPTSORTSUBTHREADS,    /* (pseudo) used when $sort_aux changes */
  OPTNEEDRESCORE,       /* (pseudo) set when the `score' command is used */
  OPTATTACHMSG,         /* (pseudo) used by attach-message */
  OPTHIDEREAD,          /* (pseudo) whether or not hide read messages */
  OPTKEEPQUIET,         /* (pseudo) shut up the message and refresh
                         *          functions while we are executing an
                         *          external program.
                         */
  OPTMENUCALLER,        /* (pseudo) tell menu to give caller a take */
  OPTREDRAWTREE,        /* (pseudo) redraw the thread tree */
  OPTPGPCHECKTRUST,     /* (pseudo) used by pgp_select_key () */
  OPTDONTHANDLEPGPKEYS, /* (pseudo) used to extract PGP keys */
  OPTIGNOREMACROEVENTS, /* (pseudo) don't process macro/push/exec events while set */

#ifdef USE_NNTP
  OPTNEWS,              /* (pseudo) used to change reader mode */
  OPTNEWSSEND,          /* (pseudo) used to change behavior when posting */
#endif
#ifdef USE_NOTMUCH
  OPTVIRTSPOOLFILE,
  OPTNOTMUCHRECORD,
#endif

  OPTMAX
};

#define mutt_bit_alloc(n)     calloc((n + 7) / 8, sizeof(char))
#define mutt_bit_set(v, n)    v[n / 8] |= (1 << (n % 8))
#define mutt_bit_unset(v, n)  v[n / 8] &= ~(1 << (n % 8))
#define mutt_bit_toggle(v, n) v[n / 8] ^= (1 << (n % 8))
#define mutt_bit_isset(v, n)  (v[n / 8] & (1 << (n % 8)))

#define set_option(x) mutt_bit_set(Options, x)
#define unset_option(x) mutt_bit_unset(Options, x)
#define toggle_option(x) mutt_bit_toggle(Options, x)
#define option(x) mutt_bit_isset(Options, x)

typedef struct list_t
{
  char *data;
  struct list_t *next;
} LIST;

typedef struct rx_list_t
{
  REGEXP *rx;
  struct rx_list_t *next;
} RX_LIST;

typedef struct replace_list_t
{
  REGEXP *rx;
  int nmatch;
  char *template;
  struct replace_list_t *next;
} REPLACE_LIST;

static inline LIST *mutt_new_list(void)
{
  return safe_calloc(1, sizeof(LIST));
}

void mutt_free_list(LIST **list);
void mutt_free_rx_list(RX_LIST **list);
void mutt_free_replace_list(REPLACE_LIST **list);
LIST *mutt_copy_list(LIST *p);
int mutt_matches_ignore(const char *s);
bool mutt_matches_list(const char *s, LIST *t);

/* add an element to a list */
LIST *mutt_add_list(LIST *head, const char *data);
LIST *mutt_add_list_n(LIST *head, const void *data, size_t len);
LIST *mutt_find_list(LIST *l, const char *data);
int mutt_remove_from_rx_list(RX_LIST **l, const char *str);

/* handle stack */
void mutt_push_list(LIST **head, const char *data);
bool mutt_pop_list(LIST **head);
const char *mutt_front_list(LIST *head);

void mutt_init(int skip_sys_rc, LIST *commands);

struct Alias
{
  struct Alias *self; /* XXX - ugly hack */
  char *name;
  struct Address *addr;
  struct Alias *next;
  bool tagged;
  bool del;
  short num;
};

struct Envelope
{
  struct Address *return_path;
  struct Address *from;
  struct Address *to;
  struct Address *cc;
  struct Address *bcc;
  struct Address *sender;
  struct Address *reply_to;
  struct Address *mail_followup_to;
  struct Address *x_original_to;
  char *list_post; /* this stores a mailto URL, or nothing */
  char *subject;
  char *real_subj; /* offset of the real subject */
  char *disp_subj; /* display subject (modified copy of subject) */
  char *message_id;
  char *supersedes;
  char *date;
  char *x_label;
  char *organization;
#ifdef USE_NNTP
  char *newsgroups;
  char *xref;
  char *followup_to;
  char *x_comment_to;
#endif
  struct Buffer *spam;
  LIST *references;  /* message references (in reverse order) */
  LIST *in_reply_to; /* in-reply-to header content */
  LIST *userhdrs;    /* user defined headers */
  int kwtypes;

  bool irt_changed : 1;  /* In-Reply-To changed to link/break threads */
  bool refs_changed : 1; /* References changed to break thread */
};

static inline struct Envelope *mutt_new_envelope(void)
{
  return safe_calloc(1, sizeof(struct Envelope));
}

typedef struct parameter
{
  char *attribute;
  char *value;
  struct parameter *next;
} PARAMETER;

static inline PARAMETER *mutt_new_parameter(void)
{
  return safe_calloc(1, sizeof(PARAMETER));
}

/* Information that helps in determining the Content-* of an attachment */
struct Content
{
  long hibin;      /* 8-bit characters */
  long lobin;      /* unprintable 7-bit chars (eg., control chars) */
  long crlf;       /* '\r' and '\n' characters */
  long ascii;      /* number of ascii chars */
  long linemax;    /* length of the longest line in the file */
  bool space : 1;  /* whitespace at the end of lines? */
  bool binary : 1; /* long lines, or CR not in CRLF pair */
  bool from : 1;   /* has a line beginning with "From "? */
  bool dot : 1;    /* has a line consisting of a single dot? */
  bool cr : 1;     /* has CR, even when in a CRLF pair */
};

struct Body
{
  char *xtype;          /* content-type if x-unknown */
  char *subtype;        /* content-type subtype */
  PARAMETER *parameter; /* parameters of the content-type */
  char *description;    /* content-description */
  char *form_name;      /* Content-Disposition form-data name param */
  long hdr_offset;      /* offset in stream where the headers begin.
                         * this info is used when invoking metamail,
                         * where we need to send the headers of the
                         * attachment
                         */
  LOFF_T offset;        /* offset where the actual data begins */
  LOFF_T length;        /* length (in bytes) of attachment */
  char *filename;       /* when sending a message, this is the file
                         * to which this structure refers
                         */
  char *d_filename;     /* filename to be used for the
                         * content-disposition header.
                         * If NULL, filename is used
                         * instead.
                         */
  char *charset;        /* charset of attached file */
  struct Content *content;     /* structure used to store detailed info about
                         * the content of the attachment.  this is used
                         * to determine what content-transfer-encoding
                         * is required when sending mail.
                         */
  struct Body *next;    /* next attachment in the list */
  struct Body *parts;   /* parts of a multipart or message/rfc822 */
  struct Header *hdr;   /* header information for message/rfc822 */

  struct AttachPtr *aptr; /* Menu information, used in recvattach.c */

  signed short attach_count;

  time_t stamp; /* time stamp of last
                 * encoding update.
                 */

  unsigned int type : 4;        /* content-type primary type */
  unsigned int encoding : 3;    /* content-transfer-encoding */
  unsigned int disposition : 2; /* content-disposition */
  bool use_disp : 1;            /* Content-Disposition uses filename= ? */
  bool unlink : 1;              /* flag to indicate the file named by
                                 * "filename" should be unlink()ed before
                                 * free()ing this structure
                                 */
  bool tagged : 1;
  bool deleted : 1; /* attachment marked for deletion */

  bool noconv : 1; /* don't do character set conversion */
  bool force_charset : 1;
  /* send mode: don't adjust the character
   * set when in send-mode.
   */
  bool is_signed_data : 1; /* A lot of MUAs don't indicate
                                      S/MIME signed-data correctly,
                                      e.g. they use foo.p7m even for
                                      the name of signed data.  This
                                      flag is used to keep track of
                                      the actual message type.  It
                                      gets set during the verification
                                      (which is done if the encryption
                                      try failed) and check by the
                                      function to figure the type of
                                      the message. */

  bool goodsig : 1; /* good cryptographic signature */
  bool warnsig : 1; /* maybe good signature */
  bool badsig : 1; /* bad cryptographic signature (needed to check encrypted s/mime-signatures) */

  bool collapsed : 1; /* used by recvattach */
  bool attach_qualifies : 1;

};

/* #3279: AIX defines conflicting struct thread */
typedef struct mutt_thread THREAD;

struct Header
{
  unsigned int security : 12; /* bit 0-8: flags, bit 9,10: application.
                                 see: mutt_crypt.h pgplib.h, smime.h */

  bool mime            : 1; /* has a MIME-Version header? */
  bool flagged         : 1; /* marked important? */
  bool tagged          : 1;
  bool deleted         : 1;
  bool purge           : 1; /* skip trash folder when deleting */
  bool quasi_deleted   : 1; /* deleted from mutt, but not modified on disk */
  bool changed         : 1;
  bool attach_del      : 1; /* has an attachment marked for deletion */
  bool old             : 1;
  bool read            : 1;
  bool expired         : 1; /* already expired? */
  bool superseded      : 1; /* got superseded? */
  bool replied         : 1;
  bool subject_changed : 1; /* used for threading */
  bool threaded        : 1; /* used for threading */
  bool display_subject : 1; /* used for threading */
  bool recip_valid     : 1; /* is_recipient is valid */
  bool active          : 1; /* message is not to be removed */
  bool trash           : 1; /* message is marked as trashed on disk.
                             * This flag is used by the maildir_trash
                             * option.
                             */
  bool xlabel_changed  : 1; /* editable - used for syncing */

  /* timezone of the sender of this message */
  unsigned int zhours : 5;
  unsigned int zminutes : 6;
  bool zoccident : 1;

  /* bits used for caching when searching */
  bool searched : 1;
  bool matched : 1;

  /* tells whether the attachment count is valid */
  bool attach_valid : 1;

  /* the following are used to support collapsing threads  */
  bool collapsed : 1; /* is this message part of a collapsed thread? */
  bool limited : 1;   /* is this message in a limited view?  */
  size_t num_hidden;  /* number of hidden messages in this view */

  short recipient;    /* user_is_recipient()'s return value, cached */

  int pair;           /* color-pair to use when displaying in the index */

  time_t date_sent;   /* time when the message was sent (UTC) */
  time_t received;    /* time when the message was placed in the mailbox */
  LOFF_T offset;      /* where in the stream does this message begin? */
  int lines;          /* how many lines in the body of this message? */
  int index;          /* the absolute (unsorted) message number */
  int msgno;          /* number displayed to the user */
  int virtual;        /* virtual message number */
  int score;
  struct Envelope *env;      /* envelope information */
  struct Body *content;      /* list of MIME parts */
  char *path;

  char *tree; /* character string to print thread tree */
  THREAD *thread;

  /* Number of qualifying attachments in message, if attach_valid */
  short attach_total;

#ifdef MIXMASTER
  LIST *chain;
#endif

#ifdef USE_POP
  int refno; /* message number on server */
#endif

#if defined(USE_POP) || defined(USE_IMAP) || defined(USE_NNTP) || defined(USE_NOTMUCH)
  void *data;                       /* driver-specific data */
  void (*free_cb)(struct Header *); /* driver-specific data free function */
#endif

  char *maildir_flags; /* unknown maildir flags */
};

static inline struct Header *mutt_new_header(void)
{
  return safe_calloc(1, sizeof(struct Header));
}

struct mutt_thread
{
  bool fake_thread : 1;
  bool duplicate_thread : 1;
  bool sort_children : 1;
  bool check_subject : 1;
  bool visible : 1;
  bool deep : 1;
  unsigned int subtree_visible : 2;
  bool next_subtree_visible : 1;
  THREAD *parent;
  THREAD *child;
  THREAD *next;
  THREAD *prev;
  struct Header *message;
  struct Header *sort_key;
};


/* flag to mutt_pattern_comp() */
#define MUTT_FULL_MSG (1 << 0) /* enable body and header matching */

typedef enum {
  MUTT_MATCH_FULL_ADDRESS = 1
} pattern_exec_flag;

struct Group
{
  struct Address *as;
  RX_LIST *rs;
  char *name;
};

typedef struct group_context_t
{
  struct Group *g;
  struct group_context_t *next;
} group_context_t;

typedef struct pattern_t
{
  short op;
  bool not : 1;
  bool alladdr : 1;
  bool stringmatch : 1;
  bool groupmatch : 1;
  bool ign_case : 1; /* ignore case for local stringmatch searches */
  bool isalias : 1;
  int min;
  int max;
  struct pattern_t *next;
  struct pattern_t *child; /* arguments to logical op */
  union {
    regex_t *rx;
    struct Group *g;
    char *str;
  } p;
} pattern_t;

/* This is used when a message is repeatedly pattern matched against.
 * e.g. for color, scoring, hooks.  It caches a few of the potentially slow
 * operations.
 * Each entry has a value of 0 = unset, 1 = false, 2 = true
 */
typedef struct
{
  int list_all;       /* ^~l */
  int list_one;       /*  ~l */
  int sub_all;        /* ^~u */
  int sub_one;        /*  ~u */
  int pers_recip_all; /* ^~p */
  int pers_recip_one; /*  ~p */
  int pers_from_all;  /* ^~P */
  int pers_from_one;  /*  ~P */
} pattern_cache_t;

/* ACL Rights */
enum
{
  MUTT_ACL_LOOKUP = 0,
  MUTT_ACL_READ,
  MUTT_ACL_SEEN,
  MUTT_ACL_WRITE,
  MUTT_ACL_INSERT,
  MUTT_ACL_POST,
  MUTT_ACL_CREATE,
  MUTT_ACL_DELMX,
  MUTT_ACL_DELETE,
  MUTT_ACL_EXPUNGE,
  MUTT_ACL_ADMIN,

  RIGHTSMAX
};

struct Context;
struct _message;

/*
 * struct mx_ops - a structure to store operations on a mailbox
 * The following operations are mandatory:
 *  - open
 *  - close
 *  - check
 *
 * Optional operations
 *  - open_new_msg
 */
struct mx_ops
{
  int (*open)(struct Context *ctx);
  int (*open_append)(struct Context *ctx, int flags);
  int (*close)(struct Context *ctx);
  int (*check)(struct Context *ctx, int *index_hint);
  int (*sync)(struct Context *ctx, int *index_hint);
  int (*open_msg)(struct Context *ctx, struct _message *msg, int msgno);
  int (*close_msg)(struct Context *ctx, struct _message *msg);
  int (*commit_msg)(struct Context *ctx, struct _message *msg);
  int (*open_new_msg)(struct _message *msg, struct Context *ctx, struct Header *hdr);
};

#include "mutt_menu.h"

struct Context
{
  char *path;
  char *realpath; /* used for buffy comparison and the sidebar */
  FILE *fp;
  time_t atime;
  time_t mtime;
  off_t size;
  off_t vsize;
  char *pattern;            /* limit pattern string */
  pattern_t *limit_pattern; /* compiled limit pattern */
  struct Header **hdrs;
  struct Header *last_tag;  /* last tagged msg. used to link threads */
  THREAD *tree;      /* top of thread tree */
  struct Hash *id_hash;     /* hash table by msg id */
  struct Hash *subj_hash;   /* hash table by subject */
  struct Hash *thread_hash; /* hash table for threading */
  struct Hash *label_hash;  /* hash table for x-labels */
  int *v2r;          /* mapping from virtual to real msgno */
  int hdrmax;        /* number of pointers in hdrs */
  int msgcount;      /* number of messages in the mailbox */
  int vcount;        /* the number of virtual messages */
  int tagged;        /* how many messages are tagged? */
  int new;           /* how many new messages? */
  int unread;        /* how many unread messages? */
  int deleted;       /* how many deleted messages */
  int flagged;       /* how many flagged messages */
  int msgnotreadyet; /* which msg "new" in pager, -1 if none */

  MUTTMENU *menu; /* needed for pattern compilation */

  short magic; /* mailbox type */

  unsigned char rights[(RIGHTSMAX + 7) / 8]; /* ACL bits */

  bool locked : 1;    /* is the mailbox locked? */
  bool changed : 1;   /* mailbox has been modified */
  bool readonly : 1;  /* don't allow changes to the mailbox */
  bool dontwrite : 1; /* don't write the mailbox on close */
  bool append : 1;    /* mailbox is opened in append mode */
  bool quiet : 1;     /* inhibit status messages? */
  bool collapsed : 1; /* are all threads collapsed? */
  bool closing : 1;   /* mailbox is being closed */
  bool peekonly : 1;  /* just taking a glance, revert atime */

#ifdef USE_COMPRESSED
  void *compress_info; /* compressed mbox module private data */
#endif /* USE_COMPRESSED */

  /* driver hooks */
  void *data; /* driver specific data */
  struct mx_ops *mx_ops;
};

typedef struct
{
  FILE *fpin;
  FILE *fpout;
  char *prefix;
  int flags;
} STATE;

/* used by enter.c */

struct EnterState
{
  wchar_t *wbuf;
  size_t wbuflen;
  size_t lastchar;
  size_t curpos;
  size_t begin;
  int tabs;
};

static inline struct EnterState *mutt_new_enter_state(void)
{
  return safe_calloc(1, sizeof(struct EnterState));
}

/* flags for the STATE struct */
#define MUTT_DISPLAY       (1 << 0) /* output is displayed to the user */
#define MUTT_VERIFY        (1 << 1) /* perform signature verification */
#define MUTT_PENDINGPREFIX (1 << 2) /* prefix to write, but character must follow */
#define MUTT_WEED          (1 << 3) /* weed headers even when not in display mode */
#define MUTT_CHARCONV      (1 << 4) /* Do character set conversions */
#define MUTT_PRINTING      (1 << 5) /* are we printing? - MUTT_DISPLAY "light" */
#define MUTT_REPLYING      (1 << 6) /* are we replying? */
#define MUTT_FIRSTDONE     (1 << 7) /* the first attachment has been done */

#define state_set_prefix(s) ((s)->flags |= MUTT_PENDINGPREFIX)
#define state_reset_prefix(s) ((s)->flags &= ~MUTT_PENDINGPREFIX)
#define state_puts(x, y) fputs(x, (y)->fpout)
#define state_putc(x, y) fputc(x, (y)->fpout)

void state_mark_attach(STATE *s);
void state_attach_puts(const char *t, STATE *s);
void state_prefix_putc(char c, STATE *s);
int state_printf(STATE *s, const char *fmt, ...);
int state_putws(const wchar_t *ws, STATE *s);

/* for attachment counter */
typedef struct
{
  char *major;
  int major_int;
  char *minor;
  regex_t minor_rx;
} ATTACH_MATCH;

/* multibyte character table.
 * Allows for direct access to the individual multibyte characters in a
 * string.  This is used for the Flagchars, Fromchars, StChars and Tochars
 * option types. */
typedef struct
{
  int len;             /* number of characters */
  char **chars;        /* the array of multibyte character strings */
  char *segmented_str; /* each chars entry points inside this string */
  char *orig_str;
} mbchar_table;

#define MUTT_PARTS_TOPLEVEL (1 << 0) /* is the top-level part */

#include "ascii.h"
#include "globals.h"
#include "lib.h"
#include "protos.h"

#endif /* _MUTT_H */
