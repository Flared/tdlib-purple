#ifndef _PURPLE_EVENTS_H
#define _PURPLE_EVENTS_H

#include <purple.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <queue>

struct PurpleEvent;

class PurpleEventReceiver {
public:
    void addEvent(std::unique_ptr<PurpleEvent> event);
    void verifyEvent(const PurpleEvent &event);
    void verifyEvents(std::initializer_list<std::unique_ptr<PurpleEvent>> events);
    void verifyNoEvents();
    void discardEvents();
private:
    std::queue<std::unique_ptr<PurpleEvent>> m_events;
};

extern PurpleEventReceiver g_purpleEvents;

enum class PurpleEventType: uint8_t {
    AccountSetAlias,
    ShowAccount,
    AddBuddy,
    HideAccount,
    RemoveBuddy,
    ConnectionError,
    ConnectionSetState,
    ConnectionUpdateProgress,
    NewConversation,
    ConversationWrite,
    NotifyMessage,
    UserStatus,
    RequestInput,
    JoinChatFailed,
    ServGotIm,
    BuddyTypingStart,
    BuddyTypingStop,
};

struct PurpleEvent {
    PurpleEventType type;

    PurpleEvent(PurpleEventType type) : type(type) {}
    virtual ~PurpleEvent() {}
    std::string toString() const;
};

struct AccountSetAliasEvent: PurpleEvent {
    PurpleAccount *account;
    std::string    alias;

    AccountSetAliasEvent(PurpleAccount *account, const std::string &alias)
    : PurpleEvent(PurpleEventType::AccountSetAlias), account(account), alias(alias) {}
};

struct ShowAccountEvent: PurpleEvent {
    PurpleAccount *account;

    ShowAccountEvent(PurpleAccount *account)
    : PurpleEvent(PurpleEventType::ShowAccount), account(account) {}
};

struct AddBuddyEvent: PurpleEvent {
    std::string      username;
    std::string      alias;
    PurpleAccount   *account;
    PurpleContact   *contact;
    PurpleGroup     *group;
    PurpleBlistNode *node;

    AddBuddyEvent(const std::string &username, const std::string &alias, PurpleAccount *account,
                  PurpleContact *contact, PurpleGroup *group, PurpleBlistNode *node)
    : PurpleEvent(PurpleEventType::AddBuddy), username(username), alias(alias), account(account),
      contact(contact), group(group), node(node) {}
};

struct HideAccountEvent: PurpleEvent {
    PurpleAccount *account;
};

struct RemoveBuddyEvent: PurpleEvent {
    PurpleAccount *account;
    std::string    username;

    RemoveBuddyEvent(PurpleAccount *account, const std::string &username)
    : PurpleEvent(PurpleEventType::RemoveBuddy), account(account), username(username) {}
};

struct ConnectionErrorEvent: PurpleEvent {
    PurpleConnection *connection;
    std::string       message;
};

struct ConnectionSetStateEvent: PurpleEvent {
    PurpleConnection      *connection;
    PurpleConnectionState  state;

    ConnectionSetStateEvent(PurpleConnection *connection, PurpleConnectionState state)
    : PurpleEvent(PurpleEventType::ConnectionSetState), connection(connection), state(state) {}
};

struct ConnectionUpdateProgressEvent: PurpleEvent {
    PurpleConnection *connection;
    size_t            step, stepCount;

    ConnectionUpdateProgressEvent(PurpleConnection *connection,
                                  size_t step, size_t stepCount)
    : PurpleEvent(PurpleEventType::ConnectionUpdateProgress), connection(connection),
      step(step), stepCount(stepCount) {}
};

struct NewConversationEvent: PurpleEvent {
    PurpleConversationType  type;
    PurpleAccount          *account;
    std::string             name;

    NewConversationEvent(PurpleConversationType type, PurpleAccount *account, const std::string &name)
    : PurpleEvent(PurpleEventType::NewConversation), type(type), account(account), name(name) {}
};

struct ConversationWriteEvent: PurpleEvent {
    std::string        conversation;
    std::string        username;
    std::string        message;
    PurpleMessageFlags flags;
    time_t             mtime;

    ConversationWriteEvent(const std::string &conversationName, const std::string &username,
                           const std::string &message, PurpleMessageFlags flags, time_t mtime)
    : PurpleEvent(PurpleEventType::ConversationWrite), conversation(conversationName),
    username(username), message(message), flags(flags), mtime(mtime) {}
};

struct NotifyMessageEvent: PurpleEvent {
    void                *handle;
    PurpleNotifyMsgType  type;
    std::string          title;
    std::string          primary;
    std::string          secondary;
};

struct UserStatusEvent: PurpleEvent {
    PurpleAccount        *account;
    std::string           username;
    PurpleStatusPrimitive status;

    UserStatusEvent(PurpleAccount *account, const std::string &username, PurpleStatusPrimitive status)
    : PurpleEvent(PurpleEventType::UserStatus), account(account), username(username), status(status) {}
};

struct RequestInputEvent: PurpleEvent {
    void               *handle;
    std::string         title, primary;
	std::string         secondary, default_value;
	GCallback           ok_cb;
	GCallback           cancel_cb;
	PurpleAccount      *account;
    std::string         username;
    PurpleConversation *conv;
	void               *user_data;
};

struct JoinChatFailedEvent: PurpleEvent {
    PurpleConnection *connection;
};

struct ServGotImEvent: PurpleEvent {
    PurpleConnection  *connection;
    std::string        username;
    std::string        message;
    PurpleMessageFlags flags;
    time_t             mtime;

    ServGotImEvent(PurpleConnection *connection, const std::string &username, const std::string &message,
                   PurpleMessageFlags flags, time_t mtime)
    : PurpleEvent(PurpleEventType::ServGotIm), connection(connection), username(username),
      message(message), flags(flags), mtime(mtime) {}
};

struct BuddyTypingStartEvent: PurpleEvent {
    PurpleConnection *connection;
    std::string       username;
    int               timeout;
    PurpleTypingState state;
};

struct BuddyTypingStopEvent: PurpleEvent {
    PurpleConnection *connection;
    std::string       username;
};

#endif
