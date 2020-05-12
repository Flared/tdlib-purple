#ifndef _ACCOUNT_DATA_H
#define _ACCOUNT_DATA_H

#include <td/telegram/td_api.h>
#include <map>
#include <mutex>

bool        isCanonicalPhoneNumber(const char *s);
bool        isPhoneNumber(const char *s);
const char *getCanonicalPhoneNumber(const char *s);
std::string getDisplayName(const td::td_api::user *user);
int32_t     getBasicGroupId(const td::td_api::chat &chat); // returns 0 if not chatTypeBasicGroup
int32_t     getSupergroupId(const td::td_api::chat &chat); // returns 0 if not chatTypeSupergroup
bool        isGroupMember(const td::td_api::object_ptr<td::td_api::ChatMemberStatus> &status);

enum {
    CHAT_HISTORY_REQUEST_LIMIT  = 50,
    CHAT_HISTORY_RETRIEVE_LIMIT = 100
};

class PendingRequest {
public:
    uint64_t requestId;

    PendingRequest(uint64_t requestId) : requestId(requestId) {}
    virtual ~PendingRequest() {} // for dynamic_cast
};

class GroupInfoRequest: public PendingRequest {
public:
    int32_t groupId;

    GroupInfoRequest(uint64_t requestId, int32_t groupId)
    : PendingRequest(requestId), groupId(groupId) {}
};

enum class FileFallback: uint8_t {
    None,
    ReplaceTgs,
};

// Matching completed downloads to chats they belong to
class DownloadRequest: public PendingRequest {
public:
    int64_t      chatId;
    std::string  sender;
    int32_t      timestamp;
    bool         outgoing;
    std::string  label;
    FileFallback fallbackType;
    td::td_api::object_ptr<td::td_api::file> fallback;

    DownloadRequest(uint64_t requestId, int64_t chatId, const std::string &sender, int32_t timestamp,
                    bool outgoing, const std::string &label, FileFallback fallbackType,
                    td::td_api::file *fallback)
    : PendingRequest(requestId), chatId(chatId), sender(sender), timestamp(timestamp),
      outgoing(outgoing), label(label), fallbackType(fallbackType), fallback(fallback) {}
};

class TdAccountData {
public:
    using TdUserPtr       = td::td_api::object_ptr<td::td_api::user>;
    using TdChatPtr       = td::td_api::object_ptr<td::td_api::chat>;
    using TdMessagePtr    = td::td_api::object_ptr<td::td_api::message>;
    using TdGroupPtr      = td::td_api::object_ptr<td::td_api::basicGroup>;
    using TdGroupInfoPtr  = td::td_api::object_ptr<td::td_api::basicGroupFullInfo>;
    using TdSupergroupPtr = td::td_api::object_ptr<td::td_api::supergroup>;

    void updateUser(TdUserPtr user);
    void updateBasicGroup(TdGroupPtr group);
    void updateBasicGroupInfo(int32_t groupId, TdGroupInfoPtr groupInfo);
    void updateSupergroup(TdSupergroupPtr group);

    void addChat(TdChatPtr chat); // Updates existing chat if any
    void setContacts(const std::vector<std::int32_t> &userIds);
    void setActiveChats(std::vector<std::int64_t> &&chats);
    void getContactsWithNoChat(std::vector<std::int32_t> &userIds);
    void getActiveChats(std::vector<const td::td_api::chat *> &chats) const;

    const td::td_api::chat       *getChat(int64_t chatId) const;
    int                           getPurpleChatId(int64_t tdChatId);
    const td::td_api::chat       *getChatByPurpleId(int32_t purpleChatId) const;
    const td::td_api::chat       *getPrivateChatByUserId(int32_t userId) const;
    const td::td_api::user       *getUser(int32_t userId) const;
    const td::td_api::user       *getUserByPhone(const char *phoneNumber) const;
    const td::td_api::user       *getUserByPrivateChat(const td::td_api::chat &chat);
    const td::td_api::basicGroup *getBasicGroup(int32_t groupId) const;
    const td::td_api::basicGroupFullInfo *getBasicGroupInfo(int32_t groupId) const;
    const td::td_api::supergroup *getSupergroup(int32_t groupId) const;
    const td::td_api::chat       *getBasicGroupChatByGroup(int32_t groupId) const;
    const td::td_api::chat       *getSupergroupChatByGroup(int32_t groupId) const;
    bool                          isGroupChatWithMembership(const td::td_api::chat &chat);

    void addNewContactRequest(uint64_t requestId, const char *phoneNumber, const char *alias, int32_t userId = 0);
    bool extractContactRequest(uint64_t requestId, std::string &phoneNumber, std::string &alias, int32_t &userId);

    void addDelayedMessage(int32_t userId, TdMessagePtr message);
    void extractDelayedMessagesByUser(int32_t userId, std::vector<TdMessagePtr> &messages);

    template<typename ReqType, typename... ArgsType>
    void addPendingRequest(ArgsType... args)
    {
        m_requests.push_back(std::make_unique<ReqType>(args...));
    }
    template<typename ReqType>
    std::unique_ptr<ReqType> getPendingRequest(uint64_t requestId)
    {
        return std::unique_ptr<ReqType>(dynamic_cast<ReqType *>(getPendingRequestImpl(requestId).release()));
    }
private:
    struct ContactRequest {
        // TODO: refactor into PendingRequest
        uint64_t    requestId;
        std::string phoneNumber;
        std::string alias;
        int32_t     userId;
    };

    struct PendingMessage {
        TdMessagePtr message;
        int32_t      userId;
    };

    struct ChatInfo {
        int32_t   purpleId;
        TdChatPtr chat;

        ChatInfo() : purpleId(0), chat() {}
    };

    struct GroupInfo {
        TdGroupPtr     group;
        TdGroupInfoPtr fullInfo;
    };

    using ChatMap = std::map<int64_t, ChatInfo>;
    using UserMap = std::map<int32_t, TdUserPtr>;
    std::map<int32_t, TdUserPtr>       m_userInfo;
    std::map<int64_t, ChatInfo>        m_chatInfo;
    std::map<int32_t, GroupInfo>       m_groups;
    std::map<int32_t, TdSupergroupPtr> m_supergroups;
    int                                m_lastChatPurpleId = 0;

    // List of contacts for which private chat is not known yet.
    std::vector<int32_t>               m_contactUserIdsNoChat;

    // m_chatInfo can contain chats that are not in m_activeChats if some other chat contains
    // messages forwarded from another channel
    std::vector<int64_t>               m_activeChats;

    // Used to remember stuff during asynchronous communication when adding contact
    std::vector<ContactRequest>        m_addContactRequests;

    // When someone completely new writes to us, the first message has been observed to arrive
    // before their phone number is known. Such message with linger here until phone number becomes
    // known, at which point it becomes possible to create libpurple contact and show the message
    // properly
    std::vector<PendingMessage>        m_delayedMessages;

    std::vector<std::unique_ptr<PendingRequest>> m_requests;

    std::unique_ptr<PendingRequest>    getPendingRequestImpl(uint64_t requestId);
};

#endif
