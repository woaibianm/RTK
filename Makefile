# makefile for rtkpost_main  (MinGW-W64 gcc, Windows)
# 用法: mingw32-make -f Makefile  或 直接双击 build.bat
CC      = gcc
SRC     = ../RTKLIB-rtklib_2.4.3/src

# 系统使能宏: GLONASS/GALILEO/QZSS/北斗/NavIC + 5频 + 跟踪
OPTS    = -DTRACE -DENAGLO -DENAQZS -DENAGAL -DENACMP -DENAIRN -DNFREQ=5
CFLAGS  = -O2 -Wall -Wno-unused-but-set-variable -I$(SRC) $(OPTS)
LDLIBS  = -lm -lwinmm

all        : rtkpost_main.exe
rtkpost_main.exe : rtkpost_main.o rtkcmn.o rinex.o rtkpos.o postpos.o solution.o
rtkpost_main.exe : lambda.o geoid.o sbas.o preceph.o pntpos.o ephemeris.o options.o
rtkpost_main.exe : ppp.o ppp_ar.o rtcm.o rtcm2.o rtcm3.o rtcm3e.o ionex.o tides.o
	$(CC) $(CFLAGS) rtkpost_main.o rtkcmn.o rinex.o rtkpos.o postpos.o solution.o \
	    lambda.o geoid.o sbas.o preceph.o pntpos.o ephemeris.o options.o \
	    ppp.o ppp_ar.o rtcm.o rtcm2.o rtcm3.o rtcm3e.o ionex.o tides.o \
	    $(LDLIBS) -o $@

rtkpost_main.o : rtkpost_main.c
	$(CC) -c $(CFLAGS) rtkpost_main.c
rtkcmn.o    : $(SRC)/rtkcmn.c
	$(CC) -c $(CFLAGS) $(SRC)/rtkcmn.c
rinex.o     : $(SRC)/rinex.c
	$(CC) -c $(CFLAGS) $(SRC)/rinex.c
rtkpos.o    : $(SRC)/rtkpos.c
	$(CC) -c $(CFLAGS) $(SRC)/rtkpos.c
postpos.o   : $(SRC)/postpos.c
	$(CC) -c $(CFLAGS) $(SRC)/postpos.c
solution.o  : $(SRC)/solution.c
	$(CC) -c $(CFLAGS) $(SRC)/solution.c
lambda.o    : $(SRC)/lambda.c
	$(CC) -c $(CFLAGS) $(SRC)/lambda.c
geoid.o     : $(SRC)/geoid.c
	$(CC) -c $(CFLAGS) $(SRC)/geoid.c
sbas.o      : $(SRC)/sbas.c
	$(CC) -c $(CFLAGS) $(SRC)/sbas.c
preceph.o   : $(SRC)/preceph.c
	$(CC) -c $(CFLAGS) $(SRC)/preceph.c
pntpos.o    : $(SRC)/pntpos.c
	$(CC) -c $(CFLAGS) $(SRC)/pntpos.c
ephemeris.o : $(SRC)/ephemeris.c
	$(CC) -c $(CFLAGS) $(SRC)/ephemeris.c
options.o   : $(SRC)/options.c
	$(CC) -c $(CFLAGS) $(SRC)/options.c
ppp.o       : $(SRC)/ppp.c
	$(CC) -c $(CFLAGS) $(SRC)/ppp.c
ppp_ar.o    : $(SRC)/ppp_ar.c
	$(CC) -c $(CFLAGS) $(SRC)/ppp_ar.c
rtcm.o      : $(SRC)/rtcm.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm.c
rtcm2.o     : $(SRC)/rtcm2.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm2.c
rtcm3.o     : $(SRC)/rtcm3.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm3.c
rtcm3e.o    : $(SRC)/rtcm3e.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm3e.c
ionex.o     : $(SRC)/ionex.c
	$(CC) -c $(CFLAGS) $(SRC)/ionex.c
tides.o     : $(SRC)/tides.c
	$(CC) -c $(CFLAGS) $(SRC)/tides.c

# 依赖 rtklib.h
rtkpost_main.o rtkcmn.o rinex.o rtkpos.o postpos.o solution.o lambda.o : $(SRC)/rtklib.h
geoid.o sbas.o preceph.o pntpos.o ephemeris.o options.o ppp.o ppp_ar.o : $(SRC)/rtklib.h
rtcm.o rtcm2.o rtcm3.o rtcm3e.o ionex.o tides.o                          : $(SRC)/rtklib.h

clean:
	-$(RM) *.o rtkpost_main.exe *.pos *.trace

