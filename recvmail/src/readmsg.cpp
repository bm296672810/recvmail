#include "readmsg.h"
#include "../stdafx.h"
/* render message */

static int etpan_render_mime(FILE * f, mailmessage * msg_info,
    struct mailmime * mime)
{
    int r;
    clistiter * cur;
    int col;
    int text;
    int show;
    struct mailmime_single_fields fields;
    int res;

    mailmime_single_fields_init(&fields, mime->mm_mime_fields,
        mime->mm_content_type);

    text = etpan_mime_is_text(mime);

    r = show_part_info(f, &fields, mime->mm_content_type);
    if (r != NO_ERROR) {
        res = r;
        goto err;
    }

    switch (mime->mm_type) {
    case MAILMIME_SINGLE:
        show = 0;
        if (text)
            show = 1;

        if (show) {
            char * data;
            size_t len;
            char * converted;
            size_t converted_len;
            char * source_charset;
            size_t write_len;

            /* viewable part */

            r = etpan_fetch_message(msg_info, mime,
                &fields, &data, &len);
            if (r != NO_ERROR) {
                res = r;
                goto err;
            }

            source_charset = fields.fld_content_charset;
            if (source_charset == NULL) {
                source_charset = new char[50];
                memset(source_charset, 0, 50);
                memcpy(source_charset, DEST_CHARSET, strlen(DEST_CHARSET));
                //source_charset = DEST_CHARSET;
            }
                

            r = charconv_buffer(source_charset, DEST_CHARSET,
                data, len, &converted, &converted_len);
            if (r != MAIL_CHARCONV_NO_ERROR) {

                r = fprintf(f, "[ error converting charset from %s to %s ]\n",
                    source_charset, DEST_CHARSET);
                if (r < 0) {
                    res = ERROR_FILE;
                    goto err;
                }

                write_len = fwrite(data, 1, len, f);
                if (write_len != len) {
                    mailmime_decoded_part_free(data);
                    res = r;
                    goto err;
                }
            }
            else {
                write_len = fwrite(converted, 1, converted_len, f);
                if (write_len != len) {
                    charconv_buffer_free(converted);
                    mailmime_decoded_part_free(data);
                    res = r;
                    goto err;
                }

                charconv_buffer_free(converted);
            }

            write_len = fwrite("\r\n\r\n", 1, 4, f);
            if (write_len < 4) {
                mailmime_decoded_part_free(data);
                res = ERROR_FILE;
                goto err;
            }

            mailmime_decoded_part_free(data);
        }
        else {
            /* not viewable part */

            r = fprintf(f, "   (not shown)\n\n");
            if (r < 0) {
                res = ERROR_FILE;
                goto err;
            }
        }

        break;

    case MAILMIME_MULTIPLE:

        if (strcasecmp(mime->mm_content_type->ct_subtype, "alternative") == 0) {
            struct mailmime * prefered_body;
            int prefered_score;

            /* case of multiple/alternative */

            /*
            we choose the better part,
            alternative preference :

            text/plain => score 3
            text/xxx   => score 2
            other      => score 1
            */

            prefered_body = NULL;
            prefered_score = 0;

            for (cur = clist_begin(mime->mm_data.mm_multipart.mm_mp_list);
                cur != NULL; cur = clist_next(cur)) {
                struct mailmime * submime;
                int score;

                score = 1;
                submime = (struct mailmime *)clist_content(cur);
                if (etpan_mime_is_text(submime))
                    score = 2;

                if (submime->mm_content_type != NULL) {
                    if (strcasecmp(submime->mm_content_type->ct_subtype, "plain") == 0)
                        score = 3;
                }

                if (score > prefered_score) {
                    prefered_score = score;
                    prefered_body = submime;
                }
            }

            if (prefered_body != NULL) {
                r = etpan_render_mime(f, msg_info, prefered_body);
                if (r != NO_ERROR) {
                    res = r;
                    goto err;
                }
            }
        }
        else {
            for (cur = clist_begin(mime->mm_data.mm_multipart.mm_mp_list);
                cur != NULL; cur = clist_next(cur)) {

                r = etpan_render_mime(f, msg_info, (struct mailmime *)clist_content(cur));
                if (r != NO_ERROR) {
                    res = r;
                    goto err;
                }
            }
        }

        break;

    case MAILMIME_MESSAGE:

        if (mime->mm_data.mm_message.mm_fields != NULL) {
            struct mailimf_fields * msg_fields;

            if (msg_info != NULL) {
                msg_fields = fetch_fields(msg_info, mime);
                if (msg_fields == NULL) {
                    res = ERROR_FETCH;
                    goto err;
                }

                col = 0;
                r = fields_write(f, &col, msg_fields);
                if (r != NO_ERROR) {
                    mailimf_fields_free(msg_fields);
                    res = r;
                    goto err;
                }

                mailimf_fields_free(msg_fields);
            }
            else {
                col = 0;
                r = fields_write(f, &col, mime->mm_data.mm_message.mm_fields);
                if (r != NO_ERROR) {
                    res = r;
                    goto err;
                }
            }

            r = fprintf(f, "\r\n");
            if (r < 0) {
                res = ERROR_FILE;
                goto err;
            }
        }

        if (mime->mm_data.mm_message.mm_msg_mime != NULL) {
            r = etpan_render_mime(f, msg_info, mime->mm_data.mm_message.mm_msg_mime);
            if (r != NO_ERROR) {
                res = r;
                goto err;
            }
        }

        break;
    }

    return NO_ERROR;

err:
    return res;
}

int init_session(struct mailstorage * storage, struct mailfolder ** folder,
    int driver, const char * server, int port,
    int connection_type, const char * user, const char * password, int auth_type, bool xoauth2,
    const char * path, const char * cache_directory, const char * flags_directory)
{
    int r;
    if (storage != NULL /*|| *folder != NULL*/) {
        return (int)Error_Code::init_error;
    }

    storage = mailstorage_new(NULL);
    if (storage == NULL)
    {
        printf("error initializing storage\n");
        return (int)Error_Code::storage_init_err;
    }

    r = init_storage(storage, driver, server, port, connection_type,
        user, password, auth_type, xoauth2, path, cache_directory, flags_directory);
    if (r != MAIL_NO_ERROR) {
        printf("error initializing storage\n");
        return r;
    }

    if (driver == POP3_STORAGE)
    {
        
    }
    switch (driver)
    {
    case POP3_STORAGE:
        *folder = mailfolder_new(storage, path, NULL);
        if (*folder == NULL) {
            printf("mailfolder_new error initializing folder\n");
            return (int)Error_Code::folder_init_err;
        }

        r = mailfolder_connect(*folder);
        if (r != MAIL_NO_ERROR) {
            printf("mailfolder_connect error initializing folder\n");
            return r;
        }
        break;
    case IMAP_STORAGE:
        r = storage->sto_driver->sto_connect(storage);
        if (r)
        {
            printf("connect erron:%d\n", r);
            return r;
        }
        break;
    }

    return r;
}

int get_mail(FILE * f, struct mailfolder * folder)
{
    int r;
    mailmessage * msg;
    uint32_t msg_num = 2;
    struct mailmime * mime;

    if (folder == NULL) {
        printf("folder is invalide !\n");
        return Error_Code::uninit_para;
    }

    r = mailsession_get_message(folder->fld_session, msg_num, &msg);
    if (r != MAIL_NO_ERROR) {
        printf("** message %i not found ** - %s\n", msg_num,
            maildriver_strerror(r));
        return r;
    }

    r = mailmessage_get_bodystructure(msg, &mime);
    if (r != MAIL_NO_ERROR) {
        printf("** message %i not found - %s **\n", msg_num,
            maildriver_strerror(r));
        mailmessage_free(msg);

        return r;
    }

    r = etpan_render_mime(f, msg, mime);

    mailmessage_free(msg);

    return r;
}

int get_mail(FILE * f,
    int driver, const char * server, int port,
    int connection_type, const char * user, const char * password, int auth_type, bool xoauth2,
    const char * path, const char * cache_directory, const char * flags_directory)
{
    int r;
    struct mailstorage * storage = NULL;
    struct mailfolder * folder = NULL;

    r = init_session(storage, &folder, driver, server, port,
        connection_type, user, password, auth_type, xoauth2,
        path, cache_directory, flags_directory);

    if (r)
    {
        printf("init_session erron:%d\n", r);
        return r;
    }

    if(driver == POP3_STORAGE)
        r = get_mail(f, folder);

    if (driver == IMAP_STORAGE)
    {
        //r = mailsession_connect_path(storage->sto_session, path);
        //if (r)
        //    return r;

        mailmessage_list * result;
        mailsession_get_messages_list(storage->sto_session, &result);


        carray * list = result->msg_tab;
        for (size_t i = 0; i < carray_count(list); i++)
        {
            mailimap_message_data* msg = (mailimap_message_data*)carray_get(list, i);
            printf("msg:%d\n", msg->mdt_number);
        }
    }
    return r;
}

int get_mail_imap(const char * server, int port, const char * user, const char * password,
    const char * path, const char * cache_directory, const char * flags_directory)
{
    struct mailstorage * storage = NULL;

    int r = init_session(storage, NULL, IMAP_STORAGE, server, port, CONNECTION_TYPE_PLAIN,
        user, password, IMAP_AUTH_TYPE_PLAIN, false, path, cache_directory, flags_directory);

    if (r)
        return r;
    
    r = mailsession_connect_path(storage->sto_session, path);
    if (r)
        return r;

    mailmessage_list * result;
    mailsession_get_messages_list(storage->sto_session, &result);

    
    carray * list = result->msg_tab;
    for (size_t i = 0; i < carray_count(list); i++)
    {
        mailimap_message_data* msg = (mailimap_message_data*)carray_get(list, i);
        printf("msg:%d\n", msg->mdt_number);
    }
}
