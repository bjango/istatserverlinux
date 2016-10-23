
dist_man_MANS = istatserver.1 istatserver.conf.5
EXTRA_DIST = istatserver.conf istatserver_generated.conf

#dist_sysconf_DATA = istatserver.conf

MKDIR=/bin/mkdir
MV=/bin/mv
ADDUSER=/usr/sbin/useradd
ADDGROUP=/usr/sbin/groupadd
ADDUSERPW=/usr/sbin/pw
MKGROUP=/usr/bin/mkgroup
OLD_CONFIG_FILE = $(sysconfdir)/istat.conf
DEFAULT_CONFIG_FILE = $(sysconfdir)/istatserver/istatserver.conf
DEFAULT_CONFIG_FILE_MV = $(sysconfdir)/istatserver/istatserver.conf.old
DEFAULT_CONFIG_GENERATED_FILE = $(sysconfdir)/istatserver/istatserver_generated.conf
CERT_FILE = $(sysconfdir)/istatserver/cert.pem
KEY_FILE = $(sysconfdir)/istatserver/key.pem
CACHE_FILE = $(sysconfdir)/istatserver/clients.dat

install-exec-hook:
	@if test -f "$(ADDUSERPW)" ; then \
		$(ADDUSERPW) groupadd istat  || true ; \
		$(ADDUSERPW) useradd istat -g istat  || true ; \
	fi

	@if test -f "$(ADDGROUP)" ; then \
		$(ADDGROUP) istat  || true ; \
	fi

	@if test -f "$(MKGROUP)" ; then \
		$(MKGROUP) istat  || true ; \
	fi

	@if test -f "$(ADDUSER)" ; then \
		$(ADDUSER) --system -g istat istat  || true ; \
		$(ADDUSER) -g istat istat  || true ; \
	fi

	$(MKDIR) -p "$(DESTDIR)$(sysconfdir)";
	$(MKDIR) -p "$(DESTDIR)$(sysconfdir)/istatserver/"
	$(MKDIR) -p "/var/run/istatserver"  || true ;
	chown istat:istat "/var/run/istatserver"  || true ;
	chown istat:istat "$(DESTDIR)$(sysconfdir)/istatserver/"  || true ;
	chmod 755 "$(DESTDIR)$(sysconfdir)/istatserver/"  || true ;

	@if test -f "$(DESTDIR)$(OLD_CONFIG_FILE)" ; then \
		$(MV) "$(DESTDIR)$(OLD_CONFIG_FILE)" "$(DESTDIR)$(DEFAULT_CONFIG_FILE)" ; \
	fi

	@if test -f "/var/cache/istat/clients.dat" ; then \
		$(MV) "/var/cache/istat/clients.dat" "$(DESTDIR)$(sysconfdir)/istatserver/clients.dat" ; \
		chown istat:istat "$(DESTDIR)$(sysconfdir)/istatserver/clients.dat"  || true ; \
	fi

	@if test -f "$(DESTDIR)$(DEFAULT_CONFIG_FILE)" ; then \
			$(MV) "$(DESTDIR)$(DEFAULT_CONFIG_FILE)" "$(DESTDIR)$(DEFAULT_CONFIG_FILE_MV)" ; \
	        $(INSTALL_DATA) "$(srcdir)/istatserver.conf" "$(DESTDIR)$(DEFAULT_CONFIG_FILE)"; \
	        echo "$@ running conf updater"; \
			../conf/istatserverconf || true ; \
			rm "$(DESTDIR)$(DEFAULT_CONFIG_FILE_MV)" || true ; \
			chown istat:istat "$(DESTDIR)$(DEFAULT_CONFIG_FILE)"  || true ; \
	else \
	        echo "$(INSTALL_DATA) istatserver.conf $(DESTDIR)$(DEFAULT_CONFIG_FILE)"; \
	        $(INSTALL_DATA) "$(srcdir)/istatserver.conf" "$(DESTDIR)$(DEFAULT_CONFIG_FILE)"; \
	        echo "$@ running conf updater and generating passcode"; \
			../conf/istatserverconf -p || true ; \
			chown istat:istat "$(DESTDIR)$(DEFAULT_CONFIG_FILE)"  || true ; \
	fi

	@if test -f "$(DESTDIR)$(DEFAULT_CONFIG_GENERATED_FILE)" ; then \
	        echo "$@ will not overwrite existing $(DESTDIR)$(DEFAULT_CONFIG_GENERATED_FILE)" ; \
	else \
	        echo "$(INSTALL_DATA) istatserver_generated.conf $(DESTDIR)$(DEFAULT_CONFIG_GENERATED_FILE)"; \
	        $(INSTALL_DATA) "$(srcdir)/istatserver_generated.conf" "$(DESTDIR)$(DEFAULT_CONFIG_GENERATED_FILE)"; \
	fi

uninstall-hook:
	@if test -f "$(DESTDIR)$(DEFAULT_CONFIG_GENERATED_FILE)" ; then \
		rm "$(DESTDIR)$(DEFAULT_CONFIG_GENERATED_FILE)"; \
	fi

	@if test -f "$(DESTDIR)$(DEFAULT_CONFIG_FILE)" ; then \
		rm "$(DESTDIR)$(DEFAULT_CONFIG_FILE)"; \
	fi

	@if test -f "$(DESTDIR)$(CERT_FILE)" ; then \
		rm "$(DESTDIR)$(CERT_FILE)"; \
	fi

	@if test -f "$(DESTDIR)$(KEY_FILE)" ; then \
		rm "$(DESTDIR)$(KEY_FILE)"; \
	fi

	@if test -f "$(DESTDIR)$(CACHE_FILE)" ; then \
		rm "$(DESTDIR)$(CACHE_FILE)"; \
	fi
