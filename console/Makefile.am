AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(ME_TV_CONSOLE_CFLAGS) \
	$(WARN_CXXFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

noinst_PROGRAMS = me-tv-console

me_tv_console_SOURCES = \
	../config.h \
	me-tv-console.cc 

me_tv_console_LDFLAGS = \
	-Wl,--as-needed `getconf \
	LFS_LDFLAGS` `getconf \
	LFS64_LDFLAGS`

me_tv_console_LDADD = \
	../common/libmetvcommon.a \
	$(ME_TV_CONSOLE_LIBS)

