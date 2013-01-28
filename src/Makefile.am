INCLUDES    = $(all_includes) $(KDE_INCLUDES)/tde
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_ldapmanager.la

kcm_ldapmanager_la_SOURCES = ldapmgr.cpp ldapconfigbase.ui userconfigbase.ui groupconfigbase.ui serviceconfigbase.ui userconfigdlg.cpp groupconfigdlg.cpp serviceconfigdlg.cpp
kcm_ldapmanager_la_LIBADD = -ltdeio $(LIB_TDEUI) -lldap -ltdeldap
kcm_ldapmanager_la_LDFLAGS = -avoid-version -module -no-undefined \
	$(all_libraries)


xdg_apps_DATA = ldapmanager.desktop


messages: rc.cpp
	$(EXTRACTRC) `find -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmldapmanager.pot

