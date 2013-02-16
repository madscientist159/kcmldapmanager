/***************************************************************************
 *   Copyright (C) 2013 by Timothy Pearson                                 *
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

#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pwd.h>

#include <tdeapplication.h>
#include <tdestartupinfo.h>
#include <tdecmdlineargs.h>
#include <kuniqueapplication.h>
#include <tdeaboutdata.h>

#include <ksimpleconfig.h>

#include <tqdatetime.h>
#include <tqfile.h>

#include <libtdeldap.h>

// FIXME
// Connect this to CMake/Automake
#define KDE_CONFDIR "/etc/trinity"

static const char description[] =
	I18N_NOOP("TDE utility for managing a Kerberos realm");

static const char version[] = "v0.0.1";

static const TDECmdLineOptions options[] =
{
	{ "forcepwchangenextlogin", I18N_NOOP("Force the user to change password on next login"), 0 },
	{ "username <username>", I18N_NOOP("Specifies the user name in the Kerberos realm (mandatory)"), 0 },
	{ "uid <user id>", I18N_NOOP("Specifies the POSIX user ID in the Kerberos realm"), 0 },
	{ "password <password>", I18N_NOOP("Sets the password for the specified account to the given value"), 0 },
	{ "displayname <full name>", I18N_NOOP("Sets the display name (common name) of the specified account to the given value"), 0 },
	{ "homedirectory <full path>", I18N_NOOP("Sets the home directory of the specified account to the given value"), 0 },
	{ "givenname <first name>", I18N_NOOP("Sets the first name of the specified account to the given value"), 0 },
	{ "surname <last name>", I18N_NOOP("Sets the last name of the specified account to the given value"), 0 },
	{ "group <groupname>", I18N_NOOP("Sets membership of the specified account in the groups listed on the command line, and revokes membership in any groups not listed.  This option may be used multiple times."), 0 },
	{ "revokeallgroups", I18N_NOOP("Revokes membership of the specified account for all groups"), 0 },
	{ "adminusername <username>", I18N_NOOP("Specifies the username of the administrative user with permissions to perform the requested task"), 0 },
	{ "adminpasswordfile <password file>", I18N_NOOP("Specifies the location of a file which contains the password of the administrative user"), 0 },
	{ "!+command", I18N_NOOP("The command to execute on the Kerberos realm.  Valid commands are: adduser deluser"), 0 },
	{ "!+realm", I18N_NOOP("The Kerberos realm on which to execute the specified command.  Example: MY.REALM"), 0 },
	{ "", I18N_NOOP("This utility will use GSSAPI to connect to the realm controller.  You must own an active, valid Kerberos ticket in order to use this utility!"), 0 },
	TDECmdLineLastOption // End of options.
};

int main(int argc, char *argv[])
{
	TDEAboutData aboutData( "tdeldapmanager", I18N_NOOP("Kerberos Realm Manager"),
		version, description, TDEAboutData::License_GPL,
		"(c) 2013, Timothy Pearson");
		aboutData.addAuthor("Timothy Pearson",0, "kb9vqf@pearsoncomputing.net");
	TDECmdLineArgs::init(argc, argv, &aboutData);
 	TDECmdLineArgs::addCmdLineOptions(options);
	KUniqueApplication::addCmdLineOptions();
	TDEApplication::disableAutoDcopRegistration();

	TDEApplication app(false, false);

	TDEStartupInfo::appStarted();

	KSimpleConfig systemconfig( TQString::fromLatin1( KDE_CONFDIR "/ldap/ldapconfigrc" ));

	//======================================================================================================================================================
	//
	// Manager code follows
	//
	//======================================================================================================================================================

	// FIXME
	// forcepwchangenextlogin not implemented!

	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
	if (args->count() > 1) {
		int retcode;

		TQString command = TQString(args->arg(0));
		TQString realm = TQString(args->arg(1));

		systemconfig.setGroup("LDAPRealm-" + realm);
		TQString host = systemconfig.readEntry("admin_server");
		LDAPCredentials credentials;
		if (args->isSet("adminusername") && args->isSet("adminpasswordfile")) {
			TQString passFileName = args->getOption("adminpasswordfile");
			TQFile passFile(passFileName);
			if (!passFile.open(IO_ReadOnly)) {
				printf("[ERROR] Unable to open specified password file '%s'\n\r", passFileName.ascii()); fflush(stdout);
				return -1;
			}
			TQTextStream stream(&passFile);
			credentials.username = args->getOption("adminusername");
			credentials.password = stream.readLine();
			passFile.close();
		}
		else {
			credentials.use_gssapi = true;
		}
		credentials.realm = realm;
		LDAPManager ldapmanager(realm, host, &credentials);

		if (command == "adduser") {
			LDAPUserInfo user;

			TQString errorString;
			if (ldapmanager.bind(&errorString) != 0) {
				printf("[ERROR] Unable to bind to Kerberos realm controller\n\r[ERROR] Detailed debugging information: %s\n\r", errorString.ascii());
				return -1;
			}

			LDAPUserInfoList userInfoList = ldapmanager.users(&retcode);
			if (retcode != 0) {
				printf("[ERROR] Unable to retrieve list of users from realm controller\n\r");
				return -1;
			}
			LDAPGroupInfoList groupInfoList = ldapmanager.groups(&retcode);
			if (retcode != 0) {
				printf("[ERROR] Unable to retrieve list of users from realm controller\n\r");
				return -1;
			}

			// Find the next available, reasonable UID
			if (args->isSet("uid")) {
				uid_t uid = atoi(args->getOption("uid"));
				LDAPUserInfoList::Iterator it;
				for (it = userInfoList.begin(); it != userInfoList.end(); ++it) {
					LDAPUserInfo user = *it;
					if (user.uid == uid) {
						printf("[ERROR] The specified POSIX user ID is already in  use\n\r");
						return -1;
					}
				}
				user.uid = uid;
			}
			else {
				uid_t uid = 100;
				LDAPUserInfoList::Iterator it;
				for (it = userInfoList.begin(); it != userInfoList.end(); ++it) {
					LDAPUserInfo user = *it;
					if (user.uid >= uid) {
						uid = user.uid + 1;
					}
				}
				user.uid = uid;
			}

			if (!args->isSet("username")) {
				printf("[ERROR] You must specify a username when adding a user\n\r");
				return -1;
			}
			if (!args->isSet("surname")) {
				printf("[ERROR] You must specify a surname when adding a user\n\r");
				return -1;
			}

			// Get user data
			user.name = args->getOption("username");
			user.new_password = args->getOption("password");
			user.givenName = args->getOption("givenname");
			user.surName = args->getOption("surname");
			if (args->isSet("displayname")) {
				user.commonName = args->getOption("displayname");
			}
			else {
				user.commonName = user.givenName + " " + user.surName;
			}
			if (args->isSet("homedirectory")) {
				user.homedir = args->getOption("homedirectory");
			}
			else {
				user.homedir = "/home/" + user.name;
			}

			// Get list of groups
			QCStringList groupList = args->getOptionList("group");

			// Try to find a reasonable place to stuff the new entry
			// Do any users exist right now?
			if (userInfoList.begin() != userInfoList.end()) {
				user.distinguishedName = (*userInfoList.begin()).distinguishedName;
				int eqpos = user.distinguishedName.find("=")+1;
				int cmpos = user.distinguishedName.find(",", eqpos);
				user.distinguishedName.remove(eqpos, cmpos-eqpos);
				user.distinguishedName.insert(eqpos, user.name);
			}
			else {
				user.distinguishedName = "uid=" + user.name + "," + ldapmanager.basedn();
			}
			if (ldapmanager.addUserInfo(user, &errorString) == 0) {
				// Modify group(s) as needed
				bool revoke_all = args->isSet("revokeallgroups");
				if ((groupList.count() > 0) || revoke_all) {
					LDAPGroupInfoList groupInfoList = ldapmanager.groups(&retcode);
					if (retcode != 0) {
						printf("[ERROR] Unable to retrieve list of users from realm controller\n\r");
						return -1;
					}
					LDAPGroupInfoList::Iterator it;
					for (it = groupInfoList.begin(); it != groupInfoList.end(); ++it) {
						LDAPGroupInfo group = *it;
						if ((!revoke_all) && (groupList.contains(group.name.ascii()))) {
							// Make sure that we are in this group!
							if (!group.userlist.contains(user.distinguishedName)) {
								group.userlist.append(user.distinguishedName);
								ldapmanager.updateGroupInfo(group, &errorString);
							}
						}
						else {
							// Make sure that we are NOT in this group!
							if (group.userlist.contains(user.distinguishedName)) {
								group.userlist.remove(user.distinguishedName);
								ldapmanager.updateGroupInfo(group, &errorString);
							}
						}
					}
				}

				if (user.new_password != "") {
					// If a new password was set, use Kerberos to set it on the server
					if (ldapmanager.setPasswordForUser(user, &errorString) != 0) {
						printf("[ERROR] Unable to set password for user\n\r[ERROR] Detailed debugging information: %s\n\r", errorString.ascii());
					}
				}
			}
			else {
				printf("[ERROR] Unable to add user with distingushed name '%s'\n\r[ERROR] Detailed debugging information: %s\n\r", user.distinguishedName.ascii(), errorString.ascii());
			}
		}
		else if (command == "deluser") {
			LDAPUserInfo deluser;

			TQString errorString;
			if (ldapmanager.bind(&errorString) != 0) {
				printf("[ERROR] Unable to bind to Kerberos realm controller\n\r[ERROR] Detailed debugging information: %s\n\r", errorString.ascii());
				return -1;
			}

			LDAPUserInfoList userInfoList = ldapmanager.users(&retcode);
			if (retcode != 0) {
				printf("[ERROR] Unable to retrieve list of users from realm controller\n\r");
				return -1;
			}

			if (!args->isSet("username")) {
				printf("[ERROR] You must specify a username when deleting a user\n\r");
				return -1;
			}

			TQString delUserName = args->getOption("username");

			bool found = false;
			LDAPUserInfoList::Iterator it;
			for (it = userInfoList.begin(); it != userInfoList.end(); ++it) {
				LDAPUserInfo user = *it;
				if (user.name == delUserName) {
					found = true;
					deluser = user;
					break;
				}
			}
			if (found) {
				ldapmanager.deleteUserInfo(deluser);
			}
			else {
				printf("[ERROR] User not found\n\r");
				return -1;
			}
			// FIXME
		}
		else {
			TDECmdLineArgs::usage(i18n("An invalid command was specified"));
			return -1;
		}
	}
	else {
		if (args->count() > 0) {
			TDECmdLineArgs::usage(i18n("No Kerberos realm was specified"));
			return -1;
		}
		else {
			TDECmdLineArgs::usage(i18n("No command was specified"));
			return -1;
		}
	}

	//======================================================================================================================================================

	return 0;
}
