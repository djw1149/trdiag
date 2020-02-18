RELEASE=beta
H=/usr/djw/router/$(RELEASE)/h
#CFLAGS = -g -I/usr/include -I../../h
CFLAGS = -O -I/usr/include -I$(H) 
CC = cc
# Fix this when the new hashed host table software is really installed
# HDB = -lhdb
# HDB = libnet.a
# HDB = -lnet

RIOBJS = switch.o showdevcnt.o dumpdevcnt.o addmap.o hash.o dispmem.o \
	 rsutil.o rcp.o

DIAGOBJS = Dil.o Dtr.o

####ELOBJS =  errorlog.o rcp.o rcputil.o netmap.o hash.o

first:

all: arplog errorlog iplog ralarm rdiag rinfo xfrdiag

test : $(RIOBJS) test.o
	$(CC) $(CFLAGS) test.o $(RIOBJS) $(HDB) -o test

ralarm : ralarm.o rsutil.o rcp.o 
	rm -f ralarm
	$(CC) $(CFLAGS) ralarm.o rsutil.o rcp.o $(HDB) -o ralarm

rpound : rpound.o rcp.o 
	$(CC) $(CFLAGS) rpound.o rcp.o $(HDB) -o rpound

rinfo : rinfo.o $(RIOBJS)
	rm -f rinfo
	$(CC) $(CFLAGS) rinfo.o $(RIOBJS) $(HDB) -o rinfo
	
errorlog : errorlog.o rsutil.o elutil.o rcp.o dispmem.o
	rm -f errorlog
	$(CC) $(CFLAGS) errorlog.o rsutil.o elutil.o rcp.o dispmem.o \
	$(HDB) -o errorlog

iplog : iplog.o rsutil.o elutil.o rcp.o dispmem.o
	rm -f iplog
	$(CC) $(CFLAGS) iplog.o rsutil.o elutil.o rcp.o dispmem.o $(HDB) \
	-o iplog

arplog : arplog.o rsutil.o elutil.o rcp.o
	rm -f arplog
	$(CC) $(CFLAGS) arplog.o rsutil.o elutil.o rcp.o $(HDB) -o arplog

rdiag : rdiag.o rcp.o rd_utils.o showdevcnt.o $(DIAGOBJS)
	rm -f rdiag
	$(CC) $(CFLAGS) rdiag.o rcp.o rd_utils.o showdevcnt.o $(DIAGOBJS) \
	$(HDB)  -o rdiag

nrdiag : nrdiag.o rcp.o nrd_utils.o showdevcnt.o $(DIAGOBJS)
	rm -f nrdiag
	$(CC) $(CFLAGS) nrdiag.o rcp.o nrd_utils.o showdevcnt.o $(DIAGOBJS) \
	$(HDB)  -o nrdiag

xfrdiag : xfrdiag.o rcp.o rd_utils.o showdevcnt.o $(DIAGOBJS)
	rm -f xfrdiag
	$(CC) $(CFLAGS) xfrdiag.o rcp.o rd_utils.o showdevcnt.o $(DIAGOBJS) \
	$(HDB) libxf.a -lXt -lX -o xfrdiag

# the following dependencies all assume that we are executing make one
# level below the source file directory

Dil.o : ../Dil.c
	rm -f Dil.o
	$(CC) $(CFLAGS) -c ../Dil.c

Dtr.o : ../Dtr.c
	rm -f Dtr.o
	$(CC) $(CFLAGS) -c ../Dtr.c

addmap.o : ../addmap.c 
	rm -f addmap.o
	$(CC) $(CFLAGS) -c ../addmap.c

arplog.o : ../arplog.c
	rm -f arplog.o
	$(CC) $(CFLAGS) -c ../arplog.c

dispmem.o : ../dispmem.c
	rm -f dispmem.o
	$(CC) $(CFLAGS) -c ../dispmem.c

dumpdevcnt.o : ../dumpdevcnt.c ../h/mch.h $(H)/proto.h \
	       $(H)/devstats.h
	rm -f dumpdevcnt.o
	$(CC) $(CFLAGS) -c ../dumpdevcnt.c

elutil.o  : ../elutil.c $(H)rcp.h
	rm -f elutil.o
	$(CC) $(CFLAGS) -c ../elutil.c

errorlog.o : ../errorlog.c
	rm -f errorlog.o
	$(CC) $(CFLAGS) -c ../errorlog.c

hash.o : ../hash.c ../h/hash.h
	rm -f hash.o
	$(CC) $(CFLAGS) -c ../hash.c

iplog.o : ../iplog.c
	rm -f iplog.o
	$(CC) $(CFLAGS) -c ../iplog.c

ralarm.o : ../ralarm.c $(H)/rtypes.h $(H)/rcp.h
	rm -f ralarm.o
	$(CC) $(CFLAGS) -c ../ralarm.c

rcp.o  : ../rcp.c $(H)/rcp.h ../debug/rcp.h
	rm -f rcp.o
	$(CC) $(CFLAGS) -c ../rcp.c

rdiag.o : ../rdiag.c ../h/rdiag.h
	rm -f rdiag.o
	$(CC) $(CFLAGS) -c ../rdiag.c

nrdiag.o : ../nrdiag.c ../h/rdiag.h
	rm -f nrdiag.o
	$(CC) $(CFLAGS) -c ../nrdiag.c

xfrdiag.o : ../xfrdiag.c ../h/rdiag.h
	rm -f xfrdiag.o
	$(CC) $(CFLAGS) -c ../xfrdiag.c

rdutil.o  : ../rdutil.c $(H)/rcp.h
	rm -f rdutil.o
	$(CC) $(CFLAGS) -c ../rdutil.c

rd_utils.o : ../rd_utils.c $(H)/rtypes.h ../h/rdiag.h
	rm -f rd_utils.o
	$(CC) $(CFLAGS) -c ../rd_utils.c

nrd_utils.o : ../nrd_utils.c $(H)/rtypes.h ../h/rdiag.h
	rm -f nrd_utils.o
	$(CC) $(CFLAGS) -c ../nrd_utils.c

rinfo.o : ../rinfo.c $(H)/proto.h $(H)/devstats.h
	rm -f rinfo.o
	$(CC) $(CFLAGS) -c ../rinfo.c

rpound.o : ../rpound.c $(H)/rtypes.h $(H)/rcp.h
	rm -f rpound.o
	$(CC) $(CFLAGS) -c ../rpound.c

rsutil.o  : ../rsutil.c $(H)/rcp.h
	rm -f rsutil.o
	$(CC) $(CFLAGS) -c ../rsutil.c

showdevcnt.o : ../showdevcnt.c ../h/mch.h $(H)/proto.h \
	       $(H)/devstats.h
	rm -f showdevcnt.o
	$(CC) $(CFLAGS) -c ../showdevcnt.c

switch.o : ../switch.c
	rm -f switch.o
	$(CC) $(CFLAGS) -c ../switch.c

test.o : ../test.c
	rm -f test.o
	$(CC) $(CFLAGS) -c ../test.c

