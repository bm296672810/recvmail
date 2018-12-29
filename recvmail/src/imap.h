#ifndef __IMAP_H__
#define __IMAP_H__

#include <string>
#include <assert.h>
#include <iostream>
#include <vector>

#include "libetpan/mailimap.h"

class folderStatus;
class imapFolder;

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

enum IMAPFolderFlag {
    IMAPFolderFlagNone = 0,
    IMAPFolderFlagMarked = 1 << 0,
    IMAPFolderFlagUnmarked = 1 << 1,
    IMAPFolderFlagNoSelect = 1 << 2,
    IMAPFolderFlagNoInferiors = 1 << 3,
    IMAPFolderFlagInbox = 1 << 4,
    IMAPFolderFlagSentMail = 1 << 5,
    IMAPFolderFlagStarred = 1 << 6,
    IMAPFolderFlagAllMail = 1 << 7,
    IMAPFolderFlagTrash = 1 << 8,
    IMAPFolderFlagDrafts = 1 << 9,
    IMAPFolderFlagSpam = 1 << 10,
    IMAPFolderFlagImportant = 1 << 11,
    IMAPFolderFlagArchive = 1 << 12,
    IMAPFolderFlagAll = IMAPFolderFlagAllMail,
    IMAPFolderFlagJunk = IMAPFolderFlagSpam,
    IMAPFolderFlagFlagged = IMAPFolderFlagStarred,
    IMAPFolderFlagFolderTypeMask = IMAPFolderFlagInbox | IMAPFolderFlagSentMail | IMAPFolderFlagStarred | IMAPFolderFlagAllMail |
    IMAPFolderFlagTrash | IMAPFolderFlagDrafts | IMAPFolderFlagSpam | IMAPFolderFlagImportant | IMAPFolderFlagArchive,
};

static int
fetch_imap(mailimap * imap, bool identifier_is_uid, uint32_t identifier,
    struct mailimap_fetch_type * fetch_type,
    char ** result, size_t * result_len);

static int fetch_rfc822(mailimap * session, bool identifier_is_uid,
    uint32_t identifier, char ** result, size_t * result_len);

static int resultsWithError(int r, clist * list, vector<imapFolder>& result);

static bool hasError(int errorCode);

static string toupper(const string& str);
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

    virtual int getfolderStatus(const string& folder, folderStatus* fs);

    virtual int fetchSubscribedFolders(vector<imapFolder>& subFolders);
    virtual int fetchAllFolders(vector<string>& allFolders); // will use xlist if available

    virtual int renameFolder(string& folder, string& otherName);
    virtual int deleteFolder(const string& folder);
    virtual int createFolder(const string& folder);

private:
    int getMessageAttachment(const string& folder, bool isUid, uint32_t uidOrNumber, string& partId, Encoding encoding, string& data);
    int getNonDecodedMessageAttachment(const string& folder, bool isUid, uint32_t uidOrNumber, string& partId,
        bool wholePart, uint32_t offset, uint32_t length, Encoding encoding, string& data);
    int getMessage(const string& folder, bool isUid, uint32_t uidOrNumber, string& data);
    void decodeData(string& data, Encoding encoding);
    int selectFolder(const string& folder);
    int selectIfNeeded(const string& folder);
    int loginIfNeeded();
    int connectIfNeeded();
    int fetchDelimiterIfNeeded(char defaultDelimiter, char& result);
    void init();
    
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

    char        m_delimiter;
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

class folderStatus
{
public:
    folderStatus() { init(); }
    ~folderStatus() {}

    virtual void setUnseenCount(uint32_t unseen) { m_unseenCount = unseen; }
    virtual uint32_t unseenCount() { return m_unseenCount; }

    virtual void setMessageCount(uint32_t messages) { m_messageCount = messages; }
    virtual uint32_t messageCount() { return m_messageCount; }

    virtual void setRecentCount(uint32_t recent) { m_recentCount = recent; }
    virtual uint32_t recentCount() { return m_recentCount; }

    virtual void setUidNext(uint32_t uidNext) { m_uidNext = uidNext; }
    virtual uint32_t uidNext() { return m_uidNext; }

    virtual void setUidValidity(uint32_t uidValidity) { m_uidValidity = uidValidity; }
    virtual uint32_t uidValidity() { return m_uidValidity; }

    virtual void setHighestModSeqValue(uint64_t highestModSeqValue) { m_highestModSeqValue = highestModSeqValue; }
    virtual uint64_t highestModSeqValue() { return m_highestModSeqValue; }

private:
    uint32_t m_unseenCount;
    uint32_t m_messageCount;
    uint32_t m_recentCount;
    uint32_t m_uidNext;
    uint32_t m_uidValidity;
    uint64_t m_highestModSeqValue;

    void init() { m_unseenCount = 0; m_messageCount = 0; m_recentCount = 0; m_uidNext = 0; m_uidValidity = 0; m_highestModSeqValue = 0; }
};

class imapFolder
{
public:
    imapFolder() { init(); }
    virtual ~imapFolder() {}
    virtual void setPath(const string& path) { m_path = path; }
    virtual string path() const { return m_path; }

    virtual void setDelimiter(char delimiter) { m_delimiter = delimiter; }
    virtual char delimiter() { return m_delimiter; }

    virtual void setFlags(IMAPFolderFlag flags) { m_flags = flags; }
    virtual IMAPFolderFlag flags() { return m_flags; }
private:
    string m_path;
    char m_delimiter;
    IMAPFolderFlag m_flags;
    void init() { m_delimiter = 0; m_flags = IMAPFolderFlagNone; }
};
#endif