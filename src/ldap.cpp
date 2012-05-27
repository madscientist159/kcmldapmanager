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

typedef KGenericFactory<LDAPConfig, TQWidget> LDAPConfigFactory;

K_EXPORT_COMPONENT_FACTORY( kcm_ldapmanager, LDAPConfigFactory("kcmldapmanager"))

LDAPConfig::LDAPConfig(TQWidget *parent, const char *name, const TQStringList&)
    : KCModule(parent, name), myAboutData(0)
{
	TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
	base = new LDAPConfigBase(this);
	layout->add(base);

	base->user_list->setAllColumnsShowFocus(true);
	base->user_list->setFullWidth(true);
	base->group_list->setAllColumnsShowFocus(true);
	base->group_list->setFullWidth(true);
	base->group_memberList->setAllColumnsShowFocus(true);
	base->group_memberList->setFullWidth(true);
	base->machine_list->setAllColumnsShowFocus(true);
	base->machine_list->setFullWidth(true);
	
	load();
	
	KAboutData* about = new KAboutData("ldap", I18N_NOOP("TDE LDAP Realm Manager"), "0.1",
		I18N_NOOP("TDE LDAP Realm Manager Control Panel Module"),
		KAboutData::License_GPL,
		I18N_NOOP("(c) 2012 Timothy Pearson"), 0, 0);
	
	about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
	setAboutData( about );

	processLockouts();
};

LDAPConfig::~LDAPConfig() {
}

void LDAPConfig::load() {
	kgs = new KGlobalSettings();
}

void LDAPConfig::defaults() {
	
}

void LDAPConfig::save() {
	
}

void LDAPConfig::processLockouts() {
	//
}

int LDAPConfig::buttons() {
	return KCModule::Apply|KCModule::Help;
}

TQString LDAPConfig::quickHelp() const
{
	return i18n("This module manages users, groups, and machines in LDAP realms.");
}
