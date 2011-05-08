AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(ME_TV_SERVER_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = me-tv-server

me_tv_server_SOURCES = \
	../config.h \
	me-tv-server.cc 

me_tv_server_LDFLAGS = 

me_tv_server_LDADD = \
	../common/libmetvcommon.a \
	$(ME_TV_SERVER_LIBS)

