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

#include <klocale.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kpassdlg.h>

#include <ldap.h>
#include <stdlib.h>
#include <sys/time.h>

#include "libtdeldap.h"
#include "ldappasswddlg.h"

#define LDAP_INSECURE_PORT 389
#define LDAP_SECURE_PORT 636

int requested_ldap_version = LDAP_VERSION3;
int requested_ldap_auth_method = LDAP_AUTH_SIMPLE;	// Is this safe and secure over an untrusted connection?
char* ldap_user_and_operational_attributes[2] = {"*", "+"};

LDAPManager::LDAPManager(TQString realm, TQString host, TQObject *parent, const char *name) : TQObject(parent, name), m_realm(realm), m_host(host), m_port(0), m_creds(0), m_ldap(0)
{
	TQStringList domainChunks = TQStringList::split(".", realm.lower());
	m_basedc = "dc=" + domainChunks.join(",dc=");
}

LDAPManager::~LDAPManager() {
	unbind(true);
}

TQString LDAPManager::realm() {
	return m_realm;
}

int LDAPManager::bind() {
	if (m_ldap) {
		return 0;
	}

	int use_secure_connection = 0;

	TQString uri;
	if (use_secure_connection == 1) {
		m_port = LDAP_SECURE_PORT;
		uri = TQString("ldaps://%1:%2").arg(m_host).arg(m_port);
	}
	else {
		m_port = LDAP_INSECURE_PORT;
		uri = TQString("ldap://%1:%2").arg(m_host).arg(m_port);
	}

	int retcode = ldap_initialize(&m_ldap, uri.ascii());
	if (retcode < 0) {
		KMessageBox::error(0, i18n("<qt>Unable to connect to LDAP server %1 on port %2<p>Reason: [%3] %4</qt>").arg(m_host).arg(m_port).arg(retcode).arg(ldap_err2string(retcode)), i18n("Unable to connect to server!"));
		return -1;
	}
	retcode = ldap_set_option(m_ldap, LDAP_OPT_PROTOCOL_VERSION, &requested_ldap_version);
	if (retcode != LDAP_OPT_SUCCESS) {
		KMessageBox::error(0, i18n("<qt>Unable to connect to LDAP server %1 on port %2<p>Reason: [%3] %4</qt>").arg(m_host).arg(m_port).arg(retcode).arg(ldap_err2string(retcode)), i18n("Unable to connect to server!"));
		return -1;
	}

	TQString errorString;
	LDAPPasswordDialog passdlg(0);
	passdlg.m_base->ldapAdminRealm->setEnabled(false);
	passdlg.m_base->ldapAdminRealm->insertItem(m_realm);
	if (passdlg.exec() == TQDialog::Accepted) {
		char* mechanism = NULL;
		struct berval cred;
		TQString ldap_dn = passdlg.m_base->ldapAdminUsername->text();
		TQCString pass = passdlg.m_base->ldapAdminPassword->password();
		cred.bv_val = pass.data();
		cred.bv_len = pass.length();

		if (!ldap_dn.contains(",")) {
			// Look for a POSIX account with anonymous bind and the specified account name
			TQString uri;
			LDAP* ldapconn;
			if (use_secure_connection == 1) {
				m_port = LDAP_SECURE_PORT;
				uri = TQString("ldaps://%1:%2").arg(m_host).arg(m_port);
			}
			else {
				m_port = LDAP_INSECURE_PORT;
				uri = TQString("ldap://%1:%2").arg(m_host).arg(m_port);
			}
			int retcode = ldap_initialize(&ldapconn, uri.ascii());
			if (retcode < 0) {
				KMessageBox::error(0, i18n("<qt>Unable to connect to LDAP server %1 on port %2<p>Reason: [%3] %4</qt>").arg(m_host).arg(m_port).arg(retcode).arg(ldap_err2string(retcode)), i18n("Unable to connect to server!"));
				return -1;
			}
			retcode = ldap_set_option(ldapconn, LDAP_OPT_PROTOCOL_VERSION, &requested_ldap_version);
			if (retcode != LDAP_OPT_SUCCESS) {
				KMessageBox::error(0, i18n("<qt>Unable to connect to LDAP server %1 on port %2<p>Reason: [%3] %4</qt>").arg(m_host).arg(m_port).arg(retcode).arg(ldap_err2string(retcode)), i18n("Unable to connect to server!"));
				return -1;
			}
			struct berval anoncred;
			anoncred.bv_val = "";
			anoncred.bv_len = strlen("");
			retcode = ldap_sasl_bind_s(ldapconn, "", mechanism, &anoncred, NULL, NULL, NULL);
			if (retcode == LDAP_SUCCESS ) {
				// Look for the DN for the specified user
				LDAPMessage* msg;
				TQString ldap_base_dn = m_basedc;
				TQString ldap_filter = TQString("(&(objectclass=posixAccount)(uid=%1))").arg(passdlg.m_base->ldapAdminUsername->text());
				struct timeval timeout;
				timeout.tv_sec = 10;	// 10 second timeout
				retcode = ldap_search_ext_s(ldapconn, ldap_base_dn.ascii(), LDAP_SCOPE_SUBTREE, ldap_filter.ascii(), NULL, 0, NULL, NULL, &timeout, 0, &msg);
				if (retcode != LDAP_SUCCESS) {
					KMessageBox::error(0, i18n("<qt>LDAP search failure<p>Reason: [%3] %4</qt>").arg(retcode).arg(ldap_err2string(retcode)), i18n("LDAP Error"));
				}
				else {
					// Iterate through the returned entries
					char* dn = NULL;
					LDAPMessage* entry;
					for(entry = ldap_first_entry(ldapconn, msg); entry != NULL; entry = ldap_next_entry(ldapconn, entry)) {
						if((dn = ldap_get_dn(ldapconn, entry)) != NULL) {
							ldap_dn = dn;
							ldap_memfree(dn);
						}
					}
				}
				// clean up
				ldap_msgfree(msg);

				// All done!
				ldap_unbind_ext_s(ldapconn, NULL, NULL);
			}
		}

		retcode = ldap_sasl_bind_s(m_ldap, ldap_dn.ascii(), mechanism, &cred, NULL, NULL, NULL);

		if (retcode != LDAP_SUCCESS ) {
			KMessageBox::error(0, i18n("<qt>Unable to connect to LDAP server %1 on port %2<p>Reason: [%3] %4</qt>").arg(m_host).arg(m_port).arg(retcode).arg(ldap_err2string(retcode)), i18n("Unable to connect to server!"));
			return -1;
		}

		return 0;
	}
	else {
		return -2;
	}

	return -3;
}

int LDAPManager::unbind(bool force) {
	if (!m_ldap) {
		return 0;
	}

	int retcode = ldap_unbind_ext_s(m_ldap, NULL, NULL);
	if ((retcode < 0) && (force == false)) {
		KMessageBox::error(0, i18n("<qt>Unable to disconnect from LDAP server %1 on port %2<p>Reason: [%3] %4</qt>").arg(m_host).arg(m_port).arg(retcode).arg(ldap_err2string(retcode)), i18n("Unable to disconnect from server!"));
		return retcode;
	}
	else {
		m_ldap = 0;
	}
	return retcode;
}

LDAPUserInfoList LDAPManager::users() {
	int retcode;
	LDAPUserInfoList users;
printf("[RAJA DEBUG 100.0] In LDAPManager::users()\n\r"); fflush(stdout);

	if (bind() < 0) {
		return LDAPUserInfoList();
	}
	else {
printf("[RAJA DEBUG 100.1] In LDAPManager::users() bind was OK\n\r"); fflush(stdout);
		LDAPMessage* msg;
		TQString ldap_base_dn = m_basedc;
		TQString ldap_filter = "(objectClass=posixAccount)";
		struct timeval timeout;
		timeout.tv_sec = 10;	// 10 second timeout
		retcode = ldap_search_ext_s(m_ldap, ldap_base_dn.ascii(), LDAP_SCOPE_SUBTREE, ldap_filter.ascii(), ldap_user_and_operational_attributes, 0, NULL, NULL, &timeout, 0, &msg);
		if (retcode != LDAP_SUCCESS) {
			KMessageBox::error(0, i18n("<qt>LDAP search failure<p>Reason: [%3] %4</qt>").arg(retcode).arg(ldap_err2string(retcode)), i18n("LDAP Error"));
			return LDAPUserInfoList();
		}
		
printf("[RAJA DEBUG 100.2] The number of entries returned was %d\n\n", ldap_count_entries(m_ldap, msg));

		// Iterate through the returned entries
		char* dn = NULL;
		char* attr;
		struct berval **vals;
		BerElement* ber;
		LDAPMessage* entry;
		int i;
		for(entry = ldap_first_entry(m_ldap, msg); entry != NULL; entry = ldap_next_entry(m_ldap, entry)) {
			LDAPUserInfo userinfo;

			if((dn = ldap_get_dn(m_ldap, entry)) != NULL) {
				printf("Returned dn: %s\n", dn);
				userinfo.distinguishedName = dn;
				TQStringList dnParts = TQStringList::split(",", dn);
				TQString id = dnParts[0];
				if (id.startsWith("uid=")) {
					id = id.remove(0, 4);
					userinfo.name = id;
				}
				ldap_memfree(dn);
			}

			for( attr = ldap_first_attribute(m_ldap, entry, &ber); attr != NULL; attr = ldap_next_attribute(m_ldap, entry, ber)) {
				if ((vals = ldap_get_values_len(m_ldap, entry, attr)) != NULL)  {
printf("[RAJA DEBUG 100.3] %s: %s\n\r", attr, vals[i]->bv_val);
					userinfo.informationValid = true;
					TQString ldap_field = attr;
					i=0;
					if (ldap_field == "uidNumber") {
						userinfo.uid = atoi(vals[i]->bv_val);
					}
					else if (ldap_field == "loginShell") {
						userinfo.shell = vals[i]->bv_val;
					}
					else if (ldap_field == "homeDirectory") {
						userinfo.homedir = vals[i]->bv_val;
					}
					else if (ldap_field == "gidNumber") {
						userinfo.primary_gid = atoi(vals[i]->bv_val);
					}
					else if (ldap_field == "krb5KDCFlags") {
						userinfo.status = (LDAPKRB5Flags)(atoi(vals[i]->bv_val));
					}
					else if (ldap_field == "createTimestamp") {		// YYYYMMDD000000Z
						TQString formattedDate = vals[i]->bv_val;
						formattedDate.insert(4,"-");
						formattedDate.insert(7,"-");
						formattedDate.insert(10,"T");
						formattedDate.insert(13,":");
						formattedDate.insert(16,":");
						formattedDate.remove(19, 1);
						userinfo.account_created = TQDateTime::fromString(formattedDate, TQt::ISODate);
					}
					else if (ldap_field == "modifyTimestamp") {		// YYYYMMDD000000Z
						TQString formattedDate = vals[i]->bv_val;
						formattedDate.insert(4,"-");
						formattedDate.insert(7,"-");
						formattedDate.insert(10,"T");
						formattedDate.insert(13,":");
						formattedDate.insert(16,":");
						formattedDate.remove(19, 1);
						userinfo.account_modified = TQDateTime::fromString(formattedDate, TQt::ISODate);
					}
						// FIXME
						// These two attributes do not seem to be available with a Heimdal KDC
						// userinfo.password_last_changed = vals[i]->bv_val;
						// userinfo.password_expires = vals[i]->bv_val;
					else if (ldap_field == "krb5PasswordEnd") {		// YYYYMMDD000000Z
						TQString formattedDate = vals[i]->bv_val;
						formattedDate.insert(4,"-");
						formattedDate.insert(7,"-");
						formattedDate.insert(10,"T");
						formattedDate.insert(13,":");
						formattedDate.insert(16,":");
						formattedDate.remove(19, 1);
						userinfo.password_expiration = TQDateTime::fromString(formattedDate, TQt::ISODate);
					}
						// FIXME
						// These six(!) attributes do not seem to be available with a Heimdal KDC
						// userinfo.password_ages = vals[i]->bv_val;
						// userinfo.new_password_interval = vals[i]->bv_val;
						// userinfo.new_password_warn_interval = vals[i]->bv_val;
						// userinfo.new_password_lockout_delay = vals[i]->bv_val;
						// userinfo.password_has_minimum_age = vals[i]->bv_val;
						// userinfo.password_minimum_age = vals[i]->bv_val;
					else if (ldap_field == "krb5MaxLife") {			// units: hours
						userinfo.maximum_ticket_lifetime = atoi(vals[i]->bv_val);
					}
					else if (ldap_field == "cn") {
						userinfo.commonName = vals[i]->bv_val;
					}
					else if (ldap_field == "givenName") {
						userinfo.givenName = vals[i]->bv_val;
					}
					else if (ldap_field == "sn") {
						userinfo.surName = vals[i]->bv_val;
					}
					else if (ldap_field == "initials") {
						userinfo.initials = vals[i]->bv_val;
					}
					else if (ldap_field == "title") {
						userinfo.title = vals[i]->bv_val;
					}
					else if (ldap_field == "mail") {
						userinfo.email = vals[i]->bv_val;
					}
					else if (ldap_field == "description") {
						userinfo.description = vals[i]->bv_val;
					}
					else if (ldap_field == "l") {
						userinfo.locality = vals[i]->bv_val;
					}
					else if (ldap_field == "telephoneNumber") {
						userinfo.telephoneNumber = vals[i]->bv_val;
					}
					else if (ldap_field == "facsimileTelephoneNumber") {
						userinfo.faxNumber = vals[i]->bv_val;
					}
					else if (ldap_field == "homePhone") {
						userinfo.homePhone = vals[i]->bv_val;
					}
					else if (ldap_field == "mobile") {
						userinfo.mobilePhone = vals[i]->bv_val;
					}
					else if (ldap_field == "pager") {
						userinfo.pagerNumber = vals[i]->bv_val;
					}
						// FIXME
						// This attribute is not present in my current LDAP schema
						// userinfo.website = vals[i]->bv_val;
					else if (ldap_field == "postOfficeBox") {
						userinfo.poBox = vals[i]->bv_val;
					}
					else if (ldap_field == "street") {
						userinfo.street = vals[i]->bv_val;
					}
					else if (ldap_field == "postalAddress") {
						userinfo.address = vals[i]->bv_val;
					}
					else if (ldap_field == "st") {
						userinfo.state = vals[i]->bv_val;
					}
					else if (ldap_field == "postalCode") {
						userinfo.postcode = vals[i]->bv_val;
					}
					else if (ldap_field == "registeredAddress") {
						userinfo.registeredAddress = vals[i]->bv_val;
					}
					else if (ldap_field == "homePostalAddress") {
						userinfo.homeAddress = vals[i]->bv_val;
					}
					else if (ldap_field == "seeAlso") {
						userinfo.seeAlso = vals[i]->bv_val;
					}
					else if (ldap_field == "physicalDeliveryOfficeName") {
						userinfo.deliveryOffice = vals[i]->bv_val;
					}
					else if (ldap_field == "departmentNumber") {
						userinfo.department = vals[i]->bv_val;
					}
					else if (ldap_field == "roomNumber") {
						userinfo.roomNumber = vals[i]->bv_val;
					}
					else if (ldap_field == "employeeType") {
						userinfo.employeeType = vals[i]->bv_val;
					}
					else if (ldap_field == "employeeNumber") {
						userinfo.employeeNumber = vals[i]->bv_val;
					}
						// FIXME
						// These two attributes are not present in my current LDAP schema
// 						userinfo.manager = vals[i]->bv_val;
// 						userinfo.secretary = vals[i]->bv_val;
					else if (ldap_field == "internationaliSDNNumber") {
						userinfo.isdnNumber = vals[i]->bv_val;
					}
						// FIXME
						// This attribute is not present in my current LDAP schema
// 						userinfo.teletexID = vals[i]->bv_val;
					else if (ldap_field == "telexNumber") {
						userinfo.telexNumber = vals[i]->bv_val;
					}
						// FIXME
						// This attribute is not present in my current LDAP schema
// 						userinfo.preferredDelivery = vals[i]->bv_val;
					else if (ldap_field == "destinationIndicator") {
						userinfo.destinationIndicator = vals[i]->bv_val;
					}
					else if (ldap_field == "x121Address") {
						userinfo.x121Address = vals[i]->bv_val;
					}
					else if (ldap_field == "displayName") {
						userinfo.displayName = vals[i]->bv_val;
					}
					else if (ldap_field == "preferredLanguage") {
						userinfo.preferredLanguage = vals[i]->bv_val;
					}
						// FIXME
						// This attribute is not present in my current LDAP schema
// 						userinfo.uniqueIdentifier = vals[i]->bv_val;
					else if (ldap_field == "preferredLanguage") {
						userinfo.businessCategory = vals[i]->bv_val;
					}
					else if (ldap_field == "carLicense") {
						userinfo.carLicense = vals[i]->bv_val;
					}
						// FIXME
						// This attribute is not present in my current LDAP schema
// 						userinfo.notes = vals[i]->bv_val;
					ldap_value_free_len(vals);
				}
				ldap_memfree(attr);
			}
			users.append(userinfo);

			if (ber != NULL) {
				ber_free(ber, 0);
			}

			printf("\n\r");
		}
		
		// clean up
		ldap_msgfree(msg);

		// RAJA FIXME
		return users;
	}

	return LDAPUserInfoList();
}

LDAPGroupInfoList LDAPManager::groups() {
	int retcode;
	LDAPGroupInfoList groups;
printf("[RAJA DEBUG 110.0] In LDAPManager::groups()\n\r"); fflush(stdout);

	if (bind() < 0) {
		return LDAPGroupInfoList();
	}
	else {
printf("[RAJA DEBUG 110.1] In LDAPManager::groups() bind was OK\n\r"); fflush(stdout);
		LDAPMessage* msg;
		TQString ldap_base_dn = m_basedc;
		TQString ldap_filter = "(objectClass=posixGroup)";
		struct timeval timeout;
		timeout.tv_sec = 10;	// 10 second timeout
		retcode = ldap_search_ext_s(m_ldap, ldap_base_dn.ascii(), LDAP_SCOPE_SUBTREE, ldap_filter.ascii(), ldap_user_and_operational_attributes, 0, NULL, NULL, &timeout, 0, &msg);
		if (retcode != LDAP_SUCCESS) {
			KMessageBox::error(0, i18n("<qt>LDAP search failure<p>Reason: [%3] %4</qt>").arg(retcode).arg(ldap_err2string(retcode)), i18n("LDAP Error"));
			return LDAPGroupInfoList();
		}
		
printf("[RAJA DEBUG 110.2] The number of entries returned was %d\n\n", ldap_count_entries(m_ldap, msg));

		// Iterate through the returned entries
		char* dn = NULL;
		char* attr;
		struct berval **vals;
		BerElement* ber;
		LDAPMessage* entry;
		int i;
		for(entry = ldap_first_entry(m_ldap, msg); entry != NULL; entry = ldap_next_entry(m_ldap, entry)) {
			LDAPGroupInfo groupinfo;

			if((dn = ldap_get_dn(m_ldap, entry)) != NULL) {
				printf("Returned dn: %s\n", dn);
				groupinfo.distinguishedName = dn;
				TQStringList dnParts = TQStringList::split(",", dn);
				TQString id = dnParts[0];
				if (id.startsWith("cn=")) {
					id = id.remove(0, 3);
					groupinfo.name = id;
				}
				else {
					continue;
				}
				ldap_memfree(dn);
			}

			for( attr = ldap_first_attribute(m_ldap, entry, &ber); attr != NULL; attr = ldap_next_attribute(m_ldap, entry, ber)) {
				if ((vals = ldap_get_values_len(m_ldap, entry, attr)) != NULL)  {
for(i = 0; vals[i] != NULL; i++) {
	printf("[RAJA DEBUG 110.3] %s: %s\n\r", attr, vals[i]->bv_val);
}
					groupinfo.informationValid = true;
					TQString ldap_field = attr;
					i=0;
					if (ldap_field == "member") {
						TQStringList members;
						for(i = 0; vals[i] != NULL; i++) {
							TQString userdn = vals[i]->bv_val;
							if (userdn.startsWith("cn=placeholder,dc=")) {
								continue;
							}
							members.append(userdn);
						}
						groupinfo.userlist = members;
					}
					else if (ldap_field == "gidNumber") {
						groupinfo.gid = atoi(vals[i]->bv_val);
					}
					ldap_value_free_len(vals);
				}
				ldap_memfree(attr);
			}
			groups.append(groupinfo);

			if (ber != NULL) {
				ber_free(ber, 0);
			}

			printf("\n\r");
		}
		
		// clean up
		ldap_msgfree(msg);

		// RAJA FIXME
		return groups;
	}

	return LDAPGroupInfoList();
}

// ===============================================================================================================
//
// DATA CLASS CONSTRUCTORS AND DESTRUCTORS
//
// ===============================================================================================================

LDAPUserInfo::LDAPUserInfo() {
	// TQStrings are always initialized to TQString::null, so they don't need initialization here...
	informationValid = false;

	uid = -1;
	primary_gid = -1;
	status = (LDAPKRB5Flags)0;
	account_created = TQDateTime::fromString("1970-01-01T00:00:00", TQt::ISODate);
	account_modified = TQDateTime::fromString("1970-01-01T00:00:00", TQt::ISODate);
	password_last_changed = TQDateTime::fromString("1970-01-01T00:00:00", TQt::ISODate);
	password_expires = false;
	password_expiration = TQDateTime::fromString("1970-01-01T00:00:00", TQt::ISODate);
	password_ages = false;
	new_password_interval = -1;
	new_password_warn_interval = -1;
	new_password_lockout_delay = -1;
	password_has_minimum_age = false;
	password_minimum_age = -1;
	maximum_ticket_lifetime = -1;
}

LDAPUserInfo::~LDAPUserInfo() {
	//
}

LDAPGroupInfo::LDAPGroupInfo() {
	// TQStrings are always initialized to TQString::null, so they don't need initialization here...
	informationValid = false;

	gid = -1;
}

LDAPGroupInfo::~LDAPGroupInfo() {
	//
}

#include "libtdeldap.moc"