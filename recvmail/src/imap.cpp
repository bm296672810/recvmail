#include "imap.h"

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
    int r = selectIfNeed(folder);
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
    r = selectIfNeed(folder);
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
int mailImap::selectIfNeed(const string& folder)
{
    int r = loginIfNeed();
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

bool mailImap::hasError(int errorCode)
{
    return ((errorCode != MAILIMAP_NO_ERROR) && (errorCode != MAILIMAP_NO_ERROR_AUTHENTICATED) &&
        (errorCode != MAILIMAP_NO_ERROR_NON_AUTHENTICATED));
}
int mailImap::loginIfNeed()
{
    int r = connectIfNeed();
    if (r)
        return r;

    if(m_status == SS_CONNECTED)
        r = login();

    return r;
}
int mailImap::connectIfNeed()
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