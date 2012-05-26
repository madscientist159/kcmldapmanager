INCLUDES    = $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_ldap.la

kcm_ldap_la_SOURCES = ldap.cpp ldapconfigbase.ui
kcm_ldap_la_LIBADD = -lkio $(LIB_TDEUI)
kcm_ldap_la_LDFLAGS = -avoid-version -module -no-undefined \
	$(all_libraries)


xdg_apps_DATA = ldap.desktop


messages: rc.cpp
	$(EXTRACTRC) `find -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmldap.pot

