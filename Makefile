TOPDIR=.

exclude_dirs= include lib

TARGET=cSPACEServer4.1

LIBPATH=$(TOPDIR)/lib
LIBETH=/opt/etherlib/lib
EXEPATH=$(TOPDIR)

CFLAGS=  -I$(TOPDIR)/include/ -I$(TOPDIR)/eth_ctrl/ -I$(TOPDIR)/net_recv/ -I$(TOPDIR)/comm/ -I/opt/ethercat/include
LDFLAGS= -L$(LIBPATH) -leth_ctrl -lnet_recv  -lcomm -lpthread -lrt -L$(LIBETH) -lethercat -lm

include $(TOPDIR)/include/Makefile.base
