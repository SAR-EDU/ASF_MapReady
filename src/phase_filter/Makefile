CFLAGS += $(HDF5_CFLAGS)
CFLAGS += $(GEOTIFF_CFLAGS)
CFLAGS += $(HDF5_CFLAGS)
######################################################################
# Makefile for 		:  phase_filt.c 
# Module Author 	:  Orion Lawlor
# Module Version	:  1.0
######################################################################

include ../../make_support/system_rules

LIBS  = $(LIBDIR)/asf_fft.a \
	$(LIBDIR)/asf_meta.a \
	$(GSL_LIBS) \
	$(LIBDIR)/libasf_proj.a \
	$(PROJ_LIBS) \
	$(LIBDIR)/asf.a \
	$(XML_LIBS)
LIBC = $(LIBS) -lm 

OBJLIB =  filter.o blend.o

all: phase_filter
	- rm *.o

phase_filter:	$(OBJLIB) $(LIBS)
	$(CC) $(CFLAGS) -o phase_filter $(OBJLIB) $(LIBC) $(LDFLAGS)
	mv phase_filter$(BIN_POSTFIX) $(BINDIR)
	cp phase_filter.1 ../../man/cat1
