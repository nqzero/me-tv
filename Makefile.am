SUBDIRS = common console client server po

desktopdir = $(datadir)/applications
desktop_in_files = client/me-tv.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = \
        client/me-tv.xpm \
        client/me-tv-recording.xpm

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = client/me-tv.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

man_MANS = \
	client/me-tv-client.1 \
	server/me-tv-server.1

EXTRA_DIST = \
	AUTHORS\
	ChangeLog\
	INSTALL\
	client/me-tv-client.1 \
	server/me-tv-server.1 \
	$(desktop_in_files) \
	$(schemas_in_files) \
	$(pixmaps_DATA) \
	intltool-update.in \
	intltool-merge.in \
	intltool-extract.in

if GCONF_SCHEMAS_INSTALL
install-data-local: $(schemas_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
endif

# Copy all the spec files. Of course, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

CLEANFILES = $(desktop_DATA) $(schemas_DATA)

DISTCLEANFILES = intltool-extract intltool-merge intltool-update

AM_CPPFLAGS = 
