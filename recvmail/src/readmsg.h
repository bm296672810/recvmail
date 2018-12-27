#ifndef __READ_MSG_H__
#define __READ_MSG_H__

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <libetpan/charconv.h>
#include <libetpan/libetpan.h>

#include "option_parser.h"
#include "readmsg_common.h"

struct Error_Code {
    static const short uninit_para = -1;
    static const short init_error = -2;
    static const short storage_init_err = -3;
    static const short folder_init_err = -4;
};

static int etpan_render_mime(FILE * f, mailmessage * msg_info,
    struct mailmime * mime);

int init_session(struct mailstorage * storage, struct mailfolder ** folder,
    int driver, const char * server, int port,
    int connection_type, const char * user, const char * password, int auth_type, bool xoauth2,
    const char * path, const char * cache_directory, const char * flags_directory);

int get_mail(FILE * f, struct mailfolder * folder);
int get_mail(FILE * f, int driver, const char * server, int port,
    int connection_type, const char * user, const char * password, int auth_type, bool xoauth2,
    const char * path, const char * cache_directory, const char * flags_directory);
int get_mail_imap(const char * server, int port, const char * user, const char * password,
    const char * path, const char * cache_directory, const char * flags_directory);

#endif // !__READ_MSG_H__