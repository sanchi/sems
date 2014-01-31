plug_in_name = mod_httpd

DSMPATH ?= ../..

# module_ldflags = -lmicrohttpd 
# module_cflags  = -I$(DSMPATH)

#or, under /usr/local:
#module_ldflags = -L/usr/local/lib -lmicrohttpd
#module_cflags  = -I$(DSMPATH) -I/usr/local/include

mhd_dir=libmicrohttpd-0.9.33/
mhd_obj_files=$(wildcard $(mhd_dir)/src/microhttpd/.libs/*.o)
module_ldflags = -lgnutls -lrt
module_extra_objs=$(mhd_obj_files)
module_cflags  = -I$(DSMPATH)  -I $(mhd_dir)/src/include


COREPATH ?=$(DSMPATH)/../../core
lib_full_name = $(DSMPATH)/mods/lib/$(lib_name)
include $(DSMPATH)/mods/Makefile.dsm_module

# COREPATH ?=../../core
# include $(COREPATH)/plug-in/Makefile.app_module

mhd_obj_files:
	make -C $(mhd_dir)