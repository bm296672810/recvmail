#ifndef __IMAP_H__
#define __IMAP_H__

#include <string>
#include <assert.h>
#include <iostream>
#include <vector>

#include "libetpan/mailimap.h"

using namespace std;

enum Encoding {
    Encoding7Bit = 0,            // should match MAILIMAP_BODY_FLD_ENC_7BIT
    Encoding8Bit = 1,            // should match MAILIMAP_BODY_FLD_ENC_8BIT
    EncodingBinary = 2,          // should match MAILIMAP_BODY_FLD_ENC_BINARY
    EncodingBase64 = 3,          // should match MAILIMAP_BODY_FLD_ENC_BASE64
    EncodingQuotedPrintable = 4, // should match MAILIMAP_BODY_FLD_ENC_QUOTED_PRINTABLE
    EncodingOther = 5,           // should match MAILIMAP_BODY_FLD_ENC_OTHER
                                 // negative values should be used for other encoding
                                 EncodingUUEncode = -1
};

static int
fetch_imap(mailimap * imap, bool identifier_is_uid, uint32_t identifier,
    struct mailimap_fetch_type * fetch_type,
    char ** result, size_t * result_len);

static int fetch_rfc822(mailimap * session, bool identifier_is_uid,
    uint32_t identifier, char ** result, size_t * result_len);

//static int fetch_imap_message(mailimap* session, uint32_t uid, char** result, size_t* result_len);
class mailImap
{
public:
    mailImap(const string& server, uint16_t port, const string& userid, const string& pwd);
    int connect();
    int login();
    void setServer(const string& server);
    string getServer();
    void setPort(uint16_t port);
    uint16_t getPort() const;
    void setUserid(const string& userid);
    string getUserid() const;
    void setPassword(const string& pwd);
    string getPassword() const;

    int getMessageByUid(const string& folder, uint32_t uid, string& data);
    int getMessageByNumber(const string& folder, uint32_t num, string& data);

    int getMessageAttachmentByUid(const string& folder, uint32_t uid, string& partId, Encoding encoding, string& data);
private:
    int getMessageAttachment(const string& folder, bool isUid, uint32_t uidOrNumber, string& partId, Encoding encoding, string& data);
    int getNonDecodedMessageAttachment(const string& folder, bool isUid, uint32_t uidOrNumber, string& partId,
        bool wholePart, uint32_t offset, uint32_t length, Encoding encoding, string& data);
    int getMessage(const string& folder, bool isUid, uint32_t uidOrNumber, string& data);
    void decodeData(string& data, Encoding encoding);
    int selectFolder(const string& folder);
    int selectIfNeed(const string& folder);
    int loginIfNeed();
    int connectIfNeed();
    void init();
    bool hasError(int errorCode);
    vector<string> splictStr(const string& str, const string& sep);
private:
    mailimap *  m_imap;
    uint16_t    m_port;
    string      m_server;
    string      m_userid;
    string      m_pwd;
    time_t      m_timeout = 10;
    bool        m_voipEenable = true;

    bool        m_isConnected = false;
    bool        m_isLogined = false;
    bool        m_yahooServer;
    bool        m_ramblerRuServer;
    bool        m_rermesServer;
    bool        m_ripServer;

    int         m_status;

    string      m_currentFolder;

    enum SessionStatus
    {
        SS_DISCONNECTED,
        SS_CONNECTED,
        SS_LOGGEDIN,
        SS_SELECTED,
    };
};

enum ErrorCode {
    ErrorNone, // 0
    ErrorConnection,
    ErrorTLSNotAvailable,
    ErrorParse,
    ErrorCertificate,
    ErrorAuthentication,
    ErrorGmailIMAPNotEnabled,
    ErrorGmailExceededBandwidthLimit,
    ErrorGmailTooManySimultaneousConnections,
    ErrorMobileMeMoved,
    ErrorYahooUnavailable, // 10
    ErrorNonExistantFolder,
    ErrorRename,
    ErrorDelete,
    ErrorCreate,
    ErrorSubscribe,
    ErrorAppend,
    ErrorCopy,
    ErrorExpunge,
    ErrorFetch,
    ErrorIdle, // 20
    ErrorIdentity,
    ErrorNamespace,
    ErrorStore,
    ErrorCapability,
    ErrorStartTLSNotAvailable,
    ErrorSendMessageIllegalAttachment,
    ErrorStorageLimit,
    ErrorSendMessageNotAllowed,
    ErrorNeedsConnectToWebmail,
    ErrorSendMessage, // 30
    ErrorAuthenticationRequired,
    ErrorFetchMessageList,
    ErrorDeleteMessage,
    ErrorInvalidAccount,
    ErrorFile,
    ErrorCompression,
    ErrorNoSender,
    ErrorNoRecipient,
    ErrorNoop,
    ErrorGmailApplicationSpecificPasswordRequired, // 40
    ErrorServerDate,
    ErrorNoValidServerFound,
    ErrorCustomCommand,
    ErrorYahooSendMessageSpamSuspected,
    ErrorYahooSendMessageDailyLimitExceeded,
    ErrorOutlookLoginViaWebBrowser,
    ErrorTiscaliSimplePassword,
};


#endif