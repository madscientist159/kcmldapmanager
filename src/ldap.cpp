/***************************************************************************
 *   Copyright (C) 2012 by Timothy Pearson                                 *
 *   kb9vqf@pearsoncomputing.net                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <tqlayout.h>

#include <klocale.h>
#include <kglobal.h>
#include <kparts/genericfactory.h>
#include <ksimpleconfig.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <klistview.h>
#include <kopenwith.h>
#include <kpropertiesdialog.h>
#include <kio/job.h>
#include <tqdir.h>
#include <tqheader.h>

#include "ldap.h"

typedef KGenericFactory<ldap, TQWidget> ldapFactory;

K_EXPORT_COMPONENT_FACTORY( kcm_ldap, ldapFactory("kcmldap"))

ldap::ldap(TQWidget *parent, const char *name, const TQStringList&)
    : KCModule(parent, name), myAboutData(0)
{
	// FIXME
	// Add UI base widget to 'this'
	
	load();
	
	KAboutData* about = new KAboutData("ldap", I18N_NOOP("TDE LDAP Manager"), "0.1",
		I18N_NOOP("TDE LDAP Manager Control Panel Module"),
		KAboutData::License_GPL,
		I18N_NOOP("(c) 2012 Timothy Pearson"), 0, 0);
	
	about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
	setAboutData( about );
};

ldap::~ldap() {
}

void ldap::load() {
	kgs = new KGlobalSettings();
	KStandardDirs *ksd = new KStandardDirs();
}

void ldap::defaults() {
	
}

void ldap::save() {
	
}

int ldap::buttons() {
	return KCModule::Apply|KCModule::Help;
}

TQString ldap::quickHelp() const
{
	return i18n("This module configures which LDAP realms TDE uses for authentication.");
}
