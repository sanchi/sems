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
#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "AmThread.h"
#include "AmApi.h"
#include "DSMModule.h"

#define MOD_CLS_NAME ModHttpd

DECLARE_MODULE_BEGIN(MOD_CLS_NAME);
  int preload();
DECLARE_MODULE_END;


/* class HttpServer : public AmDynInvokeFactory, public AmDynInvoke { */

/*  public: */
/*   DECLARE_MODULE_INSTANCE(HttpServer); */

/*   AmDynInvoke* getInstance() { return this; } */
/*   void invoke(const string& method,  */
/* 	      const AmArg& args, AmArg& ret); */

/*   HttpServer(const string& mod_name); */
/*   ~HttpServer(); */

/*   int onLoad(); */
/* }; */

#endif
