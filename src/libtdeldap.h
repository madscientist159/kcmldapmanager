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

#ifndef _LIBTDELDAP_H_
#define _LIBTDELDAP_H_

#include <unistd.h>
#include <ldap.h>

#include <tqobject.h>
#include <tqstring.h>
#include <tqdatetime.h>
#include <tqvaluelist.h>

// Values from hdb.asn1
enum LDAPKRB5Flags {
	KRB5_INITIAL			= 0x00000001,
	KRB5_FORWARDABLE		= 0x00000002,
	KRB5_PROXIABLE			= 0x00000004,
	KRB5_RENEWABLE			= 0x00000008,
	KRB5_POSTDATE			= 0x00000010,
	KRB5_SERVER			= 0x00000020,
	KRB5_CLIENT			= 0x00000040,
	KRB5_INVALID			= 0x00000080,
	KRB5_REQUIRE_PREAUTH		= 0x00000100,
	KRB5_CHANGE_PW			= 0x00000200,
	KRB5_REQUIRE_HWAUTH		= 0x00000400,
	KRB5_OK_AS_DELEGATE		= 0x00000800,
	KRB5_USER_TO_USER		= 0x00001000,
	KRB5_IMMUTABLE			= 0x00002000,
	KRB5_TRUSTED_FOR_DELEGATION	= 0x00004000,
	KRB5_ALLOW_KERBEROS_4		= 0x00008000,
	KRB5_ALLOW_DIGEST		= 0x00010000,
	KRB5_LOCKED_OUT			= 0x00020000,

	KRB5_ACTIVE_DEFAULT		= KRB5_FORWARDABLE | KRB5_RENEWABLE | KRB5_CLIENT | KRB5_CHANGE_PW,
	KRB5_DISABLED_ACCOUNT		= KRB5_FORWARDABLE | KRB5_SERVER | KRB5_INVALID | KRB5_REQUIRE_PREAUTH | KRB5_REQUIRE_HWAUTH | KRB5_OK_AS_DELEGATE | KRB5_USER_TO_USER,
	KRB5_FLAG_MAX			= 0x80000000
};

typedef TQValueList<uid_t> UserList;
typedef TQValueList<gid_t> GroupList;

class LDAPCredentials
{
	public:
		TQString username;
		TQCString password;
		TQString realm;
};

class LDAPUserInfo
{
	public:
		LDAPUserInfo();
		~LDAPUserInfo();

	public:
		bool informationValid;
		TQString distinguishedName;

		TQString name;
		uid_t uid;
		TQString shell;
		TQString homedir;
		gid_t primary_gid;
		LDAPKRB5Flags status;			// Default active user is 586 [KRB5_ACTIVE_DEFAULT] and locked out user is 7586 [KRB5_DISABLED_ACCOUNT]
		TQDateTime account_created;
		TQDateTime account_modified;
		TQDateTime password_last_changed;
		bool password_expires;
		TQDateTime password_expiration;
		bool password_ages;
		int new_password_interval;
		int new_password_warn_interval;
		int new_password_lockout_delay;
		bool password_has_minimum_age;
		int password_minimum_age;
		int maximum_ticket_lifetime;

		// Page 1
		TQString commonName;
		TQString givenName;
		TQString surName;
		TQString initials;
		TQString title;
		TQString email;
		TQString description;
		TQString locality;
		TQString telephoneNumber;
		TQString faxNumber;
		TQString homePhone;
		TQString mobilePhone;
		TQString pagerNumber;
		TQString website;

		// Page 2
		TQString poBox;
		TQString street;
		TQString address;
		TQString state;
		TQString postcode;
		TQString registeredAddress;
		TQString homeAddress;

		// Page 3
		TQString seeAlso;
		TQString deliveryOffice;
		TQString department;
		TQString roomNumber;
		TQString employeeType;
		TQString employeeNumber;
		TQString manager;
		TQString secretary;
		TQString isdnNumber;
		TQString teletexID;
		TQString telexNumber;
		TQString preferredDelivery;
		TQString destinationIndicator;
		TQString x121Address;
		TQString displayName;
		TQString preferredLanguage;
		TQString uniqueIdentifier;
		TQString businessCategory;
		TQString carLicense;
		TQString notes;
};

class LDAPGroupInfo
{
	public:
		LDAPGroupInfo();
		~LDAPGroupInfo();

	public:
		bool informationValid;
		TQString distinguishedName;

		TQString name;
		gid_t gid;
		TQStringList userlist;
};

typedef TQValueList<LDAPUserInfo> LDAPUserInfoList;
typedef TQValueList<LDAPGroupInfo> LDAPGroupInfoList;

class LDAPManager : public TQObject {
	Q_OBJECT

	public:
		LDAPManager(TQString realm, TQString host, TQObject *parent=0, const char *name=0);
		~LDAPManager();

		TQString realm();
		int bind();
		int unbind(bool force);
		LDAPUserInfoList users();
		LDAPGroupInfoList groups();
		LDAPUserInfo getUserByDistinguishedName(TQString dn);
		LDAPGroupInfo getGroupByDistinguishedName(TQString dn);

	private:
		LDAPUserInfo parseLDAPUserRecord(LDAPMessage* entry);
		LDAPGroupInfo parseLDAPGroupRecord(LDAPMessage* entry);

	private:
		TQString m_realm;
		TQString m_host;
		int m_port;
		TQString m_basedc;
		LDAPCredentials* m_creds;
		LDAP *m_ldap;
};

#endif // _LIBTDELDAP_H_