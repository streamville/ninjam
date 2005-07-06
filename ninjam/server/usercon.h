#ifndef _USERCON_H_
#define _USERCON_H_


#include "../netmsg.h"
#include "../../WDL/string.h"
#include "../../WDL/sha.h"
#include "../../WDL/ptrlist.h"
#include "../mpb.h"

#define MAX_USER_CHANNELS 32
#define MAX_USERS 64
#define MAX_UPLOADS 32
#define MAX_DOWNLOADS 128

#define PRIV_TOPIC 1
#define PRIV_CHATSEND 2
#define PRIV_BPM 4
#define PRIV_KICK 8
#define PRIV_RESERVE 16


class User_SubscribeMask
{
public:
  User_SubscribeMask()  {} 
  ~User_SubscribeMask() {}
  WDL_String username;
  unsigned int channelmask;
};

class User_Channel
{
public:
  User_Channel() { active=0; volume=0; panning=128; flags=0; }
  ~User_Channel() { }
  int active;
  WDL_String name;
  int volume;
  int panning;
  int flags;
};


class IUserInfoLookup // abstract base class, overridden by server
{
public:
  IUserInfoLookup() { user_valid=0; isanon=0; privs=0; max_channels=0; }
  virtual ~IUserInfoLookup() { }

  virtual int Run()=0; // return 1 if run is complete, 0 if still needs to run more
  virtual void OnAbandon()=0; // default should be "delete this", but can override if needed

  int user_valid;

  WDL_String username;

  int isanon;
  WDL_String anon_username;

  unsigned char sha1buf_user[WDL_SHA1SIZE];
  unsigned int privs;
  int max_channels;

  unsigned char sha1buf_request[WDL_SHA1SIZE]; // don't use, internal for User_Connection
};


class User_TransferState
{
public:
  User_TransferState() : fourcc(0), bytes_estimated(0), bytes_sofar(0), fp(0)
  { 
    time(&last_acttime);
    memset(guid,0,sizeof(guid));
  }
  ~User_TransferState() 
  { 
    if (fp) fclose(fp);
    fp=0;
  }

  time_t last_acttime;
  unsigned char guid[16];
  unsigned int fourcc;
  unsigned int bytes_estimated;

  unsigned int bytes_sofar;
  
  FILE *fp;
};


class User_Group;

class User_Connection
{
  public:
    User_Connection(JNL_Connection *con, User_Group *grp);
    ~User_Connection();

    int Run(User_Group *group, int *wantsleep=0); // returns 1 if disconnected, -1 if error in data. 0 if ok.
    void SendConfigChangeNotify(int bpm, int bpi);

    void Send(Net_Message *msg) { m_netcon.Send(msg); }

    int OnRunAuth(User_Group *group, IUserInfoLookup *uinfo);

    Net_Connection m_netcon;
    WDL_String m_username;
    
    // auth info
    time_t m_connect_time;
    int m_auth_state;      // 1 if authorized, 0 if not yet, -1 if auth pending
    unsigned char m_challenge[8];
    int m_clientcaps;

    int m_auth_privs;

    int m_reserved;

    int m_max_channels;

    User_Channel m_channels[MAX_USER_CHANNELS];

    WDL_PtrList<User_SubscribeMask> m_sublist; // people+channels we subscribe to

    WDL_PtrList<User_TransferState> m_recvfiles;
    WDL_PtrList<User_TransferState> m_sendfiles;

    IUserInfoLookup *m_lookup;
};



class User_Group
{
  public:
    User_Group();
    ~User_Group();

    void AddConnection(JNL_Connection *con, int isres=0);

    int Run(); // return 1 if safe to sleep
    void SetConfig(int bpi, int bpm);
    void SetLicenseText(char *text) { m_licensetext.Set(text); }
    void Broadcast(Net_Message *msg, User_Connection *nosend=0);


    void SetLogDir(char *path); // NULL to not log

    // sends a message to the people subscribing to a channel of a user
    void BroadcastToSubs(Net_Message *msg, User_Connection *src, int channel);

    IUserInfoLookup *(*CreateUserLookup)(char *username);

    void onChatMessage(User_Connection *con, mpb_chat_message *msg);

    WDL_PtrList<User_Connection> m_users;

    int m_max_users;
    int m_last_bpm, m_last_bpi;

    int m_loopcnt;

    int m_allow_hidden_users;

    WDL_String m_licensetext;
    WDL_String m_topictext;

    WDL_String m_logdir;
    FILE *m_logfp;

#ifdef _WIN32
    DWORD m_next_loop_time;
#else
    struct timeval m_next_loop_time;
#endif
};



#endif//_USERCON_H_
