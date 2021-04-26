TOPDIR=./..


LIB=libeth_ctrl.a

LIBPATH=$(TOPDIR)/lib
LIBETH =/opt/ethercat/lib
CFLAGS= -I$(TOPDIR)/include/ -I$(TOPDIR)/comm/ -I./ -I/opt/ethercat/include
LDFLAGS= -L$(LIBPATH) -lrt -L$(LIBETH) -lethercat

#include ./.deps/ec_dc_user_example-main.Po
include $(TOPDIR)/include/Makefile.base 
