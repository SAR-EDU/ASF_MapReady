CFLAGS += $(HDF5_CFLAGS)
CFLAGS += $(GEOTIFF_CFLAGS)
CFLAGS += $(HDF5_CFLAGS)
# Makefile for fine_coregister
# Module Authors: Rob Fatland, Mike Shindle, Tom Logan, Orion Lawlor,
#                 Mark Ayers, Patrick Denny

include ../../make_support/system_rules

LIBS  = \
	$(LIBDIR)/asf_meta.a \
	$(GSL_LIBS) \
	$(LIBDIR)/libasf_proj.a \
	$(LIBDIR)/asf.a \
	$(PROJ_LIBS) \
	$(LIBDIR)/libifm.a \
	$(LIBDIR)/asf_fft.a \
	$(XML_LIBS) \
	-lm

OBJS  = fft_corr.o \
	coherence.o \
	coregister_fine.o

all: coregister_fine clean

coregister_fine: $(OBJS)
	$(CC) $(CFLAGS) -o coregister_fine $(OBJS) $(LIBS) $(LDFLAGS)
	mv coregister_fine$(BIN_POSTFIX) $(BINDIR)
	cp coregister_fine.1 ../../man/man1

clean:
	rm -f core *.o
