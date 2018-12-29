#include "imap.h"

static struct {
    const char * name;
    int flag;
} mb_keyword_flag[] = {
    { "Inbox",     IMAPFolderFlagInbox },
{ "AllMail",   IMAPFolderFlagAllMail },
{ "Sent",      IMAPFolderFlagSentMail },
{ "Spam",      IMAPFolderFlagSpam },
{ "Starred",   IMAPFolderFlagStarred },
{ "Trash",     IMAPFolderFlagTrash },
{ "Important", IMAPFolderFlagImportant },
{ "Drafts",    IMAPFolderFlagDrafts },
{ "Archive",   IMAPFolderFlagArchive },
{ "All",       IMAPFolderFlagAll },
{ "Junk",      IMAPFolderFlagJunk },
{ "Flagged",   IMAPFolderFlagFlagged },
};
static string toupper(const string& str)
{
    string result;
    for (size_t i = 0; i < str.size(); i++)
    {
        char t = str.at(i);
        if (t >= 'a' && t <= 'z')
        {
            result.push_back(t - 'a' + 'A');
        }
        else
        {
            result.push_back(t);
        }
    }

    return result;
}
static int imap_mailbox_flags_to_flags(struct mailimap_mbx_list_flags * imap_flags)
{
    int flags;
    clistiter * cur;

    flags = 0;
    if (imap_flags->mbf_type == MAILIMAP_MBX_LIST_FLAGS_SFLAG) {
        switch (imap_flags->mbf_sflag) {
        case MAILIMAP_MBX_LIST_SFLAG_MARKED:
            flags |= IMAPFolderFlagMarked;
            break;
        case MAILIMAP_MBX_LIST_SFLAG_NOSELECT:
            flags |= IMAPFolderFlagNoSelect;
            break;
        case MAILIMAP_MBX_LIST_SFLAG_UNMARKED:
            flags |= IMAPFolderFlagUnmarked;
            break;
        }
    }

    for (cur = clist_begin(imap_flags->mbf_oflags); cur != NULL;
        cur = clist_next(cur)) {
        struct mailimap_mbx_list_oflag * oflag;

        oflag = (struct mailimap_mbx_list_oflag *) clist_content(cur);

        switch (oflag->of_type) {
        case MAILIMAP_MBX_LIST_OFLAG_NOINFERIORS:
            flags |= IMAPFolderFlagNoInferiors;
            break;

        case MAILIMAP_MBX_LIST_OFLAG_FLAG_EXT:
            for (unsigned int i = 0; i < sizeof(mb_keyword_flag) / sizeof(mb_keyword_flag[0]); i++) {
                if (strcasecmp(mb_keyword_flag[i].name, oflag->of_flag_ext) == 0) {
                    flags |= mb_keyword_flag[i].flag;
                }
            }
            break;
        }
    }

    return flags;
}

mailImap::mailImap(const string& server, uint16_t port, const string& userid, const string& pwd)
{
    init();

    setServer(server);
    setPort(port);
    setUserid(userid);
    setPassword(pwd);
}

int mailImap::connect()
{
    if (m_isConnected)
        return 0;

    m_imap = mailimap_new(0, NULL);
    mailimap_set_timeout(m_imap, m_timeout);
    int r = mailimap_socket_connect_voip(m_imap, m_server.c_str(), m_port, m_voipEenable);

    if (r == MAILIMAP_NO_ERROR_NON_AUTHENTICATED)
    {
        printf("mailimap_socket_connect_voip errno:%d\n", r);
        m_isConnected = true;
        m_status = SS_CONNECTED;
        r = ErrorNone;
    }

    return r;
}

int mailImap::login()
{
    if (m_isLogined)
        return 0;

    mailstream_low * low;
    low = mailstream_get_low(m_imap->imap_stream);
    //string identifier = m_userid + "@" + m_server + ":" + port;
    size_t len = m_userid.length() + m_server.length() + 10;
    char* identifier = new char[len];
    sprintf_s(identifier, len, "%s@%s:%d", m_userid.c_str(), m_server.c_str(), m_port);
    int r = mailstream_low_set_identifier(low, identifier);
    printf("mailstream_low_set_identifier errno:%d\n", r);

    r = mailimap_login(m_imap, m_userid.c_str(), m_pwd.c_str());

    if (!r)
    {
        m_status = SS_LOGGEDIN;
        m_isLogined = true;
    }

    return r;
}

void mailImap::init()
{
    m_port = 0;
    m_imap = NULL;

    m_isConnected   = false;
    m_isLogined     = false;
}

void mailImap::setServer(const string& server)
{
    m_server = server;
}
string mailImap::getServer()
{
    return m_server;
}
void mailImap::setPort(uint16_t port)
{
    m_port = port;
}
uint16_t mailImap::getPort() const
{
    return m_port;
}
void mailImap::setUserid(const string& userid)
{
    m_userid = userid;
}
string mailImap::getUserid() const
{
    return m_userid;
}
void mailImap::setPassword(const string& pwd)
{
    m_pwd = pwd;
}
string mailImap::getPassword() const
{
    return m_pwd;
}

int mailImap::getMessageByUid(const string& folder, uint32_t uid, string& data)
{
    return getMessage(folder, true, uid, data);
}

int mailImap::getMessageByNumber(const string& folder, uint32_t num, string& data)
{
    return getMessage(folder, false, num, data);
}

int mailImap::getMessage(const string& folder, bool isUid, uint32_t uidOrNumber, string& data)
{
    int r = selectIfNeeded(folder);
    if (r)
        return r;

    char * rfc822;
    size_t rfc822_len = 0;

    rfc822 = NULL;
    r = fetch_rfc822(m_imap, isUid, uidOrNumber, &rfc822, &rfc822_len);

    if (r == MAILIMAP_NO_ERROR) {
        size_t len;

        len = 0;
        if (rfc822 != NULL) {
            len = strlen(rfc822);
        }
        //bodyProgress((unsigned int)len, (unsigned int)len);
    }

    if (r == MAILIMAP_ERROR_STREAM) {
        //mShouldDisconnect = true;
        return ErrorConnection;
    }
    else if (r == MAILIMAP_ERROR_PARSE) {
        //mShouldDisconnect = true;
        return ErrorParse;
    }
    else if (hasError(r)) {
        return ErrorFetch;
    }

    if (rfc822 != NULL) {
        data = string(rfc822, (unsigned int)rfc822_len);
    }

    mailimap_nstring_free(rfc822);

    return ErrorNone;
}

int mailImap::getMessageAttachmentByUid(const string& folder, uint32_t uid, string& partId, Encoding encoding, string& data)
{
    return getMessageAttachment(folder, true, uid, partId, encoding, data);
}

int mailImap::getMessageAttachment(const string& folder, bool isUid, uint32_t uidOrNumber, string& partId, Encoding encoding, string& data)
{
    int r = getNonDecodedMessageAttachment(folder, isUid, uidOrNumber, partId, true, 0, 0, encoding, data);
    if (r == ErrorNone)
        decodeData(data, encoding);

    return r;
}

int mailImap::getNonDecodedMessageAttachment(const string& folder, bool isUid, uint32_t uidOrNumber, string& partId,
    bool wholePart, uint32_t offset, uint32_t length, Encoding encoding, string& data)
{
    struct mailimap_fetch_type * fetch_type;
    struct mailimap_fetch_att * fetch_att;
    struct mailimap_section * section;
    struct mailimap_section_part * section_part;
    clist * sec_list;
    vector<string> partIDArray;
    //Array * partIDArray;
    int r;
    char * text = NULL;
    size_t text_length = 0;
    //Data * data;
    r = selectIfNeeded(folder);
    if (r)
        return r;

    partIDArray = splictStr(partId, ".");
    sec_list = clist_new();
    for (unsigned int i = 0; i < partIDArray.size(); i++) {
        uint32_t * value;
        string element;

        element = partIDArray.at(i);
        value = (uint32_t *)malloc(sizeof(*value));
        *value = (uint32_t)(atol(element.c_str()));
        clist_append(sec_list, value);
    }

    section_part = mailimap_section_part_new(sec_list);
    section = mailimap_section_new_part(section_part);
    if (wholePart) {
        fetch_att = mailimap_fetch_att_new_body_peek_section(section);
    }
    else {
        fetch_att = mailimap_fetch_att_new_body_peek_section_partial(section, offset, length);
    }
    fetch_type = mailimap_fetch_type_new_fetch_att(fetch_att);

#ifdef LIBETPAN_HAS_MAILIMAP_RAMBLER_WORKAROUND
    if (m_ramblerRuServer && (encoding == EncodingBase64 || encoding == EncodingUUEncode)) {
        mailimap_set_rambler_workaround_enabled(m_imap, 1);
    }
#endif

    r = fetch_imap(m_imap, isUid, uidOrNumber, fetch_type, &text, &text_length);
    mailimap_fetch_type_free(fetch_type);

#ifdef LIBETPAN_HAS_MAILIMAP_RAMBLER_WORKAROUND
    mailimap_set_rambler_workaround_enabled(m_imap, 0);
#endif

    if (r == MAILIMAP_ERROR_STREAM) {
        //mShouldDisconnect = true;
        //*pError = ErrorConnection;
        return ErrorConnection;
    }
    else if (r == MAILIMAP_ERROR_PARSE) {
        //mShouldDisconnect = true;
        //*pError = ErrorParse;
        return ErrorParse;
    }
    else if (hasError(r)) {
        //*pError = ErrorFetch;
        return ErrorFetch;
    }

    data = string(text, text_length);

    return ErrorNone;
}
vector<string> mailImap::splictStr(const string& str, const string& sep)
{
    vector<string> result;
    string::size_type pos = 0, first_pos = 0;

    while (1)
    {
        pos = str.find(sep, first_pos);
        if (pos == string::npos)
        {
            result.push_back(str.substr(first_pos));
            break;
        }
        else
        {
            result.push_back(str.substr(first_pos, pos - first_pos));
            first_pos = pos + 1;
        }
    }
    
    return result;
}

void mailImap::decodeData(string& data, Encoding encoding)
{

}
int mailImap::selectIfNeeded(const string& folder)
{
    int r = loginIfNeeded();
    if (r)
        return r;

    if (m_status == SS_SELECTED)
    {
        if (m_currentFolder != folder)
            r = selectFolder(folder);
    }
    else if(m_status == SS_LOGGEDIN)
    {
        r = selectFolder(folder);
    }
    else
    {
        r = 0;
    }
    
    return r;
}

int mailImap::selectFolder(const string& folder)
{
    assert(m_status == SS_SELECTED || m_status == SS_LOGGEDIN);

    int r = mailimap_select(m_imap, folder.c_str());
    if (r == MAILIMAP_ERROR_STREAM)
    {
        return ErrorConnection;
    }
    else if (r == MAILIMAP_ERROR_PARSE)
    {
        return ErrorParse;
    }
    else if (hasError(r))
    {
        return ErrorNonExistantFolder;
    }

    m_currentFolder = folder;

    m_status = SS_SELECTED;
    return ErrorNone;
}

static bool hasError(int errorCode)
{
    return ((errorCode != MAILIMAP_NO_ERROR) && (errorCode != MAILIMAP_NO_ERROR_AUTHENTICATED) &&
        (errorCode != MAILIMAP_NO_ERROR_NON_AUTHENTICATED));
}
int mailImap::loginIfNeeded()
{
    int r = connectIfNeeded();
    if (r)
        return r;

    if(m_status == SS_CONNECTED)
        r = login();

    return r;
}
int mailImap::connectIfNeeded()
{
    int r = 0;
    if (m_status == SS_DISCONNECTED)
        r = connect();

    return r;
}


static int fetch_rfc822(mailimap * session, bool identifier_is_uid,
    uint32_t identifier, char ** result, size_t * result_len)
{
    struct mailimap_section * section;
    struct mailimap_fetch_att * fetch_att;
    struct mailimap_fetch_type * fetch_type;

    section = mailimap_section_new(NULL);
    fetch_att = mailimap_fetch_att_new_body_peek_section(section);
    fetch_type = mailimap_fetch_type_new_fetch_att(fetch_att);
    cout << "1 att_type:" << fetch_att->att_type << " att_size:" << fetch_att->att_size << endl;
    int r = fetch_imap(session, identifier_is_uid, identifier,
        fetch_type, result, result_len);
    mailimap_fetch_type_free(fetch_type);
    cout << "2 att_type:" << fetch_att->att_type << " att_size:" << fetch_att->att_size << endl;
    cout << "fetch_rfc822 errno:" << r << endl;
    return r;
}

static int
fetch_imap(mailimap * imap, bool identifier_is_uid, uint32_t identifier,
    struct mailimap_fetch_type * fetch_type,
    char ** result, size_t * result_len)
{
    int r;
    struct mailimap_msg_att * msg_att;
    struct mailimap_msg_att_item * msg_att_item;
    clist * fetch_result;
    struct mailimap_set * set;
    char * text;
    size_t text_length;
    clistiter * cur;

    set = mailimap_set_new_single(identifier);
    if (identifier_is_uid) {
        r = mailimap_uid_fetch(imap, set, fetch_type, &fetch_result);
        cout << "mailimap_uid_fetch errno:" << r << " fetch_type:" << fetch_type->ft_type << endl;
    }
    else {
        r = mailimap_fetch(imap, set, fetch_type, &fetch_result);
    }

    mailimap_set_free(set);

    switch (r) {
    case MAILIMAP_NO_ERROR:
        break;
    default:
        return r;
    }

    if (clist_isempty(fetch_result)) {
        mailimap_fetch_list_free(fetch_result);
        return MAILIMAP_ERROR_FETCH;
    }

    msg_att = (struct mailimap_msg_att *) clist_begin(fetch_result)->data;

    text = NULL;
    text_length = 0;

    for (cur = clist_begin(msg_att->att_list); cur != NULL;
        cur = clist_next(cur)) {
        msg_att_item = (struct mailimap_msg_att_item *) clist_content(cur);

        if (msg_att_item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
            continue;
        }

        if (msg_att_item->att_data.att_static->att_type !=
            MAILIMAP_MSG_ATT_BODY_SECTION) {
            continue;
        }

        text = msg_att_item->att_data.att_static->att_data.att_body_section->sec_body_part;
        msg_att_item->att_data.att_static->att_data.att_body_section->sec_body_part = NULL;
        text_length = msg_att_item->att_data.att_static->att_data.att_body_section->sec_length;
    }

    mailimap_fetch_list_free(fetch_result);

    if (text == NULL)
        return MAILIMAP_ERROR_FETCH;

    *result = text;
    *result_len = text_length;

    return MAILIMAP_NO_ERROR;
}


int mailImap::getfolderStatus(const string& folder, folderStatus* fs)
{
    assert(m_status == STATE_LOGGEDIN || m_status == STATE_SELECTED);

    int r;

    struct mailimap_mailbox_data_status * status;

    struct mailimap_status_att_list * status_att_list;

    status_att_list = mailimap_status_att_list_new_empty();
    mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_UNSEEN);
    mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_MESSAGES);
    mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_RECENT);
    mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_UIDNEXT);
    mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_UIDVALIDITY);
    //if (mCondstoreEnabled || mXYMHighestModseqEnabled) {
    //    mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_HIGHESTMODSEQ);
    //}

    r = mailimap_status(m_imap, folder.c_str(), status_att_list, &status);

    if (r == MAILIMAP_ERROR_STREAM) {
        //mShouldDisconnect = true;
        //*pError = ErrorConnection;
        //MCLog("status error : %s %i", MCUTF8DESC(this), *pError);
        mailimap_status_att_list_free(status_att_list);
        return ErrorConnection;
    }
    else if (r == MAILIMAP_ERROR_PARSE) {
        //mShouldDisconnect = true;
        //*pError = ErrorParse;
        mailimap_status_att_list_free(status_att_list);
        return ErrorParse;
    }
    else if (hasError(r)) {
        //*pError = ErrorNonExistantFolder;
        mailimap_status_att_list_free(status_att_list);
        return ErrorNonExistantFolder;
    }

    clistiter * cur;

    if (status != NULL) {

        struct mailimap_status_info * status_info;
        for (cur = clist_begin(status->st_info_list); cur != NULL;
            cur = clist_next(cur)) {

            status_info = (struct mailimap_status_info *) clist_content(cur);

            switch (status_info->st_att) {
            case MAILIMAP_STATUS_ATT_UNSEEN:
                fs->setUnseenCount(status_info->st_value);
                break;
            case MAILIMAP_STATUS_ATT_MESSAGES:
                fs->setMessageCount(status_info->st_value);
                break;
            case MAILIMAP_STATUS_ATT_RECENT:
                fs->setRecentCount(status_info->st_value);
                break;
            case MAILIMAP_STATUS_ATT_UIDNEXT:
                fs->setUidNext(status_info->st_value);
                break;
            case MAILIMAP_STATUS_ATT_UIDVALIDITY:
                fs->setUidValidity(status_info->st_value);
                break;
            case MAILIMAP_STATUS_ATT_EXTENSION: {
                struct mailimap_extension_data * ext_data = status_info->st_ext_data;
                if (ext_data->ext_extension == &mailimap_extension_condstore) {
                    struct mailimap_condstore_status_info * status_info = (struct mailimap_condstore_status_info *) ext_data->ext_data;
                    fs->setHighestModSeqValue(status_info->cs_highestmodseq_value);
                }
                break;
            }
            }
        }

        mailimap_mailbox_data_status_free(status);
    }

    mailimap_status_att_list_free(status_att_list);

    return ErrorNone;
}

int mailImap::fetchSubscribedFolders(vector<imapFolder>& subFolders)
{
    int r;
    clist * imap_folders;

    r = loginIfNeeded();
    if (r != ErrorNone)
        return r;

    if (m_delimiter == 0) {
        char delimiter;

        r = fetchDelimiterIfNeeded(m_delimiter, delimiter);
        if (r != ErrorNone)
            return r;

        //setDelimiter(delimiter);
        m_delimiter = delimiter;
    }

    string prefix = "";
    //prefix = defaultNamespace()->mainPrefix();
    //if (prefix == NULL) {
    //    prefix = MCSTR("");
    //}
    //if (prefix->length() > 0) {
    //    if (!prefix->hasSuffix(String::stringWithUTF8Format("%c", mDelimiter))) {
    //        prefix = prefix->stringByAppendingUTF8Format("%c", mDelimiter);
    //    }
    //}

    r = mailimap_lsub(m_imap, prefix.c_str(), "*", &imap_folders);

    r = resultsWithError(r, imap_folders, subFolders);
    if (r != ErrorNone)
        return r;

    return r;
}
int mailImap::fetchDelimiterIfNeeded(char defaultDelimiter, char& result)
{
    int r;
    clist * imap_folders;
    imapFolder * folder;
    vector<imapFolder> folders;

    if (defaultDelimiter != 0)
        return defaultDelimiter;

    r = mailimap_list(m_imap, "", "", &imap_folders);
    r = resultsWithError(r, imap_folders, folders);
    //if (*pError == ErrorConnection || *pError == ErrorParse)
        //mShouldDisconnect = true;
    if (r != ErrorNone)
        return r;

    if (folders.size() > 0) {
        folder = &folders.at(0);
    }
    else {
        folder = NULL;
    }
    if (folder == NULL)
        return 0;

    result = folder->delimiter();
    r = ErrorNone;
    return r;
}
int mailImap::fetchAllFolders(vector<string>& allFolders)
{
    int r;
    clist * imap_folders;

    r = loginIfNeeded();
    if (r != ErrorNone)
        return r;

    if (m_delimiter == 0) {
        char delimiter;

        r = fetchDelimiterIfNeeded(m_delimiter, delimiter);
        if (r != ErrorNone)
            return r;

        //setDelimiter(delimiter);
        m_delimiter = delimiter;
    }

    //String * prefix = NULL;
    //if (defaultNamespace()) {
    //    prefix = defaultNamespace()->mainPrefix();
    //}
    //if (prefix == NULL) {
    //    prefix = MCSTR("");
    //}
    //if (prefix->length() > 0) {
    //    if (!prefix->hasSuffix(String::stringWithUTF8Format("%c", mDelimiter))) {
    //        prefix = prefix->stringByAppendingUTF8Format("%c", mDelimiter);
    //    }
    //}

    //if (mXListEnabled) {
    //    r = mailimap_xlist(mImap, MCUTF8(prefix), "*", &imap_folders);
    //}
    //else {
    //    r = mailimap_list(mImap, MCUTF8(prefix), "*", &imap_folders);
    //}
    //Array * result = resultsWithError(r, imap_folders, pError);
    //if (*pError == ErrorConnection || *pError == ErrorParse)
    //    mShouldDisconnect = true;

    //if (result != NULL) {
    //    bool hasInbox = false;
    //    mc_foreacharray(IMAPFolder, folder, result) {
    //        if (folder->path()->isEqual(MCSTR("INBOX"))) {
    //            hasInbox = true;
    //        }
    //    }

    //    if (!hasInbox) {
    //        mc_foreacharray(IMAPFolder, folder, result) {
    //            if (folder->flags() & IMAPFolderFlagInbox) {
    //                // some mail providers use non-standart name for inbox folder
    //                hasInbox = true;
    //                folder->setPath(MCSTR("INBOX"));
    //                break;
    //            }
    //        }

    //        if (!hasInbox) {
    //            r = mailimap_list(mImap, "", "INBOX", &imap_folders);
    //            Array * inboxResult = resultsWithError(r, imap_folders, pError);
    //            if (*pError == ErrorConnection || *pError == ErrorParse)
    //                mShouldDisconnect = true;
    //            result->addObjectsFromArray(inboxResult);
    //            hasInbox = true;
    //        }
    //    }
    //}

    return result;
}
int mailImap::renameFolder(string& folder, string& otherName)
{

}
int mailImap::deleteFolder(const string& folder)
{

}
int mailImap::createFolder(const string& folder)
{

}

static int resultsWithError(int r, clist * list, vector<imapFolder>& result)
{
    clistiter * cur;

    if (r == MAILIMAP_ERROR_STREAM) {
        //*pError = ErrorConnection;
        return ErrorConnection;
    }
    else if (r == MAILIMAP_ERROR_PARSE) {
        //*pError = ErrorParse;
        return ErrorParse;
    }
    else if (hasError(r)) {
        //*pError = ErrorNonExistantFolder;
        return ErrorNonExistantFolder;
    }

    for (cur = clist_begin(list); cur != NULL; cur = cur->next) {
        struct mailimap_mailbox_list * mb_list;
        IMAPFolderFlag flags;
        imapFolder folder;
        string path;

        mb_list = (struct mailimap_mailbox_list *) cur->data;

        flags = IMAPFolderFlagNone;
        if (mb_list->mb_flag != NULL)
            flags = (IMAPFolderFlag)imap_mailbox_flags_to_flags(mb_list->mb_flag);

        //folder = new IMAPFolder();
        path = mb_list->mb_name;
        
        if (toupper(path) == "INBOX") {
            folder.setPath("INBOX");
        }
        else {
            folder.setPath(path);
        }
        folder.setDelimiter(mb_list->mb_delimiter);
        folder.setFlags(flags);

        result.push_back(folder);
    }

    mailimap_list_result_free(list);

    //*pError = ErrorNone;
    return ErrorNone;
}