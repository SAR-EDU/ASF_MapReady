CFLAGS += $(HDF5_CFLAGS)
CFLAGS += $(GEOTIFF_CFLAGS)
CFLAGS += $(HDF5_CFLAGS)
# Makefile for		: eleverr 
# Module Author		: Mike Shindle

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
OBJS  = eleverr.o 

eleverr: $(OBJS)
	$(CC) $(CFLAGS) -o eleverr $(OBJS) $(LIBS) $(LDFLAGS)
	rm *.o
	mv eleverr$(BIN_POSTFIX) $(BINDIR)
	cp eleverr.1 ../../man/cat1
