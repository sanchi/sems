#include "SBCSimpleRelay.h"

#include "SBCSimpleRelay.h"
#include "AmSession.h"
#include "AmB2BSession.h"
#include "AmEventDispatcher.h"
#include "AmEventQueueProcessor.h"
#include "SBC.h"

/**
 * SimpleRelayDialog
 */

SimpleRelayDialog::SimpleRelayDialog(atomic_ref_cnt* parent_obj)
  : AmBasicSipDialog(this),
    AmEventQueue(this),
    parent_obj(parent_obj),
    finished(false)
{
  if(parent_obj) {
    inc_ref(parent_obj);
  }
}

SimpleRelayDialog::~SimpleRelayDialog()
{
  DBG("~SimpleRelayDialog: local_tag = %s\n",local_tag.c_str());
  if(!local_tag.empty()) {
    AmEventDispatcher::instance()->delEventQueue(local_tag);
  }
}

int SimpleRelayDialog::relayRequest(const AmSipRequest& req)
{
  relayed_reqs[cseq] = req.cseq;

  string hdrs = req.hdrs;
  if(headerfilter.size()) inplaceHeaderFilter(hdrs, headerfilter);
  if(!append_headers.empty()) hdrs += append_headers;

  if(sendRequest(req.method,&req.body,hdrs,SIP_FLAGS_VERBATIM)) {

    AmSipReply error;
    error.code = 500;
    error.reason = SIP_REPLY_SERVER_INTERNAL_ERROR;
    error.cseq = req.cseq;

    B2BSipReplyEvent* b2b_ev = new B2BSipReplyEvent(error,true,req.method);
    if(!AmEventDispatcher::instance()->post(other_dlg,b2b_ev)) {
      delete b2b_ev;
    }
    return -1;
  }
  
  return 0;
}

int SimpleRelayDialog::relayReply(const AmSipReply& reply)
{
  AmSipRequest* uas_req = getUASTrans(reply.cseq);
  if(!uas_req){
    //TODO: request already replied???
    ERROR("request already replied???\n");
    return -1;
  }

  string hdrs = reply.hdrs;
  if(headerfilter.size()) inplaceHeaderFilter(hdrs, headerfilter);
  if(!append_headers.empty()) hdrs += append_headers;

  // reply translations
  int code = reply.code;
  string reason = reply.reason;

  map<unsigned int, pair<unsigned int, string> >::iterator it =
    reply_translations.find(code);

  if (it != reply_translations.end()) {
    DBG("translating reply %u %s => %u %s\n",
	code, reason.c_str(), it->second.first, it->second.second.c_str());

    code = it->second.first;
    reason = it->second.second;
  }

  if(transparent_dlg_id &&
     ext_local_tag.empty() &&
     !reply.to_tag.empty()) {

    ext_local_tag = reply.to_tag;
  }

  if(this->reply(*uas_req,code,reason,&reply.body,
		 hdrs,SIP_FLAGS_VERBATIM)) {
    //TODO: what can we do???
    return -1;
  }

  return 0;
}

void SimpleRelayDialog::process(AmEvent* ev)
{
  AmSipEvent* sip_ev = dynamic_cast<AmSipEvent*>(ev);
  if(sip_ev) {
    (*sip_ev)(this);
    return;
  }

  B2BSipEvent* b2b_ev = dynamic_cast<B2BSipEvent*>(ev);
  if(b2b_ev) {
    if(b2b_ev->event_id == B2BSipRequest) {
      const AmSipRequest& req = ((B2BSipRequestEvent*)b2b_ev)->req;
      onB2BRequest(req);
      return;
    }
    else if(b2b_ev->event_id == B2BSipReply){
      const AmSipReply& reply = ((B2BSipReplyEvent*)b2b_ev)->reply;
      onB2BReply(reply);
      //TODO: if error, kill dialog???
      return;
    }
  }
    
  ERROR("received unknown event\n");
}

bool SimpleRelayDialog::processingCycle()
{
  DBG("vv [%s|%s] %i usages (%s) vv\n",
      callid.c_str(),local_tag.c_str(),
      getUsages(), terminated() ? "term" : "active");

  processEvents();

  DBG("^^ [%s|%s] %i usages (%s) ^^\n",
      callid.c_str(),local_tag.c_str(),
      getUsages(), terminated() ? "term" : "active");

  return !terminated();
}

void SimpleRelayDialog::finalize()
{
  DBG("finalize(): tag=%s\n",local_tag.c_str());
  if(parent_obj) {
    // this might delete us!!!
    dec_ref(parent_obj);
  }
}

void SimpleRelayDialog::onB2BRequest(const AmSipRequest& req)
{
  relayRequest(req);
}

void SimpleRelayDialog::onB2BReply(const AmSipReply& reply)
{
  if(reply.code >= 200)
    finished = true;

  relayReply(reply);
}

void SimpleRelayDialog::onSipRequest(const AmSipRequest& req)
{
  if(other_dlg.empty()) 
    return;

  B2BSipRequestEvent* b2b_ev = new B2BSipRequestEvent(req,true);
  if(!AmEventDispatcher::instance()->post(other_dlg,b2b_ev)) {
    delete b2b_ev;
  }
}

void SimpleRelayDialog::onSipReply(const AmSipRequest& req,
				   const AmSipReply& reply, 
				   AmBasicSipDialog::Status old_dlg_status)
{
  if(reply.code >= 200)
    finished = true;

  if(other_dlg.empty())
    return;

  RelayMap::iterator t_req_it = relayed_reqs.find(reply.cseq);
  if(t_req_it == relayed_reqs.end()) {
    DBG("reply to a local request ????\n");
    return;
  }
  
  B2BSipReplyEvent* b2b_ev = new B2BSipReplyEvent(reply,true,req.method);
  b2b_ev->reply.cseq = t_req_it->second;
  if(reply.code >= 200)
    relayed_reqs.erase(t_req_it);

  if(!AmEventDispatcher::instance()->post(other_dlg,b2b_ev)) {
    delete b2b_ev;
  }
}

void SimpleRelayDialog::onRequestSent(const AmSipRequest& req)
{
}

void SimpleRelayDialog::onReplySent(const AmSipRequest& req,
				     const AmSipReply& reply)
{
}

void SimpleRelayDialog::onRemoteDisappeared(const AmSipReply& reply)
{
}

void SimpleRelayDialog::onLocalTerminate(const AmSipReply& reply)
{
}

int SimpleRelayDialog::initUAC(const AmSipRequest& req, 
				const SBCCallProfile& cp)
{
  local_tag = AmSession::getNewId();

  AmEventDispatcher* ev_disp = AmEventDispatcher::instance();
  if(!ev_disp->addEventQueue(local_tag,this)) {
    ERROR("addEventQueue(%s,%p) failed.\n",
	  local_tag.c_str(),this);
    return -1;
  }

  AmSipRequest n_req(req);
  n_req.from_tag = local_tag;

  ParamReplacerCtx ctx;
  if((cp.apply_b_routing(ctx,n_req,*this) < 0) ||
     (cp.apply_common_fields(n_req,ctx) < 0) )
    return -1;

  if(cp.transparent_dlg_id){
    ext_local_tag = req.from_tag;
  }
  else if(n_req.callid == req.callid)
    n_req.callid = AmSession::getNewId();

  initFromLocalRequest(n_req);

  headerfilter = cp.headerfilter;
  reply_translations = cp.reply_translations;
  append_headers = cp.append_headers_req;

  return 0;
}

int SimpleRelayDialog::initUAS(const AmSipRequest& req, 
			       const SBCCallProfile& cp)
{
  local_tag = AmSession::getNewId();

  AmEventDispatcher* ev_disp = AmEventDispatcher::instance();
  if(!ev_disp->addEventQueue(local_tag,this)) {
    ERROR("addEventQueue(%s,%p) failed.\n",
	  local_tag.c_str(),this);
    return -1;
  }

  ParamReplacerCtx ctx;
  if(cp.apply_a_routing(ctx,req,*this) < 0)
    return -1;

  headerfilter = cp.headerfilter;
  reply_translations = cp.reply_translations;
  append_headers = cp.aleg_append_headers_req;
  transparent_dlg_id = cp.transparent_dlg_id;

  return 0;
}

/**
 * SBCSimpleRelay
 */

SBCSimpleRelay::SBCSimpleRelay(SimpleRelayDialog* a, SimpleRelayDialog* b)
  : a_leg(a), b_leg(b)
{
  assert(a_leg);
  assert(b_leg);
  a_leg->setParent(this);
  b_leg->setParent(this);
}

SBCSimpleRelay::~SBCSimpleRelay()
{
  delete a_leg;
  delete b_leg;
}

int SBCSimpleRelay::start(const AmSipRequest& req, const SBCCallProfile& cp)
{
  AmSipRequest n_req(req);

  if (!cp.append_headers.empty()) {
    n_req.hdrs += cp.append_headers;
  }

  a_leg->initUAS(n_req,cp);
  b_leg->initUAC(n_req,cp);

  a_leg->setOtherDlg(b_leg->local_tag);
  b_leg->setOtherDlg(a_leg->local_tag);

  a_leg->onRxRequest(n_req);
  if(a_leg->terminated()) {
    // free memory
    a_leg->finalize();
    b_leg->finalize();
    // avoid the caller to reply with 500
    // as the request should have been replied
    // already from within updateStatus(req)
    return 0;
  }

  // must be added to the same worker thread
  EventQueueWorker* worker = SBCFactory::subnot_processor.getWorker();
  if(!worker) return -1;

  worker->startEventQueue(a_leg);
  worker->startEventQueue(b_leg);
  
  return 0;
}
