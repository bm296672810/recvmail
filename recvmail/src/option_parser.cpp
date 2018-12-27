#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "../config.h" //this should be conditional for win builds.
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <libetpan/libetpan.h>

#include "option_parser.h"

/*
options

--driver (pop3|imap|nntp|mbox|mh|maildir)  -d

default driver is mbox

--server {server-name} -s
--port {port-number}   -p
--tls                  -t
--starttls             -x
--user {login}         -u
--password {password}  -v
--path {mailbox}       -l
--apop                 -a
--cache {directory}    -c
--flags {directory}    -f
*/

struct storage_name {
    int id;
    const char * name;
};

static struct storage_name storage_tab[] = {
    { POP3_STORAGE, "pop3" },
{ IMAP_STORAGE, "imap" },
{ NNTP_STORAGE, "nntp" },
{ MBOX_STORAGE, "mbox" },
{ MH_STORAGE, "mh" },
{ MAILDIR_STORAGE, "maildir" },
{ FEED_STORAGE, "feed" },
};

static int get_driver(char * name)
{
    int driver_type;
    unsigned int i;

    driver_type = -1;
    for (i = 0; i < sizeof(storage_tab) / sizeof(struct storage_name); i++) {
        if (strcasecmp(name, storage_tab[i].name) == 0) {
            driver_type = i;
            break;
        }
    }

    return driver_type;
}

int init_storage(struct mailstorage * storage,
    int driver, const char * server, int port,
    int connection_type, const char * user, const char * password, int auth_type, bool xoauth2,
    const char * path, const char * cache_directory, const char * flags_directory)
{
    int r;
    int cached;

    cached = (cache_directory != NULL);

    switch (driver) {
    case POP3_STORAGE:
        r = pop3_mailstorage_init(storage, server, port, NULL, connection_type,
            auth_type, user, password, cached, cache_directory,
            flags_directory);
        if (r != MAIL_NO_ERROR) {
            printf("error initializing POP3 storage\n");
            goto err;
        }
        break;

    case IMAP_STORAGE:
        if (xoauth2) {
            r = imap_mailstorage_init_sasl(storage, server, port, NULL, connection_type,
                "xoauth2", NULL, NULL, NULL, NULL, user, password, NULL, cached, cache_directory);
        }
        else {
            r = imap_mailstorage_init(storage, server, port, NULL, connection_type,
                IMAP_AUTH_TYPE_PLAIN, user, password, cached, cache_directory);
        }
        if (r != MAIL_NO_ERROR) {
            printf("error initializing IMAP storage\n");
            goto err;
        }
        break;

    case NNTP_STORAGE:
        r = nntp_mailstorage_init(storage, server, port, NULL, connection_type,
            NNTP_AUTH_TYPE_PLAIN, user, password, cached, cache_directory,
            flags_directory);
        if (r != MAIL_NO_ERROR) {
            printf("error initializing NNTP storage\n");
            goto err;
        }
        break;

    case MBOX_STORAGE:
        r = mbox_mailstorage_init(storage, path, cached, cache_directory,
            flags_directory);
        if (r != MAIL_NO_ERROR) {
            printf("error initializing mbox storage\n");
            goto err;
        }
        break;

    case MH_STORAGE:
        r = mh_mailstorage_init(storage, path, cached, cache_directory,
            flags_directory);
        if (r != MAIL_NO_ERROR) {
            printf("error initializing MH storage\n");
            goto err;
        }
        break;
    case MAILDIR_STORAGE:
        r = maildir_mailstorage_init(storage, path, cached, cache_directory,
            flags_directory);
        if (r != MAIL_NO_ERROR) {
            printf("error initializing maildir storage\n");
            goto err;
        }
        break;
    case FEED_STORAGE:
        r = feed_mailstorage_init(storage, path, cached, cache_directory,
            flags_directory);
        if (r != MAIL_NO_ERROR) {
            printf("error initializing feed storage\n");
            goto err;
        }
        break;
    }

    return MAIL_NO_ERROR;

err:
    return r;
}