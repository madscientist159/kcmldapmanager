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

#ifndef _LDAPMGR_H_
#define _LDAPMGR_H_

#include <tdecmodule.h>
#include <tdeaboutdata.h>
#include <kpushbutton.h>
#include <tdelistview.h>
#include <tdefileitem.h>
#include <tdeglobalsettings.h>
#include <tqpushbutton.h>
#include <tqcombobox.h>

#include "libtdeldap.h"
#include "ldapconfigbase.h"

class KSimpleConfig;

class LDAPConfig: public TDECModule
{
	Q_OBJECT

	public:
		LDAPConfig( TQWidget *parent=0, const char *name=0, const TQStringList& = TQStringList() );
		~LDAPConfig();
		
		virtual void load();
		virtual void save();
		virtual void defaults();
		virtual int buttons();
		virtual TQString quickHelp() const;
		virtual const TDEAboutData *aboutData() const { return myAboutData; };

	private slots:
		void processLockouts();
		void connectToRealm(const TQString&);
		int populateUsers();
		int populateGroups();
		int populateMachines();
		int populateServices();
		void updateUsersList();
		void updateGroupsList();
		void updateMachinesList();
		void updateServicesList();
		void userHighlighted();
		void groupHighlighted();
		void machineHighlighted();
		void serviceHighlighted();
		void addNewUser();
		void addNewGroup();
		void addNewService();
		void modifySelectedUser();
		void modifySelectedGroup();
		void removeSelectedUser();
		void removeSelectedGroup();
		void removeSelectedMachine();
		void removeSelectedService();
		void updateAllInformation();
		void abortConnection();

	public:
		LDAPUserInfo findUserInfoByName(TQString name);
		LDAPGroupInfo findGroupInfoByName(TQString name);
		LDAPMachineInfo findMachineInfoByName(TQString name);
		LDAPServiceInfo findServiceInfoByName(TQString name, TQString machine);
		LDAPUserInfo findUserInfoByNameAndUID(TQString name, TQString uid);
		LDAPGroupInfo findGroupInfoByNameAndGID(TQString name, TQString gid);
		LDAPGroupInfo findGroupInfoByGID(TQString gid);
		LDAPUserInfo findUserByDistinguishedName(TQString dn);
		LDAPGroupInfoList findGroupsForUserByDistinguishedName(TQString dn);
		LDAPUserInfoList userList();
		LDAPGroupInfoList groupList();
		LDAPMachineInfoList machineList();

	private:
		LDAPUserInfo selectedUser();
		LDAPGroupInfo selectedGroup();
		LDAPMachineInfo selectedMachine();
		LDAPServiceInfo selectedService();
		int setPasswordForUser(LDAPUserInfo user, TQString *errstr);

	private:
		TDEAboutData *myAboutData;

		LDAPConfigBase *base;
		KSimpleConfig *m_systemconfig;
		LDAPManager *m_ldapmanager;

		LDAPUserInfoList m_userInfoList;
		LDAPGroupInfoList m_groupInfoList;
		LDAPMachineInfoList m_machineInfoList;
		LDAPServiceInfoList m_serviceInfoList;
};

#endif
