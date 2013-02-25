/*
 * Copyright (C) 2013 Stefan Sayer
 *
 * This file is part of SEMS, a free SIP media server.
 *
 * SEMS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * For a license to use the sems software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * SEMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ModHttpd.h"

#include "AmPlugIn.h"
#include "log.h"
#include "AmConfigReader.h"
#include "AmUtils.h"
#include "AmArg.h"
#include "AmSession.h"
#include "AmEventDispatcher.h"

#include "jsonArg.h"
#include "AmUAC.h"
#include "DSM.h"

#define MOD_NAME "http"

#define HTTP_PORT   7090 // default port


#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// EXPORT_MODULE_FACTORY(HttpServer)
// DEFINE_MODULE_INSTANCE(HttpServer, MOD_NAME)

// HttpServer::HttpServer(const string& mod_name)
// : AmDynInvokeFactory(mod_name)
// {
// }

// HttpServer::~HttpServer() {
// }

// void HttpServer::invoke(const string& method, const AmArg& args, 
// 				AmArg& ret)
// {

// }

MOD_ACTIONEXPORT_BEGIN(MOD_CLS_NAME)
MOD_ACTIONEXPORT_END
MOD_CONDITIONEXPORT_NONE(MOD_CLS_NAME)

SC_EXPORT(MOD_CLS_NAME);

#define PAGE "<html><head><title>SEMS embedded http server</title>"\
             "</head><body>SEMS embedded http server</body></html>\n"

#define NOT_FOUND_ERROR "<html><head><title>404 Not found</title></head><body>404 Not found</body></html>\n"

struct RequestHandlerContext {
  struct MHD_PostProcessor * pp;
  map<string, string> form_values;
  RequestHandlerContext() : pp(NULL) { };
};

static int
post_iterator (void *cls,
	       enum MHD_ValueKind kind,
	       const char *key,
	       const char *filename,
	       const char *content_type,
	       const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct RequestHandlerContext *ctx = (RequestHandlerContext *)cls;
  ctx->form_values[string(key)] += string(data+off, size);
  return MHD_YES;
}

static int create_404_response(struct MHD_Connection * connection) {
  int ret;
  struct MHD_Response * response;
  response = MHD_create_response_from_data (strlen (NOT_FOUND_ERROR),
					    (void *) NOT_FOUND_ERROR,
					    MHD_NO, MHD_NO);
 
  ret = MHD_queue_response(connection,
			   MHD_HTTP_NOT_FOUND,
			   response);

  MHD_destroy_response(response);
  return ret;
}

static int create_str_response(struct MHD_Connection * connection, unsigned short http_status, const string& body) {
  int ret;
  struct MHD_Response * response;
  response = MHD_create_response_from_data (body.size(),
					    (void *) body.c_str(),
					    MHD_NO, MHD_YES);
 
  ret = MHD_queue_response(connection,
			   http_status,
			   response);

  MHD_destroy_response(response);
  return ret;
}

static int create_simple_str_response(struct MHD_Connection * connection, unsigned short http_status, const string& msg) {
  return create_str_response(connection, http_status,
			     "<html><head><title>"+int2str(http_status)+" "+msg+"</title></head>"
			     "<body>"+int2str(http_status)+" "+msg+"</body></html>\n");
}

static int create_response(void * cls,
		    struct MHD_Connection * connection,
		    const char * url,
		    const char * method,
                    const char * version,
		    const char * upload_data,
		    size_t * upload_data_size,
                    void ** ptr) {
  const char * page = (const char*)cls;
  struct MHD_Response * response;
  int ret;
  
  // DBG("method = '%s'\n", method);
  // DBG("url = '%s'\n", url);
  // DBG("upload_data_size = %zd, data = '%s'\n", *upload_data_size, upload_data);

  struct RequestHandlerContext* ctx = (RequestHandlerContext*) *ptr;

  // struct MHD_PostProcessor * pp = *ptr;

  if (0 == strcmp("POST", method)) {

    if (ctx == NULL) {
      // only accept these URLs
      if ((0 != strncmp("/session/create", url, sizeof("/session/create"))) &&
	  (0 != strncmp("/session/ccp", url, sizeof("/session/ccp")))) {
	return create_404_response(connection);
      }
      
      ctx = new RequestHandlerContext();
      ctx->pp = MHD_create_post_processor(connection, 1024, &post_iterator, ctx);
      if (NULL == ctx->pp) {
	delete ctx;
	return MHD_NO;
      }

      *ptr = ctx;
      return MHD_YES;
    }

    // process data 
    if (*upload_data_size) {
      MHD_post_process(ctx->pp, upload_data, *upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
    }

    // finished with data for this request: process values
    DBG("received request to %s\n", url);
    for (map<string, string>::iterator it = ctx->form_values.begin(); it !=ctx->form_values.end(); it++) {
      DBG(" '%s' = '%s'\n", it->first.c_str(), it->second.c_str());
    } 

    // sessionid
    // msgType
    // msg

    if (0 == strcmp("/session/create", url)) {
      if (ctx->form_values["msgType"] != "CreateCall") {
	return create_404_response(connection); // todo: better errors
      }

      AmArg msg;
      if (!json2arg(ctx->form_values["msg"], msg)) {
	DBG("no valid json message found in '%s'\n", ctx->form_values["msg"].c_str());
	return create_404_response(connection); // todo: better errors
      }
      DBG("creating new call from msg: '%s'\n", AmArg::print(msg).c_str());

      if (!msg.hasMember("from") || !msg.hasMember("to") || !msg.hasMember("timeout") ||
	  !msg.hasMember("scCallId")) {
	return create_simple_str_response(connection, 400, "Call could not be created: mandatory parameter missing");
      }

      try {
	string ltag = AmSession::getNewId();
	string user = ltag;
	string app_name = "sc_outgoing"; // todo: configurable
	string from = msg["from"].asCStr();
	string from_uri = from;      
	string to = msg["to"].asCStr();
	string r_uri = to;
	string hdrs;
	if  (msg.hasMember("headers") && isArgStruct(msg["headers"])) {
	  for (AmArg::ValueStruct::const_iterator it = msg["headers"].begin(); it != msg["headers"].end(); it++) {
	    if (isArgCStr(it->second))
	      hdrs += it->first+COLSP+it->second.asCStr()+CRLF;
	  }
	} 
      
	AmArg* sess_params = new AmArg();
	// for auth, push another AmArg here with UACAuthCred

	// for only variables, just the struct
	(*sess_params)["timeout"] =  int2str(msg["timeout"].asInt()); // $timeout
	(*sess_params)["scCallId"] = msg["scCallId"];                 // pass SC call id as $scCallId

	string session_id = AmUAC::dialout(user, app_name, r_uri, from, from_uri, to, ltag, hdrs, sess_params);

	if (session_id.empty()) {
	  return create_simple_str_response(connection, 500, "Call could not be created");
	} else {
	  return create_simple_str_response(connection, 200, "Call created with msCallId "+ltag+"(Call-ID "+session_id+")");
	}
      } catch (const AmArg::OutOfBoundsException& e) {
	return create_simple_str_response(connection, 400, "Call could not be created: out of bounds in parameters");
      } catch (const AmArg::TypeMismatchException& e) {
	return create_simple_str_response(connection, 400, "Call could not be created: type mismatch in parameters");
      } catch (...) {
	return create_simple_str_response(connection, 500, "Call could not be created, unspecified error");
      }

    } else if (0 == strcmp("/session/ccp", url)) {

      string ltag = ctx->form_values["sessionid"];
      string msgType = ctx->form_values["msgType"];

      AmArg msg;
      if (!json2arg(ctx->form_values["msg"], msg)) {
	DBG("no valid json message found in '%s'\n", ctx->form_values["msg"].c_str());
	return create_simple_str_response(connection, 400, "CCP command could not be executed: "
					  "no valid json in msg parameter");
      }
      
      if (ltag.empty()) {
	DBG("no sessionid\n");
	return create_simple_str_response(connection, 400, "CCP command could not be executed: "
					  "no sessionid");
      }

      DSMEvent* ev = new DSMEvent();
      ev->params = ctx->form_values; // copy all form values (??? does that make sense?)
      
      // ??? may overwrite form_values as it's the same namespace
      for (AmArg::ValueStruct::const_iterator it = msg.begin(); it != msg.end(); it++) {  
	ev->params[it->first] = AmArg::print(it->second);
      }

      if (AmEventDispatcher::instance()->post(ltag, ev)) {
	return create_simple_str_response(connection, 200, "CCP command queued");
      } else {
	delete ev; // unlike AmSessionContainer, AmEventDispatcher doesn't delete unposted events
	return create_simple_str_response(connection, 404, "CCP command could not be executed: "
					  "Session not found");
      }
    }

    response = MHD_create_response_from_data(strlen(page),
					   (void*) page,
					     MHD_NO,
					     MHD_NO);
    ret = MHD_queue_response(connection,
			     MHD_HTTP_OK,
			     response);

    MHD_destroy_response(response);

    *ptr = NULL; /* clear context pointer */
    MHD_destroy_post_processor(ctx->pp);
    delete ctx;

    return ret;
  }

  // other methods


  *ptr = NULL; /* clear context pointer */

  return create_404_response(connection);
  return ret;
}

int MOD_CLS_NAME::preload() {
  DBG("Starting http server on port %u\n", HTTP_PORT);
  struct MHD_Daemon * d;
 ///void* opt = (void*);

  d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, //MHD_USE_THREAD_PER_CONNECTION,
		       HTTP_PORT,
		       NULL,
		       NULL,
		       &create_response,
		       (void*)PAGE,
		       MHD_OPTION_END);
  if (d == NULL)
    return -1;

  return 0;
}
