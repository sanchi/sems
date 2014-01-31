
#include "SimpleRtpRelay.h"
#include "AmSession.h"

bool _SimpleRtpRelay::createNewRelay(string a_relay_ip, string b_relay_ip, string& session_id,
				     unsigned int& a_port, unsigned int& b_port) {

  try {

    session_id = long2str((((long)get_random()) << 32) | (u_int32_t)((unsigned long)pthread_self())); //AmSession::getNewId();
    DBG("creating simple RTP relay with session ID '%s'\n", session_id.c_str());
    RtpRelayPair* rp = new RtpRelayPair();
 
    rp->a.setRelayStream(&rp->a);
    rp->b.setRelayStream(&rp->b);

    rp->a.enableRtpRelay();
    rp->b.enableRtpRelay();

    rp->a.enableRawRelay();
    rp->b.enableRawRelay();
  
    rp->a.setLocalIP(a_relay_ip);
    rp->b.setLocalIP(b_relay_ip);

    a_port = rp->a.getLocalPort();
    b_port = rp->b.getLocalPort();

    rp->a.resumeReceiving();
    rp->b.resumeReceiving();

    // default to passive mode (no remote IPs known anyway)
    rp->a.setPassiveMode(true);
    rp->b.setPassiveMode(true);

    rp->a.setRAddr("0.0.0.0", 0 ,0);
    rp->b.setRAddr("0.0.0.0", 0 ,0);

    rp->a.setHttpResponseMode();

    relays_mut.lock();
    relays[session_id] = rp;
    relays_mut.unlock();

    DBG("created RTP relay 0.0.0.0/0 <--- %s:%u <=> %s:%u ---> 0.0.0.0/0\n",
	a_relay_ip.c_str(), a_port, b_relay_ip.c_str(), b_port);

    return true;
  } catch (const string& s) {
    ERROR("initializing RTP stream pair: '%s'\n", s.c_str());
    return false;
  }
}

bool _SimpleRtpRelay::removeRelay(string& session_id) {
  relays_mut.lock();
  map<string, RtpRelayPair*>::iterator it=relays.find(session_id);
  if (it == relays.end()) {
    relays_mut.unlock();
    ERROR("removing inexistent relay '%s'\n", session_id.c_str());
    return false;
  }
  RtpRelayPair* rp = it->second;
  relays.erase(it);
  relays_mut.unlock();

  rp->a.stopReceiving();
  rp->b.stopReceiving();

  delete rp;

  DBG("removed relay '%s'\n", session_id.c_str());
  return true;
}

CONST_ACTION_2P(MHCreateSimpleRtpRelay, ',', false);
EXEC_ACTION_START(MHCreateSimpleRtpRelay) {

  string session_id;
  unsigned int a_port;
  unsigned int b_port;
  if (SimpleRtpRelay::instance()->
      createNewRelay(resolveVars(par1, sess, sc_sess, event_params),
		     resolveVars(par2, sess, sc_sess, event_params),
		     session_id, a_port, b_port)) {
    if (event_params) {
      (*event_params)["session_id"] = session_id;
      (*event_params)["a_port"] = int2str(a_port);
      (*event_params)["b_port"] = int2str(b_port);
    }

    sc_sess->CLR_ERRNO;
  } else {
    sc_sess->SET_ERRNO(DSM_ERRNO_GENERAL);    
  }
} EXEC_ACTION_END;


EXEC_ACTION_START(MHRemoveRtpRelay) {
  string session_id = resolveVars(arg, sess, sc_sess, event_params);
  if (SimpleRtpRelay::instance()->removeRelay(session_id)) {
    sc_sess->CLR_ERRNO;
  } else {
    sc_sess->SET_ERRNO(DSM_ERRNO_UNKNOWN_ARG);    
  }
} EXEC_ACTION_END;
