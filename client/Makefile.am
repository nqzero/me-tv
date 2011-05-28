gladedir = $(datadir)/me-tv/glade
glade_DATA = me-tv.ui \
	me-tv.xpm

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(ME_TV_CLIENT_CFLAGS) \
	$(SQLITE_CFLAGS) \
	$(VLC_LIB_CFLAGS) \
	$(WARN_CXXFLAGS) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" 

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = me-tv-client

me_tv_client_SOURCES = \
	../config.h \
	auto_record_dialog.cc \
	auto_record_dialog.h \
	configuration_manager.cc \
	configuration_manager.h \
	channels_dialog.cc \
	channels_dialog.h \
	epg_event_search_dialog.cc \
	epg_event_search_dialog.h \
	epg_event_dialog.cc \
	epg_event_dialog.h \
	gstreamer_engine.h \
	gstreamer_engine.cc \
	me-tv-client.cc \
	me-tv-client.h \
	main_window.cc \
	main_window.h \
	preferences_dialog.cc \
	preferences_dialog.h \
	engine.h \
	engine.cc \
	xine_engine.h \
	xine_engine.cc \
	vlc_engine.h \
	vlc_engine.cc \
	scan_dialog.cc \
	scan_dialog.h \
	scheduled_recordings_dialog.cc \
	scheduled_recordings_dialog.h

me_tv_client_CXXFLAGS = \
	-I/usr/include/liveMedia \
	-I/usr/include/groupsock \
	-I/usr/include/BasicUsageEnvironment \
	-I/usr/include/UsageEnvironment

me_tv_client_LDFLAGS = \
	-Wl,--as-needed `getconf \
	LFS_LDFLAGS` `getconf \
	LFS64_LDFLAGS`

me_tv_client_LDADD = \
	../common/libmetvcommon.a \
	$(ME_TV_CLIENT_LIBS) \
	-lX11 \
	/usr/lib/libliveMedia.a \
	/usr/lib/libgroupsock.a \
	/usr/lib/libUsageEnvironment.a \
	/usr/lib/libBasicUsageEnvironment.a

EXTRA_DIST = $(glade_DATA)

