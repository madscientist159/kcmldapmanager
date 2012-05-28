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
#include <ksimpleconfig.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <klineedit.h>

#include "ldapmgr.h"

#include "libtdeldap.h"
#include "ldappasswddlg.h"
#include "userconfigdlg.h"

// FIXME
// Connect this to CMake/Automake
#define KDE_CONFDIR "/etc/trinity"

typedef KGenericFactory<LDAPConfig, TQWidget> LDAPConfigFactory;

K_EXPORT_COMPONENT_FACTORY( kcm_ldapmanager, LDAPConfigFactory("kcmldapmanager"))

LDAPConfig::LDAPConfig(TQWidget *parent, const char *name, const TQStringList&)
    : KCModule(parent, name), myAboutData(0), m_ldapmanager(0)
{
	m_systemconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/ldap/ldapconfigrc" ));

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

	base->user_loginName->setEnabled(false);
	base->user_uid->setEnabled(false);
	base->user_primaryGroup->setEnabled(false);
	base->user_realName->setEnabled(false);
	base->user_status->setEnabled(false);
	base->user_secondaryGroups->setEnabled(false);

	connect(base->user_ldapRealm, TQT_SIGNAL(highlighted(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->user_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(userHighlighted()));

	connect(base->user_buttonModify, TQT_SIGNAL(clicked()), this, TQT_SLOT(modifySelectedUser()));
	
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
	delete m_systemconfig;
}

void LDAPConfig::load() {
	// Load realms
	int i;
	base->user_ldapRealm->clear();
	TQStringList cfgRealms = m_systemconfig->groupList();
	for (TQStringList::Iterator it(cfgRealms.begin()); it != cfgRealms.end(); ++it) {
		if ((*it).startsWith("LDAPRealm-")) {
			m_systemconfig->setGroup(*it);
			TQString realmName=*it;
			realmName.remove(0,strlen("LDAPRealm-"));
			base->user_ldapRealm->insertItem(realmName);
		}
	}
	TQString defaultRealm = m_systemconfig->readEntry("DefaultRealm", TQString::null);
	if (defaultRealm != "") {
		for (i=0; i<base->user_ldapRealm->count(); i++) {
			if (base->user_ldapRealm->text(i).lower() == defaultRealm.lower()) {
				base->user_ldapRealm->setCurrentItem(i);
				break;
			}
		}
	}
	connectToRealm(base->user_ldapRealm->currentText().upper());
}

void LDAPConfig::defaults() {
	
}

void LDAPConfig::save() {
	
}

void LDAPConfig::processLockouts() {
	//
}

void LDAPConfig::connectToRealm(const TQString& realm) {
	if (m_ldapmanager) {
		if (m_ldapmanager->realm() == realm) {
			return;
		}
		delete m_ldapmanager;
	}

	m_systemconfig->setGroup("LDAPRealm-" + realm);
	TQString host = m_systemconfig->readEntry("admin_server");
	m_ldapmanager = new LDAPManager(realm, host);

	populateUsers();
	// RAJA FIXME
	// Groups?? Machines??
}

void LDAPConfig::populateUsers() {
	m_userInfoList = m_ldapmanager->users();
	updateUsersList();
}

void LDAPConfig::updateUsersList() {
	base->user_list->clear();
	LDAPUserInfoList::Iterator it;
	for (it = m_userInfoList.begin(); it != m_userInfoList.end(); ++it) {
		LDAPUserInfo user = *it;
		(void)new TQListViewItem(base->user_list, user.name, user.commonName, TQString("%1").arg(user.uid));
	}
	processLockouts();
}

LDAPUserInfo LDAPConfig::findUserInfoByNameAndUID(TQString name, TQString uid) {
	// Figure out which user is selected
	LDAPUserInfoList::Iterator it;
	for (it = m_userInfoList.begin(); it != m_userInfoList.end(); ++it) {
		LDAPUserInfo user = *it;
		if ((user.name == name) && (TQString("%1").arg(user.uid) == uid)) {
			return user;
		}
	}
	return LDAPUserInfo();
}

LDAPUserInfo LDAPConfig::selectedUser() {
	TQListViewItem* lvi = base->user_list->currentItem();
	if (!lvi) {
		return LDAPUserInfo();
	}
	return findUserInfoByNameAndUID(lvi->text(0), lvi->text(2));
}

void LDAPConfig::userHighlighted() {
	// Show information in the quick view area
	LDAPUserInfo user = selectedUser();

	base->user_loginName->setText(user.name);
	base->user_uid->setText(TQString("%1").arg(user.uid));
	base->user_primaryGroup->setText(TQString("%1").arg(user.primary_gid));
	base->user_realName->setText(user.commonName);
	base->user_status->setText((user.status == KRB5_DISABLED_ACCOUNT)?"Disabled":"Enabled");
	base->user_secondaryGroups->setText("RAJA FIXME");

	processLockouts();
}

void LDAPConfig::modifySelectedUser() {
	// Launch a dialog to edit the user
	LDAPUserInfo user = selectedUser();

	// RAJA FIXME
	UserConfigDialog userconfigdlg(user, this);
	if (userconfigdlg.exec() == TQDialog::Accepted) {
	}
}

int LDAPConfig::buttons() {
	return KCModule::Apply|KCModule::Help;
}

TQString LDAPConfig::quickHelp() const
{
	return i18n("This module manages users, groups, and machines in LDAP realms.");
}
