// recvmail.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "src/readmsg.h"
#include "libetpan/mmapstring.h"
#include "src/imap.h"

int main()
{
    int r = 0;
    /*int r = get_mail(stdout, IMAP_STORAGE, "mail.eims.com.cn", 143, CONNECTION_TYPE_PLAIN, "wangyafeng02@eims.com.cn", "228827",
        IMAP_AUTH_TYPE_PLAIN, false, NULL, "mailcache", "mailflags");*/
    //mailimap * imap = mailimap_new(0, NULL);
    //mailimap_set_timeout(imap, 10);
    ////mailimap_set_progress_callback(imap, body_progress, IMAPSession::items_progress, this);
    ////mailimap_set_logger(mImap, logger, this);

    //r = mailimap_socket_connect_voip(imap, "mail.eims.com.cn", 143, true);
    //printf("mailimap_socket_connect_voip:%d\n", r);

    //mailstream_low * low;
    //low = mailstream_get_low(imap->imap_stream);
    //char identifier[100];
    //sprintf_s(identifier, "wangyafeng02@eims.com.cn@mail.eims.com.cn:%d", 143);
    //r = mailstream_low_set_identifier(low, identifier);
    //printf("mailstream_low_set_identifier errno:%d\n", r);

    //r = mailimap_login(imap, "wangyafeng02@eims.com.cn", "228827");

    //printf("mailimap_login errno:%d\n", r);
    //r = mailimap_create(imap, "test_1215");
    //printf("mailimap_create errno:%d\n", r);
    string server = "mail.eims.com.cn";
    string userid = "wangyafeng02@eims.com.cn";
    string password = "228827";

    mailImap imap(server, 143, userid, password);
    //imap.connect();
    string data;
    r = imap.getMessageByUid("INBOX", 3199, data);
    if (r == ErrorNone)
    {
        cout << "getMessageByUid:"<< data << endl;
    }
    else
    {
        cout << "getMessageByUid errno:" << r << endl;
    }
    //MMAPString* str = mmap_string_new("hello");
    //mmap_string_append(str, "1");
    //mmap_string_ref(str);
    //int r = get_mail_imap("mail.eims.com.cn", 143, "wangyafeng02@eims.com.cn", "228827", "mails", "mailcache", "mailflags");
    system("pause");
    return 0;
}

