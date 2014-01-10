plug_in_name = mod_httpd

DSMPATH ?= ../..

# module_ldflags = -lmicrohttpd 
# module_cflags  = -I$(DSMPATH)

#module_ldflags = libmicrohttpd/libmicrohttpd-0.4.6/src/daemon/.libs/libmicrohttpd.a
#module_cflags  = -I libmicrohttpd/libmicrohttpd-0.4.6/src/include

module_ldflags = -L/usr/local/lib -lmicrohttpd
module_cflags  = -I$(DSMPATH) -I/usr/local/include


COREPATH ?=$(DSMPATH)/../../core
lib_full_name = $(DSMPATH)/mods/lib/$(lib_name)
include $(DSMPATH)/mods/Makefile.dsm_module

# COREPATH ?=../../core
# include $(COREPATH)/plug-in/Makefile.app_module
