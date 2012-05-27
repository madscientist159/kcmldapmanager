INCLUDES    = $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_ldapmanager.la

kcm_ldapmanager_la_SOURCES = ldap.cpp ldapconfigbase.ui userconfigbase.ui groupconfigbase.ui libtdeldap.cpp ldaplogindlgbase.ui ldaplogindlg.cpp ldappasswddlg.cpp
kcm_ldapmanager_la_LIBADD = -lkio $(LIB_TDEUI)
kcm_ldapmanager_la_LDFLAGS = -avoid-version -module -no-undefined \
	$(all_libraries)


xdg_apps_DATA = ldapmanager.desktop


messages: rc.cpp
	$(EXTRACTRC) `find -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmldapmanager.pot

