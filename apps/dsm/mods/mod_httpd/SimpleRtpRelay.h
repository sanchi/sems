#ifndef _SimpleRtpRelay_h_
#define _SimpleRtpRelay_h_

#include "DSMModule.h"
#include "DSMSession.h"

#include "AmRtpStream.h"
#include <map>

#include "singleton.h"

using std::map;


struct RtpRelayPair {
RtpRelayPair() : a(NULL, -1), b(NULL, -1) { }
  AmRtpStream a;
  AmRtpStream b;
};

class _SimpleRtpRelay {

  map<string, RtpRelayPair*> relays;
  AmMutex relays_mut;

 public:
  _SimpleRtpRelay()  { }
  ~_SimpleRtpRelay() { }

  bool createNewRelay(string a_relay_ip, string b_relay_ip, string& session_id,
				 unsigned int& a_port, unsigned int& b_port);
  bool removeRelay(string& session_id);
};

typedef singleton<_SimpleRtpRelay> SimpleRtpRelay;


DEF_ACTION_2P(MHCreateSimpleRtpRelay);
DEF_ACTION_1P(MHRemoveRtpRelay);

#endif
