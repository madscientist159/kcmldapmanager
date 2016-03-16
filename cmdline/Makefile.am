INCLUDES= $(all_includes) $(KDE_INCLUDES)/tde $(KRB5_CFLAGS)

bin_PROGRAMS = tdeldapmanager

tdeldapmanager_SOURCES = main.cpp

tdeldapmanager_METASOURCES = AUTO
tdeldapmanager_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_QT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor -ltdeldap $(KRB5_LIBS)

KDE_OPTIONS = nofinal
