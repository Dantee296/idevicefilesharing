AM_CFLAGS =			\
	$(GLOBAL_CFLAGS)	\
	$(libimobiledevice_CFLAGS)	\
	$(libglib2_CFLAGS)	\
	$(libplist_CFLAGS)	

AM_LDFLAGS =			\
	$(libimobiledevice_LIBS)	\
	$(libglib2_LIBS)	\
	$(libplist_LIBS)	

bin_PROGRAMS = idevicefilesharing

idevicefilesharing_SOURCES = idevicefilesharing.c
idevicefilesharing_CFLAGS = $(AM_CFLAGS)
idevicefilesharing_LDFLAGS = $(AM_LDFLAGS)