CFLAGS += $(HDF5_CFLAGS)
CFLAGS += $(GEOTIFF_CFLAGS)
CFLAGS += $(HDF5_CFLAGS)
# Makefile for		: dem2phase
# Module Author		: Orion Lawlor

include ../../make_support/system_rules

LIBS  = \
	$(LIBDIR)/libasf_insar.a \
	$(LIBDIR)/asf_meta.a \
	$(GSL_LIBS) \
	$(LIBDIR)/libasf_proj.a \
	$(LIBDIR)/asf.a \
	$(PROJ_LIBS) \
	$(XML_LIBS) \
	-lm

OBJS = dem2phase.o

all: dem2phase
	rm *.o

dem2phase: $(OBJS)
	$(CC) $(CFLAGS) -o dem2phase $(OBJS) $(LIBS) $(LDFLAGS)
	mv dem2phase$(BIN_POSTFIX) $(BINDIR)
	cp dem2phase.1 ../../man/man1

clean:
	rm -f core ${OBJS} *.o
