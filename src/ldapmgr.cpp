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
#include "groupconfigdlg.h"

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

	connect(base->user_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->group_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->machine_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->user_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(userHighlighted()));
	connect(base->group_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(groupHighlighted()));

	connect(base->user_buttonModify, TQT_SIGNAL(clicked()), this, TQT_SLOT(modifySelectedUser()));
	connect(base->group_buttonModify, TQT_SIGNAL(clicked()), this, TQT_SLOT(modifySelectedGroup()));
	
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
	// Update all drop down lists
	base->user_ldapRealm->setCurrentItem(realm, false, -1);
	base->group_ldapRealm->setCurrentItem(realm, false, -1);
	base->machine_ldapRealm->setCurrentItem(realm, false, -1);

	if (m_ldapmanager) {
		if (m_ldapmanager->realm() == realm) {
			return;
		}
		delete m_ldapmanager;
	}

	m_systemconfig->setGroup("LDAPRealm-" + realm);
	TQString host = m_systemconfig->readEntry("admin_server");
	m_ldapmanager = new LDAPManager(realm, host);

	updateAllInformation();
}

void LDAPConfig::updateAllInformation() {
	populateUsers();
	populateGroups();
	// RAJA FIXME
	// Machines??

	updateUsersList();
	updateGroupsList();
	// RAJA FIXME
	// Machines??
}

void LDAPConfig::populateUsers() {
	m_userInfoList = m_ldapmanager->users();
}

void LDAPConfig::populateGroups() {
	m_groupInfoList = m_ldapmanager->groups();
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

void LDAPConfig::updateGroupsList() {
	base->group_list->clear();
	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		(void)new TQListViewItem(base->group_list, group.name, TQString("%1").arg(group.gid));
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

LDAPGroupInfo LDAPConfig::findGroupInfoByNameAndGID(TQString name, TQString gid) {
	// Figure out which group is selected
	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		if ((group.name == name) && (TQString("%1").arg(group.gid) == gid)) {
			return group;
		}
	}
	return LDAPGroupInfo();
}

LDAPGroupInfo LDAPConfig::findGroupInfoByGID(TQString gid) {
	// Figure out which group is selected
	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		if (TQString("%1").arg(group.gid) == gid) {
			return group;
		}
	}
	return LDAPGroupInfo();
}

LDAPUserInfo LDAPConfig::selectedUser() {
	TQListViewItem* lvi = base->user_list->currentItem();
	if (!lvi) {
		return LDAPUserInfo();
	}
	return findUserInfoByNameAndUID(lvi->text(0), lvi->text(2));
}

LDAPGroupInfo LDAPConfig::selectedGroup() {
	TQListViewItem* lvi = base->group_list->currentItem();
	if (!lvi) {
		return LDAPGroupInfo();
	}
	return findGroupInfoByNameAndGID(lvi->text(0), lvi->text(1));
}

LDAPUserInfo LDAPConfig::findUserByDistinguishedName(TQString dn) {
	LDAPUserInfoList::Iterator it;
	for (it = m_userInfoList.begin(); it != m_userInfoList.end(); ++it) {
		LDAPUserInfo user = *it;
		if (user.distinguishedName == dn) {
			return user;
		}
	}
	return LDAPUserInfo();
}

LDAPGroupInfoList LDAPConfig::findGroupsForUserByDistinguishedName(TQString dn) {
	LDAPGroupInfoList groups;

	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		if (group.userlist.contains(dn)) {
			groups.append(group);
		}
	}

	return groups;
}

LDAPUserInfoList LDAPConfig::userList() {
	return m_userInfoList;
}

LDAPGroupInfoList LDAPConfig::groupList() {
	return m_groupInfoList;
}

void LDAPConfig::userHighlighted() {
	// Show information in the quick view area
	LDAPUserInfo user = selectedUser();

	base->user_loginName->setText(user.name);
	base->user_uid->setText(TQString("%1").arg(user.uid));
	base->user_primaryGroup->setText(findGroupInfoByGID(TQString("%1").arg(user.primary_gid)).name);
	base->user_realName->setText(user.commonName);
	base->user_status->setText((user.status == KRB5_DISABLED_ACCOUNT)?"Disabled":"Enabled");
	LDAPGroupInfoList groupsForUser = findGroupsForUserByDistinguishedName(user.distinguishedName);
	TQString groupsForUserText;
	LDAPGroupInfoList::Iterator it;
	for (it = groupsForUser.begin(); it != groupsForUser.end(); ++it) {
		if (it != groupsForUser.begin()) {
			groupsForUserText.append(",");
		}
		groupsForUserText.append((*it).name);
	}
	base->user_secondaryGroups->setText(groupsForUserText);

	processLockouts();
}

void LDAPConfig::groupHighlighted() {
	// Show information in the quick view area
	LDAPGroupInfo group = selectedGroup();

	base->group_memberList->clear();
	for ( TQStringList::Iterator it = group.userlist.begin(); it != group.userlist.end(); ++it ) {
		LDAPUserInfo user = findUserByDistinguishedName(*it);
		(void)new TQListViewItem(base->group_memberList, user.name, user.commonName, TQString("%1").arg(user.uid));
	}

	// RAJA FIXME

	processLockouts();
}

void LDAPConfig::modifySelectedUser() {
	// Launch a dialog to edit the user
	LDAPUserInfo user = selectedUser();

	// Reload user data from LDAP before launching dialog
	user = m_ldapmanager->getUserByDistinguishedName(user.distinguishedName);
	UserConfigDialog userconfigdlg(user, this);
	if (userconfigdlg.exec() == TQDialog::Accepted) {
		// RAJA FIXME
	}
	updateAllInformation();
}

void LDAPConfig::modifySelectedGroup() {
	// Launch a dialog to edit the user
	LDAPGroupInfo group = selectedGroup();

	// Reload group data from LDAP before launching dialog
	group = m_ldapmanager->getGroupByDistinguishedName(group.distinguishedName);
	GroupConfigDialog groupconfigdlg(group, this);
	if (groupconfigdlg.exec() == TQDialog::Accepted) {
		// RAJA FIXME
	}
	updateAllInformation();
}

int LDAPConfig::buttons() {
	return KCModule::Apply|KCModule::Help;
}

TQString LDAPConfig::quickHelp() const
{
	return i18n("This module manages users, groups, and machines in LDAP realms.");
}
