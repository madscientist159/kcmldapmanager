INCLUDES    = $(all_includes) $(KDE_INCLUDES)/tde $(KRB5_CFLAGS)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_ldapmanager.la

kcm_ldapmanager_la_SOURCES = ldapmgr.cpp ldapconfigbase.ui userconfigbase.ui groupconfigbase.ui machineconfigbase.ui serviceconfigbase.ui userconfigdlg.cpp groupconfigdlg.cpp machineconfigdlg.cpp serviceconfigdlg.cpp
kcm_ldapmanager_la_LIBADD = -ltdeio $(LIB_TDEUI) -lldap -ltdeldap
kcm_ldapmanager_la_LDFLAGS = -avoid-version -module -no-undefined \
	$(all_libraries) $(KRB5_LIBS)


xdg_apps_DATA = ldapmanager.desktop


messages: rc.cpp
	$(EXTRACTRC) `find -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmldapmanager.pot

