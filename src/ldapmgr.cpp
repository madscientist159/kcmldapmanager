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
#include <tqapplication.h>

#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeparts/genericfactory.h>
#include <ksimpleconfig.h>
#include <tdeglobalsettings.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <tdelistview.h>
#include <kopenwith.h>
#include <kpropertiesdialog.h>
#include <tdeio/job.h>
#include <tqdir.h>
#include <tqheader.h>
#include <ksimpleconfig.h>
#include <kcombobox.h>
#include <tdemessagebox.h>
#include <klineedit.h>
#include <kiconloader.h>

#include <tdesu/process.h>

#include "ldapmgr.h"

#include "libtdeldap.h"
#include "ldappasswddlg.h"
#include "userconfigdlg.h"
#include "groupconfigdlg.h"
#include "serviceconfigdlg.h"

// FIXME
// Connect this to CMake/Automake
#define KDE_CONFDIR "/etc/trinity"

typedef KGenericFactory<LDAPConfig, TQWidget> LDAPConfigFactory;

K_EXPORT_COMPONENT_FACTORY( kcm_ldapmanager, LDAPConfigFactory("kcmldapmanager"))

LDAPConfig::LDAPConfig(TQWidget *parent, const char *name, const TQStringList&)
    : TDECModule(parent, name), myAboutData(0), m_ldapmanager(0)
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
	base->service_list->setAllColumnsShowFocus(true);
	base->service_list->setFullWidth(true);

	base->user_loginName->setEnabled(false);
	base->user_uid->setEnabled(false);
	base->user_primaryGroup->setEnabled(false);
	base->user_realName->setEnabled(false);
	base->user_status->setEnabled(false);
	base->user_secondaryGroups->setEnabled(false);

	base->machine_name->setEnabled(false);
	base->machine_author->setEnabled(false);
	base->service_name->setEnabled(false);
	base->service_author->setEnabled(false);

	base->user_icon->setPixmap(SmallIcon("personal.png"));
	base->group_icon->setPixmap(SmallIcon("kdmconfig.png"));
	base->machine_icon->setPixmap(SmallIcon("system.png"));
	base->service_icon->setPixmap(SmallIcon("kcmsystem.png"));

	connect(base->user_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->group_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->machine_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->service_ldapRealm, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(connectToRealm(const TQString&)));
	connect(base->user_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(userHighlighted()));
	connect(base->group_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(groupHighlighted()));
	connect(base->machine_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(machineHighlighted()));
	connect(base->service_list, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(serviceHighlighted()));
	connect(base->user_list, TQT_SIGNAL(executed(TQListViewItem*)), this, TQT_SLOT(modifySelectedUser()));
	connect(base->group_list, TQT_SIGNAL(executed(TQListViewItem*)), this, TQT_SLOT(modifySelectedGroup()));

	connect(base->user_buttonAdd, TQT_SIGNAL(clicked()), this, TQT_SLOT(addNewUser()));
	connect(base->group_buttonAdd, TQT_SIGNAL(clicked()), this, TQT_SLOT(addNewGroup()));
	connect(base->service_buttonAdd, TQT_SIGNAL(clicked()), this, TQT_SLOT(addNewService()));
	connect(base->user_buttonModify, TQT_SIGNAL(clicked()), this, TQT_SLOT(modifySelectedUser()));
	connect(base->group_buttonModify, TQT_SIGNAL(clicked()), this, TQT_SLOT(modifySelectedGroup()));
	connect(base->user_buttonDelete, TQT_SIGNAL(clicked()), this, TQT_SLOT(removeSelectedUser()));
	connect(base->group_buttonDelete, TQT_SIGNAL(clicked()), this, TQT_SLOT(removeSelectedGroup()));
	connect(base->machine_buttonDelete, TQT_SIGNAL(clicked()), this, TQT_SLOT(removeSelectedMachine()));
	connect(base->service_buttonDelete, TQT_SIGNAL(clicked()), this, TQT_SLOT(removeSelectedService()));

	connect(base->user_buttonRefresh, TQT_SIGNAL(clicked()), this, TQT_SLOT(updateAllInformation()));
	connect(base->group_buttonRefresh, TQT_SIGNAL(clicked()), this, TQT_SLOT(updateAllInformation()));
	connect(base->machine_buttonRefresh, TQT_SIGNAL(clicked()), this, TQT_SLOT(updateAllInformation()));
	connect(base->service_buttonRefresh, TQT_SIGNAL(clicked()), this, TQT_SLOT(updateAllInformation()));
	
	load();
	
	TDEAboutData* about = new TDEAboutData("ldapmanager", I18N_NOOP("TDE LDAP Realm Manager"), "0.1",
		I18N_NOOP("TDE LDAP Realm Manager Control Panel Module"),
		TDEAboutData::License_GPL,
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
	base->group_ldapRealm->clear();
	base->machine_ldapRealm->clear();
	base->service_ldapRealm->clear();
	base->user_ldapRealm->insertItem("<none>");
	base->group_ldapRealm->insertItem("<none>");
	base->machine_ldapRealm->insertItem("<none>");
	base->service_ldapRealm->insertItem("<none>");
	TQStringList cfgRealms = m_systemconfig->groupList();
	for (TQStringList::Iterator it(cfgRealms.begin()); it != cfgRealms.end(); ++it) {
		if ((*it).startsWith("LDAPRealm-")) {
			m_systemconfig->setGroup(*it);
			TQString realmName=*it;
			realmName.remove(0,strlen("LDAPRealm-"));
			base->user_ldapRealm->insertItem(realmName);
			base->group_ldapRealm->insertItem(realmName);
			base->machine_ldapRealm->insertItem(realmName);
			base->service_ldapRealm->insertItem(realmName);
		}
	}
	TQString defaultRealm = m_systemconfig->readEntry("DefaultRealm", TQString::null);
	if (defaultRealm != "") {
		for (i=0; i<base->user_ldapRealm->count(); i++) {
			if (base->user_ldapRealm->text(i).lower() == defaultRealm.lower()) {
				base->user_ldapRealm->setCurrentItem(i);
				base->group_ldapRealm->setCurrentItem(i);
				base->machine_ldapRealm->setCurrentItem(i);
				base->service_ldapRealm->setCurrentItem(i);
				break;
			}
		}
	}
	else {
		// Try hard not to select the "<none>" realm
		for (i=0; i<base->user_ldapRealm->count(); i++) {
			if (base->user_ldapRealm->text(i).lower() != "<none>") {
				base->user_ldapRealm->setCurrentItem(i);
				base->group_ldapRealm->setCurrentItem(i);
				base->machine_ldapRealm->setCurrentItem(i);
				base->service_ldapRealm->setCurrentItem(i);
				break;
			}
		}
	}
	if (base->user_ldapRealm->currentText().lower() != "<none>") {
		connectToRealm(base->user_ldapRealm->currentText().upper());
	}
}

void LDAPConfig::defaults() {
	//
}

void LDAPConfig::save() {
	//
}

void LDAPConfig::processLockouts() {
	bool connected = (!(!m_ldapmanager));

	TQListViewItem* lvi = base->user_list->selectedItem();
	if (lvi) {
		LDAPUserInfo user = selectedUser();
		base->user_buttonModify->setEnabled(connected);
		base->user_buttonDelete->setEnabled(!user.tde_builtin_account);
	}
	else {
		base->user_buttonModify->setEnabled(false);
		base->user_buttonDelete->setEnabled(false);
	}
	base->user_buttonAdd->setEnabled(connected);
	base->user_buttonRefresh->setEnabled(connected);

	lvi = base->group_list->selectedItem();
	if (lvi) {
		LDAPGroupInfo group = selectedGroup();
		base->group_buttonModify->setEnabled(connected);
		base->group_buttonDelete->setEnabled(!group.tde_builtin_account);
	}
	else {
		base->group_buttonModify->setEnabled(false);
		base->group_buttonDelete->setEnabled(false);
	}
	base->group_buttonAdd->setEnabled(connected);
	base->group_buttonRefresh->setEnabled(connected);

	lvi = base->machine_list->selectedItem();
	if (lvi) {
		LDAPMachineInfo machine = selectedMachine();
		base->machine_buttonDelete->setEnabled(!machine.tde_builtin_account);
	}
	else {
		base->machine_buttonDelete->setEnabled(false);
	}
	// FIXME
	// Disable machine add/modify as they are not implemented
	// In fact, I don't know if I CAN implement them!
	// Think about it...yes you can add the 'add' feature...kadmin 'ank --random-key host/HOSTNAME.FQDN'...
	base->machine_buttonAdd->setEnabled(false);
	base->machine_buttonModify->setEnabled(false);
	base->machine_buttonRefresh->setEnabled(connected);

	lvi = base->service_list->selectedItem();
	if (lvi) {
		LDAPServiceInfo service = selectedService();
		base->service_buttonDelete->setEnabled(!service.tde_builtin_account);
	}
	else {
		base->service_buttonDelete->setEnabled(false);
	}
	base->service_buttonAdd->setEnabled(connected);
	// FIXME
	// Disable service modify as it is not implemented
	base->service_buttonModify->setEnabled(false);
	base->service_buttonRefresh->setEnabled(connected);
}

void LDAPConfig::connectToRealm(const TQString& realm) {
	// Update all drop down lists
	base->user_ldapRealm->setCurrentItem(realm, false, -1);
	base->group_ldapRealm->setCurrentItem(realm, false, -1);
	base->machine_ldapRealm->setCurrentItem(realm, false, -1);
	base->service_ldapRealm->setCurrentItem(realm, false, -1);

	if (realm == "<none>") {
		abortConnection();
	}
	else {
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

	processLockouts();
}

void LDAPConfig::abortConnection() {
	if (m_ldapmanager) delete m_ldapmanager;
	m_ldapmanager = 0;
	base->user_list->clear();
	base->group_list->clear();
	base->machine_list->clear();
	base->service_list->clear();
	base->user_ldapRealm->setCurrentItem("<none>", false, -1);
	base->group_ldapRealm->setCurrentItem("<none>", false, -1);
	base->machine_ldapRealm->setCurrentItem("<none>", false, -1);
	base->service_ldapRealm->setCurrentItem("<none>", false, -1);
}

void LDAPConfig::updateAllInformation() {
	if (populateUsers() != 0) {
		abortConnection();
		return;
	}
	else {
		if (populateGroups() != 0) {
			abortConnection();
			return;
		}
		else {
			if (populateMachines() != 0) {
				abortConnection();
				return;
			}
			else {
				if (populateServices() != 0) {
					abortConnection();
					return;
				}
			}
		}
	}

	updateUsersList();
	updateGroupsList();
	updateMachinesList();
	updateServicesList();
}

int LDAPConfig::populateUsers() {
	if (!m_ldapmanager) {
		return -1;
	}

	int retcode;
	m_userInfoList = m_ldapmanager->users(&retcode);
	return retcode;
}

int LDAPConfig::populateGroups() {
	if (!m_ldapmanager) {
		return -1;
	}

	int retcode;
	m_groupInfoList = m_ldapmanager->groups(&retcode);
	return retcode;
}

int LDAPConfig::populateMachines() {
	if (!m_ldapmanager) {
		return -1;
	}

	int retcode;
	m_machineInfoList = m_ldapmanager->machines(&retcode);
	return retcode;
}

int LDAPConfig::populateServices() {
	if (!m_ldapmanager) {
		return -1;
	}

	int retcode;
	m_serviceInfoList = m_ldapmanager->services(&retcode);
	return retcode;
}

void LDAPConfig::updateUsersList() {
	TQListViewItem* itm = base->user_list->selectedItem();
	TQString prevSelectedItemText;
	if (itm) {
		prevSelectedItemText = itm->text(0);
	}

	base->user_list->clear();
	LDAPUserInfoList::Iterator it;
	for (it = m_userInfoList.begin(); it != m_userInfoList.end(); ++it) {
		LDAPUserInfo user = *it;
		itm = new TQListViewItem(base->user_list, user.name, user.commonName, TQString("%1").arg(user.uid));
		if (prevSelectedItemText != "") {
			if (user.name == prevSelectedItemText) {
				base->user_list->setSelected(itm, true);
			}
		}
	}
	
	processLockouts();
}

void LDAPConfig::updateGroupsList() {
	TQListViewItem* itm = base->group_list->selectedItem();
	TQString prevSelectedItemText;
	if (itm) {
		prevSelectedItemText = itm->text(0);
	}

	base->group_list->clear();
	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		itm = new TQListViewItem(base->group_list, group.name, TQString("%1").arg(group.gid));
		if (prevSelectedItemText != "") {
			if (group.name == prevSelectedItemText) {
				base->group_list->setSelected(itm, true);
			}
		}
	}
	processLockouts();
}

void LDAPConfig::updateMachinesList() {
	TQListViewItem* itm = base->machine_list->selectedItem();
	TQString prevSelectedItemText;
	if (itm) {
		prevSelectedItemText = itm->text(0);
	}

	base->machine_list->clear();
	LDAPMachineInfoList::Iterator it;
	for (it = m_machineInfoList.begin(); it != m_machineInfoList.end(); ++it) {
		LDAPMachineInfo machine = *it;
		itm = new TQListViewItem(base->machine_list, machine.name);
		if (prevSelectedItemText != "") {
			if (machine.name == prevSelectedItemText) {
				base->machine_list->setSelected(itm, true);
			}
		}
	}
	processLockouts();
}

void LDAPConfig::updateServicesList() {
	TQListViewItem* itm = base->service_list->selectedItem();
	TQString prevSelectedItemText;
	if (itm) {
		prevSelectedItemText = itm->text(0);
	}

	base->service_list->clear();
	LDAPServiceInfoList::Iterator it;
	for (it = m_serviceInfoList.begin(); it != m_serviceInfoList.end(); ++it) {
		LDAPServiceInfo service = *it;
		itm = new TQListViewItem(base->service_list, service.name, service.machine);
		if (prevSelectedItemText != "") {
			if (service.name == prevSelectedItemText) {
				base->service_list->setSelected(itm, true);
			}
		}
	}
	processLockouts();
}

LDAPUserInfo LDAPConfig::findUserInfoByName(TQString name) {
	// Figure out which user is selected
	LDAPUserInfoList::Iterator it;
	for (it = m_userInfoList.begin(); it != m_userInfoList.end(); ++it) {
		LDAPUserInfo user = *it;
		if (user.name == name) {
			return user;
		}
	}
	return LDAPUserInfo();
}

LDAPGroupInfo LDAPConfig::findGroupInfoByName(TQString name) {
	// Figure out which group is selected
	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		if (group.name == name) {
			return group;
		}
	}
	return LDAPGroupInfo();
}

LDAPMachineInfo LDAPConfig::findMachineInfoByName(TQString name) {
	// Figure out which machine is selected
	LDAPMachineInfoList::Iterator it;
	for (it = m_machineInfoList.begin(); it != m_machineInfoList.end(); ++it) {
		LDAPMachineInfo machine = *it;
		if (machine.name == name) {
			return machine;
		}
	}
	return LDAPMachineInfo();
}

LDAPServiceInfo LDAPConfig::findServiceInfoByName(TQString name, TQString machine) {
	// Figure out which service is selected
	LDAPServiceInfoList::Iterator it;
	for (it = m_serviceInfoList.begin(); it != m_serviceInfoList.end(); ++it) {
		LDAPServiceInfo service = *it;
		if ((service.name == name) && (service.machine == machine)) {
			return service;
		}
	}
	return LDAPServiceInfo();
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
	TQListViewItem* lvi = base->user_list->selectedItem();
	if (!lvi) {
		return LDAPUserInfo();
	}
	return findUserInfoByNameAndUID(lvi->text(0), lvi->text(2));
}

LDAPGroupInfo LDAPConfig::selectedGroup() {
	TQListViewItem* lvi = base->group_list->selectedItem();
	if (!lvi) {
		return LDAPGroupInfo();
	}
	return findGroupInfoByNameAndGID(lvi->text(0), lvi->text(1));
}

LDAPMachineInfo LDAPConfig::selectedMachine() {
	TQListViewItem* lvi = base->machine_list->selectedItem();
	if (!lvi) {
		return LDAPMachineInfo();
	}
	return findMachineInfoByName(lvi->text(0));
}

LDAPServiceInfo LDAPConfig::selectedService() {
	TQListViewItem* lvi = base->service_list->selectedItem();
	if (!lvi) {
		return LDAPServiceInfo();
	}
	return findServiceInfoByName(lvi->text(0), lvi->text(1));
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

LDAPMachineInfoList LDAPConfig::machineList() {
	return m_machineInfoList;
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
		if (user.name != "") {
			(void)new TQListViewItem(base->group_memberList, user.name, user.commonName, TQString("%1").arg(user.uid));
		}
	}

	processLockouts();
}

void LDAPConfig::machineHighlighted() {
	// Show information in the quick view area
	LDAPMachineInfo machine = selectedMachine();

	base->machine_name->setText(machine.name);
	base->machine_author->setText(findUserByDistinguishedName(machine.creatorsName).name);

	processLockouts();
}

void LDAPConfig::serviceHighlighted() {
	// Show information in the quick view area
	LDAPServiceInfo service = selectedService();

	base->service_name->setText(service.name);
	base->service_author->setText(findUserByDistinguishedName(service.creatorsName).name);

	processLockouts();
}

void LDAPConfig::addNewUser() {
	// Launch a dialog to add the user
	LDAPUserInfo user;

	// Find the next available, reasonable UID
	uid_t uid = 100;
	LDAPUserInfoList::Iterator it;
	for (it = m_userInfoList.begin(); it != m_userInfoList.end(); ++it) {
		LDAPUserInfo user = *it;
		if (user.uid >= uid) {
			uid = user.uid + 1;
		}
	}
	user.uid = uid;

	UserConfigDialog userconfigdlg(user, this);
	if (userconfigdlg.exec() == TQDialog::Accepted) {
		user = userconfigdlg.m_user;
		if (user.name != "") {
			// Try to find a reasonable place to stuff the new entry
			// Do any users exist right now?
			if (m_userInfoList.begin() != m_userInfoList.end()) {
				user.distinguishedName = (*m_userInfoList.begin()).distinguishedName;
				int eqpos = user.distinguishedName.find("=")+1;
				int cmpos = user.distinguishedName.find(",", eqpos);
				user.distinguishedName.remove(eqpos, cmpos-eqpos);
				user.distinguishedName.insert(eqpos, user.name);
			}
			else {
				user.distinguishedName = "uid=" + user.name + "," + m_ldapmanager->basedn();
			}
			if (m_ldapmanager->addUserInfo(user) == 0) {
				// Modify group(s) as needed
				populateGroups();
				LDAPGroupInfoList::Iterator it;
				for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
					LDAPGroupInfo group = *it;
					if (userconfigdlg.selectedGroups.contains(group.name)) {
						// Make sure that we are in this group!
						if (!group.userlist.contains(user.distinguishedName)) {
							group.userlist.append(user.distinguishedName);
							m_ldapmanager->updateGroupInfo(group);
						}
					}
					else {
						// Make sure that we are NOT in this group!
						if (group.userlist.contains(user.distinguishedName)) {
							group.userlist.remove(user.distinguishedName);
							m_ldapmanager->updateGroupInfo(group);
						}
					}
				}

				if (user.new_password != "") {
					// If a new password was set, use Kerberos to set it on the server
					TQString errorString;
					if (setPasswordForUser(user, &errorString) != 0) {
						KMessageBox::error(0, i18n("<qt>Unable to set password for user!<p>%1</qt>").arg(errorString), i18n("Kerberos Failure"));
					}
					m_ldapmanager->unbind(true);	// Using kadmin on admin users/groups can disrupt our LDAP connection (likely due to the ACL rewrite)
				}
			}
		}
		else {
			// PEBKAC
			KMessageBox::error(0, i18n("<qt>Unable to add new user with no name!<p>Enter a name and try again</qt>"), i18n("Illegal Operation"));
		}
	}
	updateAllInformation();
}

void LDAPConfig::addNewGroup() {
	// Launch a dialog to add the group
	LDAPGroupInfo group;

	// Find the next available, reasonable GID
	gid_t gid = 100;
	LDAPGroupInfoList::Iterator it;
	for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
		LDAPGroupInfo group = *it;
		if (group.gid >= gid) {
			gid = group.gid + 1;
		}
	}
	group.gid = gid;

	GroupConfigDialog groupconfigdlg(group, this);
	if (groupconfigdlg.exec() == TQDialog::Accepted) {
		group = groupconfigdlg.m_group;
		if (group.name != "") {
			// Try to find a reasonable place to stuff the new entry
			// Do any groups exist right now?
			if (m_groupInfoList.begin() != m_groupInfoList.end()) {
				group.distinguishedName = (*m_groupInfoList.begin()).distinguishedName;
				int eqpos = group.distinguishedName.find("=")+1;
				int cmpos = group.distinguishedName.find(",", eqpos);
				group.distinguishedName.remove(eqpos, cmpos-eqpos);
				group.distinguishedName.insert(eqpos, group.name);
			}
			else {
				group.distinguishedName = "cn=" + group.name + "," + m_ldapmanager->basedn();
			}
			m_ldapmanager->addGroupInfo(group);
		}
		else {
			// PEBKAC
			KMessageBox::error(0, i18n("<qt>Unable to add new group with no name!<p>Enter a name and try again</qt>"), i18n("Illegal Operation"));
		}
	}
	updateAllInformation();
}

void LDAPConfig::addNewService() {
	// Launch a dialog to add the service
	LDAPServiceInfo service;

	ServiceConfigDialog serviceconfigdlg(service, this);
	if (serviceconfigdlg.exec() == TQDialog::Accepted) {
		service = serviceconfigdlg.m_service;
		TQString errorstring;
		if (m_ldapmanager->addServiceInfo(service, &errorstring) != 0) {
			KMessageBox::error(0, i18n("<qt>Unable to add new service!<p>%1</qt>").arg(errorstring), i18n("Internal Failure"));
		}
	}
	updateAllInformation();
}

void LDAPConfig::modifySelectedUser() {
	// Launch a dialog to edit the user
	LDAPUserInfo user = selectedUser();

	// Reload user data from LDAP before launching dialog
	user = m_ldapmanager->getUserByDistinguishedName(user.distinguishedName);
	UserConfigDialog userconfigdlg(user, this);
	if (userconfigdlg.exec() == TQDialog::Accepted) {
		user = userconfigdlg.m_user;
		if (m_ldapmanager->updateUserInfo(user) == 0) {
			// Modify group(s) as needed
			populateGroups();
			LDAPGroupInfoList::Iterator it;
			for (it = m_groupInfoList.begin(); it != m_groupInfoList.end(); ++it) {
				LDAPGroupInfo group = *it;
				if (userconfigdlg.selectedGroups.contains(group.name)) {
					// Make sure that we are in this group!
					if (!group.userlist.contains(user.distinguishedName)) {
						group.userlist.append(user.distinguishedName);
						m_ldapmanager->updateGroupInfo(group);
					}
				}
				else {
					// Make sure that we are NOT in this group!
					if (group.userlist.contains(user.distinguishedName)) {
						group.userlist.remove(user.distinguishedName);
						m_ldapmanager->updateGroupInfo(group);
					}
				}
			}

			if (user.new_password != "") {
				// If a new password was set, use Kerberos to set it on the server
				TQString errorString;
				if (setPasswordForUser(user, &errorString) != 0) {
					KMessageBox::error(0, i18n("<qt>Unable to set password for user!<p>%1</qt>").arg(errorString), i18n("Kerberos Failure"));
				}
				m_ldapmanager->unbind(true);	// Using kadmin on admin users/groups can disrupt our LDAP connection (likely due to the ACL rewrite)
			}
		}
	}
	updateAllInformation();
}

void LDAPConfig::modifySelectedGroup() {
	// Launch a dialog to edit the group
	LDAPGroupInfo group = selectedGroup();

	// Reload group data from LDAP before launching dialog
	group = m_ldapmanager->getGroupByDistinguishedName(group.distinguishedName);
	GroupConfigDialog groupconfigdlg(group, this);
	if (groupconfigdlg.exec() == TQDialog::Accepted) {
		group = groupconfigdlg.m_group;
		m_ldapmanager->updateGroupInfo(group);
	}
	updateAllInformation();
}

void LDAPConfig::removeSelectedUser() {
	LDAPUserInfo user = selectedUser();

	if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to delete the user %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(user.name), i18n("Confirmation Required")) == KMessageBox::Yes) {
		m_ldapmanager->deleteUserInfo(user);
	}

	updateAllInformation();
}

void LDAPConfig::removeSelectedGroup() {
	LDAPGroupInfo group = selectedGroup();

	if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to delete the group %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(group.name), i18n("Confirmation Required")) == KMessageBox::Yes) {
		m_ldapmanager->deleteGroupInfo(group);
	}

	updateAllInformation();
}

void LDAPConfig::removeSelectedMachine() {
	LDAPMachineInfo machine = selectedMachine();

	if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to delete the machine %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(machine.name), i18n("Confirmation Required")) == KMessageBox::Yes) {
		m_ldapmanager->deleteMachineInfo(machine);
	}

	updateAllInformation();
}

void LDAPConfig::removeSelectedService() {
	LDAPServiceInfo service = selectedService();

	if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to delete the service %1 for host %2</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(service.name).arg(service.machine), i18n("Confirmation Required")) == KMessageBox::Yes) {
		m_ldapmanager->deleteServiceInfo(service);
	}

	updateAllInformation();
}

int LDAPConfig::setPasswordForUser(LDAPUserInfo user, TQString *errstr) {
	if (user.new_password == "") {
		return 0;
	}

	return m_ldapmanager->setPasswordForUser(user, errstr);
}

int LDAPConfig::buttons() {
	return TDECModule::Apply|TDECModule::Help;
}

TQString LDAPConfig::quickHelp() const
{
	return i18n("This module manages users, groups, and machines in LDAP realms.");
}
