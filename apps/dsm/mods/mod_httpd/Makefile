plug_in_name = mod_httpd

DSMPATH ?= ../..

module_ldflags = -lmicrohttpd 
module_cflags  = -I$(DSMPATH)


COREPATH ?=$(DSMPATH)/../../core
lib_full_name = $(DSMPATH)/mods/lib/$(lib_name)
include $(DSMPATH)/mods/Makefile.dsm_module

# COREPATH ?=../../core
# include $(COREPATH)/plug-in/Makefile.app_module
