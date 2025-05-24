#
# makefile for gnss-sdrlib-pvt
#

# Use or not use RTLSDR and BLADERF
# 1:Use 0:Not Use
USE_RTLSDR=1
USE_BLADERF=1

SRC=../../src
RTKLIB=../../lib/rtklib
NMLLIB=../../lib/nml
RTLSDR=../../src/rcv/rtlsdr
BLADERF=../../src/rcv/bladerf
TTF=../../src/openSans

INCLUDE=-I$(SRC) -I$(RTKLIB) -I$(RTLSDR) -I$(BLADERF) -I$(NMLLIB) -I$(TTF)

CC=gcc
OPTIONS=-DSSE2_ENABLE -DFFTMTX

LIBS=-lfec -lusb-1.0 -lncurses

OBS= sdrmain.o sdrcmn.o sdracq.o sdrcode.o sdrinit.o sdrnav.o\
     sdrnav_gps.o sdrnav_sbs.o sdrpvt.o sdrrcv.o sdrtrk.o sdrsync.o sdrgui.o\
     nml.o nml_util.o rtkcmn.o
#OBS= sdrmain.o sdrcmn.o sdracq.o sdrcode.o sdrekf.o sdrinit.o sdrnav.o\
#     sdrnav_gps.o sdrnav_sbs.o sdrpvt.o sdrrcv.o sdrtrk.o sdrsync.o sdrgui.o\
#     nml.o nml_util.o rtkcmn.o

ifeq ($(USE_RTLSDR),1)
OPTIONS+=-DRTLSDR
LIBS+=-lrtlsdr
OBS+=rtlsdr.o convenience.o
endif

ifeq ($(USE_BLADERF),1)
OPTIONS+=-DBLADERF
LIBS+=-lbladeRF
OBS+=bladerf.o
endif

# Compile flag options:
# Set -O3 to -O0 to use GDB. Leave as -O3 for best real-time performance.
# Use -Wno-stringop-truncation to stop string warnings in rinex.c and others
# Add -g and -O0 or -O1 for running valgrind
#CFLAGS=-Wall -O0 -march=native $(INCLUDE) $(OPTIONS) -Wno-stringop-truncation -g
CFLAGS=-Wall -O3 -march=native $(INCLUDE) $(OPTIONS) -Wno-stringop-truncation
LDLIBS=-lm -lrt -lfftw3f -lfftw3f_threads -lpthread $(LIBS)

BIN= gnss-sdrlib-pvt
$(BIN): $(OBS)
	 $(CC) -o $(BIN) $(OBS) $(CFLAGS) $(LDLIBS)
	 @cp $(BIN) ../../bin

sdrmain.o: $(SRC)/sdrmain.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrmain.c
sdrcmn.o : $(SRC)/sdrcmn.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrcmn.c
sdracq.o : $(SRC)/sdracq.c
	$(CC) -c $(CFLAGS) $(SRC)/sdracq.c
sdrcode.o: $(SRC)/sdrcode.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrcode.c
#sdrekf.o: $(SRC)/sdrekf.c
#	$(CC) -c $(CFLAGS) $(SRC)/sdrekf.c
sdrgui.o: $(SRC)/sdrgui.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrgui.c
sdrinit.o: $(SRC)/sdrinit.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrinit.c
#sdrmsg.o: $(SRC)/sdrmsg.c
#	$(CC) -c $(CFLAGS) $(SRC)/sdrmsg.c
sdrnav.o : $(SRC)/sdrnav.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrnav.c
sdrnav_gps.o : $(SRC)/sdrnav_gps.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrnav_gps.c
sdrnav_sbs.o : $(SRC)/sdrnav_sbs.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrnav_sbs.c
sdrpvt.o: $(SRC)/sdrpvt.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrpvt.c
sdrrcv.o : $(SRC)/sdrrcv.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrrcv.c
sdrtrk.o : $(SRC)/sdrtrk.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrtrk.c
sdrsync.o : $(SRC)/sdrsync.c
	$(CC) -c $(CFLAGS) $(SRC)/sdrsync.c
rtkcmn.o   : $(RTKLIB)/rtkcmn.c
	$(CC) -c $(CFLAGS) $(RTKLIB)/rtkcmn.c
nml.o    : $(NMLLIB)/nml.c
	$(CC) -c $(CFLAGS) $(NMLLIB)/nml.c
nml_util.o    : $(NMLLIB)/nml_util.c
	$(CC) -c $(CFLAGS) $(NMLLIB)/nml_util.c
rtlsdr.o : $(RTLSDR)/rtlsdr.c
	$(CC) -c $(CFLAGS) $(RTLSDR)/rtlsdr.c
convenience.o : $(RTLSDR)/convenience.c
	$(CC) -c $(CFLAGS) $(RTLSDR)/convenience.c
bladerf.o : $(BLADERF)/bladerf.c
	$(CC) -c $(CFLAGS) $(BLADERF)/bladerf.c

sdrmain.o: $(SRC)/sdr.h
sdrcmn.o : $(SRC)/sdr.h
sdracq.o : $(SRC)/sdr.h
sdrcode.o: $(SRC)/sdr.h
#sdrekf.o: $(SRC)/sdr.h
sdrgui.o:  $(SRC)/sdr.h
sdrinit.o: $(SRC)/sdr.h
#sdrmsg.o: $(SRC)/sdr.h
sdrnav.o : $(SRC)/sdr.h
sdrnav_gps.o : $(SRC)/sdr.h
sdrnav_sbs.o : $(SRC)/sdr.h
sdrrcv.o : $(SRC)/sdr.h
sdrtrk.o : $(SRC)/sdr.h
sdrsync.o: $(SRC)/sdr.h
rtkcmn.o : $(SRC)/sdr.h
rtlsdr.o : $(SRC)/sdr.h
convenience.o : $(SRC)/sdr.h
bladerf.o : $(SRC)/sdr.h

clean:
	rm -f *.o $(BIN)
