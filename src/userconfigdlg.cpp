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

#include <tdelocale.h>
#include <klineedit.h>
#include <ktextedit.h>
#include <knuminput.h>
#include <ktempdir.h>
#include <tdetempfile.h>
#include <kstandarddirs.h>
#include <tdemessagebox.h>
#include <tdefiledialog.h>
#include <tdeactionselector.h>
#include <tqlistbox.h>
#include <kpushbutton.h>
#include <tqpixmap.h>
#include <tqiconset.h>
#include <tqlabel.h>
#include <kurlrequester.h>
#include <kcombobox.h>
#include <tqradiobutton.h>
#include <tqcheckbox.h>
#include <kdatewidget.h>
#include <kdatetimewidget.h>
#include <kpassdlg.h>
#include <kiconloader.h>
#include <ksslcertificate.h>

#include "ldapmgr.h"
#include "userconfigdlg.h"

UserConfigDialog::UserConfigDialog(LDAPUserInfo user, LDAPConfig* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("LDAP User Properties"), Ok|Cancel, Ok, true), m_user(user), m_ldapconfig(parent)
{
	m_base = new LDAPUserConfigBase(this);
	setMainWidget(m_base);

	TQStringList availableShells = TQStringList::split(" ", AVAILABLE_SHELLS);
	for ( TQStringList::Iterator it = availableShells.begin(); it != availableShells.end(); ++it ) {
		m_base->shell->insertItem(*it, -1);
	}
	if (user.distinguishedName != "") {
		m_base->loginName->setEnabled(false);
	}
	m_base->lastChanged->setEnabled(false);

	m_base->detailsIcon->setPixmap(SmallIcon("personal.png"));
	m_base->enabledIcon->setPixmap(SmallIcon("decrypted.png"));
	m_base->disabledIcon->setPixmap(SmallIcon("encrypted.png"));
	m_base->userIcon->setPixmap(SmallIcon("personal.png"));
	m_base->groupsIcon->setPixmap(SmallIcon("tdmconfig.png"));
	m_base->passwordIcon->setPixmap(SmallIcon("password.png"));
	m_base->certificateIcon->setPixmap(SmallIcon("password.png"));

	connect(m_base->loginName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->realName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->surName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->homeDirectory, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->passwordExpireEnabled, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->passwordExpireDisabled, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->requirePasswordAging, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->requirePasswordMinAge, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->primaryGroup, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->certGenPrivateKey, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->certPrivateKeyFileName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->certPublicCertFileName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->createCertificate, TQT_SIGNAL(clicked()), this, TQT_SLOT(createPKICertificate()));
	connect(m_base->revokeCertificate, TQT_SIGNAL(clicked()), this, TQT_SLOT(revokePKICertificate()));
	connect(m_base->downloadCertificate, TQT_SIGNAL(clicked()), this, TQT_SLOT(downloadPKICertificate()));
	connect(m_base->certPKIDatabaseList, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(processLockouts()));
	connect(m_base->certPKIDatabaseList, TQT_SIGNAL(executed(TQListViewItem*)), this, TQT_SLOT(downloadPKICertificate()));

	if (m_user.status == KRB5_DISABLED_ACCOUNT) {
		m_base->userStatusEnabled->setChecked(false);
		m_base->userStatusDisabled->setChecked(true);
	}
	else {
		m_base->userStatusEnabled->setChecked(true);
		m_base->userStatusDisabled->setChecked(false);
	}
	m_base->loginName->setText(m_user.name);
	m_base->realName->setText(m_user.commonName);
	m_base->UID->setValue(m_user.uid);

	LDAPGroupInfoList groupList = m_ldapconfig->groupList();
	LDAPGroupInfoList::Iterator it;
	for (it = groupList.begin(); it != groupList.end(); ++it) {
		m_base->primaryGroup->insertItem((*it).name, -1);
	}
	m_base->primaryGroup->setCurrentItem(m_ldapconfig->findGroupInfoByGID(TQString("%1").arg(m_user.primary_gid)).name, false, -1);
	m_prevPrimaryGroup = m_base->primaryGroup->currentText();

	m_base->homeDirectory->setURL(m_user.homedir);
	m_base->shell->setEditText(m_user.shell);

	for (it = groupList.begin(); it != groupList.end(); ++it) {
		LDAPGroupInfo group = *it;
		TQCheckListItem* item = new TQCheckListItem(m_base->secondary_group_list, group.name, TQCheckListItem::CheckBox);
		item->setOn(group.userlist.contains(m_user.distinguishedName));
	}

	m_base->lastChanged->setText(m_user.password_last_changed.toString(TQt::TextDate));
	if (m_user.password_expires) {
		m_base->passwordExpireEnabled->setChecked(true);
		m_base->passwordExpireDisabled->setChecked(false);
	}
	else {
		m_base->passwordExpireEnabled->setChecked(false);
		m_base->passwordExpireDisabled->setChecked(true);
	}
	m_base->expirationDate->setDateTime(m_user.password_expiration);
	m_base->requirePasswordAging->setChecked(m_user.password_ages);
	m_base->requirePasswordInterval->setValue(m_user.new_password_interval/24);
	m_base->warnPasswordExpireInterval->setValue(m_user.new_password_warn_interval/24);
	m_base->disablePasswordDelay->setValue(m_user.new_password_lockout_delay/24);
	m_base->requirePasswordMinAge->setChecked(m_user.password_has_minimum_age);
	m_base->passwordMinAge->setValue(m_user.password_minimum_age/24);

	// User information
	m_base->givenName->setText(m_user.givenName);
	m_base->surName->setText(m_user.surName);
	m_base->initials->setText(m_user.initials);
	m_base->title->setText(m_user.title);
	m_base->description->setText(m_user.description);
	m_base->office->setText(m_user.deliveryOffice);
	m_base->telephoneNumber->setText(m_user.telephoneNumber);
	m_base->faxNumber->setText(m_user.faxNumber);
	m_base->email->setText(m_user.email);

	// Certificate generation information
	TQDateTime suggestedExpiration = TQDateTime::currentDateTime().addDays(KERBEROS_PKI_KRB_EXPIRY_DAYS);
	m_base->certificateExpirationDate->setDate(suggestedExpiration.date());

	m_base->certPKIDatabaseList->setAllColumnsShowFocus(true);
	m_base->certPKIDatabaseList->setFullWidth(true);
	updatePKICertificateList();

	processLockouts();
}

void UserConfigDialog::slotOk() {
	// Update data
	if (m_base->userStatusEnabled->isOn() == true) {
		m_user.status = KRB5_ACTIVE_DEFAULT;
	}
	else {
		m_user.status = KRB5_DISABLED_ACCOUNT;
	}
	m_user.commonName = m_base->realName->text();
	m_user.uid = m_base->UID->value();
	m_user.primary_gid = m_ldapconfig->findGroupInfoByName(m_base->primaryGroup->currentText()).gid;
	m_user.homedir = m_base->homeDirectory->url();
	m_user.shell = m_base->shell->currentText();

	m_user.new_password = m_base->passwordEntry->password();
	if (m_base->passwordExpireEnabled->isOn() == true) {
		m_user.password_expires = true;
	}
	else {
		m_user.password_expires = false;
	}

	m_user.password_expiration = m_base->expirationDate->dateTime();
	m_user.password_ages = m_base->requirePasswordAging->isOn();
	m_user.new_password_interval = m_base->requirePasswordInterval->value()*24;
	m_user.new_password_warn_interval = m_base->warnPasswordExpireInterval->value()*24;
	m_user.new_password_lockout_delay = m_base->disablePasswordDelay->value()*24;
	m_user.password_has_minimum_age = m_base->requirePasswordMinAge->isOn();
	m_user.password_minimum_age = m_base->passwordMinAge->value()*24;

	selectedGroups.clear();
	TQListViewItemIterator it(m_base->secondary_group_list);
	while ( it.current() ) {
		TQCheckListItem* itm = dynamic_cast<TQCheckListItem*>(it.current());
		if (itm) {
			if (itm->isOn()) {
				selectedGroups.append(itm->text());
			}
		}
		++it;
	}


	// User information
	m_user.givenName = m_base->givenName->text();
	m_user.surName = m_base->surName->text();
	m_user.initials = m_base->initials->text();
	m_user.title = m_base->title->text();
	m_user.description = m_base->description->text();
	m_user.deliveryOffice = m_base->office->text();
	m_user.telephoneNumber = m_base->telephoneNumber->text();
	m_user.faxNumber = m_base->faxNumber->text();
	m_user.email = m_base->email->text();

	// Special handler for new group
	if (m_user.distinguishedName == "") {
		m_user.name = m_base->loginName->text();
	}

	accept();
}

void UserConfigDialog::processLockouts() {
	if (m_base->passwordExpireEnabled->isChecked()) {
		m_base->expirationDate->setEnabled(true);
	}
	else {
		m_base->expirationDate->setEnabled(false);
	}

	if (m_base->requirePasswordAging->isChecked()) {
		m_base->requirePasswordInterval->setEnabled(true);
		m_base->warnPasswordExpireInterval->setEnabled(true);
		m_base->disablePasswordDelay->setEnabled(true);
	}
	else {
		m_base->requirePasswordInterval->setEnabled(false);
		m_base->warnPasswordExpireInterval->setEnabled(false);
		m_base->disablePasswordDelay->setEnabled(false);
	}

	if (m_base->requirePasswordMinAge->isChecked()) {
		m_base->passwordMinAge->setEnabled(true);
	}
	else {
		m_base->passwordMinAge->setEnabled(false);
	}

	// Disable the primary group checkbox in the group list
	TQListViewItemIterator it(m_base->secondary_group_list);
	while (it.current()) {
		if (it.current()->text(0) == m_base->primaryGroup->currentText()) {
			dynamic_cast<TQCheckListItem*>(it.current())->setOn(true);
			it.current()->setEnabled(false);
		}
		else {
			it.current()->setEnabled(true);
			if (it.current()->text(0) == m_prevPrimaryGroup) {
				dynamic_cast<TQCheckListItem*>(it.current())->setOn(false);
			}
		}
		++it;
	}

	bool ok_enabled = true;

	// Special handler for new group
	if ((m_user.distinguishedName == "") && (m_base->loginName->text() == "")) {
		ok_enabled = false;
	}
	if (m_base->realName->text() == "") {
		ok_enabled = false;
	}
	if (m_base->surName->text() == "") {
		ok_enabled = false;
	}
	if (m_base->homeDirectory->url() == "") {
		ok_enabled = false;
	}
	enableButton(KDialogBase::Ok, ok_enabled);

	if (m_base->certPrivateKeyFileName->url() == "") {
		ok_enabled = false;
	}
	if (m_base->certPublicCertFileName->url() == "") {
		ok_enabled = false;
	}
	if (!m_base->certGenPrivateKey->isChecked()) {
		if (!TQFile(m_base->certPrivateKeyFileName->url()).exists()) {
			ok_enabled = false;
		}
	}
	m_base->createCertificate->setEnabled(ok_enabled);

	TQListViewItem* lvi = m_base->certPKIDatabaseList->selectedItem();
	if (lvi) {
		if (lvi->text(1) != i18n("Revoked")) {
			m_base->revokeCertificate->setEnabled(true);
			m_base->downloadCertificate->setEnabled(true);
		}
		else {
			m_base->revokeCertificate->setEnabled(false);
			m_base->downloadCertificate->setEnabled(true);
		}
	}
	else {
		m_base->revokeCertificate->setEnabled(false);
		m_base->downloadCertificate->setEnabled(false);
	}

	m_prevPrimaryGroup = m_base->primaryGroup->currentText();
}

void UserConfigDialog::createPKICertificate() {
	int ret;
	TQString errorstring;
	LDAPRealmConfigList realms = LDAPManager::fetchAndReadTDERealmList();

	int expirydays = TQDate::currentDate().daysTo(m_base->certificateExpirationDate->date());

	if (m_base->certGenPrivateKey->isChecked()) {
		// Generate new private key
		// FIXME
		// Make RSA private key length user-configurable (currently locked to 2048 bits)
		if (LDAPManager::generateClientCertificatePrivateKey(m_base->certPrivateKeyFileName->url(), 2048, &errorstring) != 0) {
			KMessageBox::sorry(this, i18n("<qt><b>Unable to generate new private key</b><p>Details: %1</qt>").arg(errorstring), i18n("Unable to Obtain Certificate"));
			return;
		}
	}

	// Get the CA root private key from LDAP
	// WARNING
	// Anyone with access to this key would be able to create accounts that could access any resource on the realm!
	// Secure the key file accordingly...
	KTempFile caPrivateKeyTempFile(locateLocal("tmp", "krbcakey"), ".key.pem", 0600);
	caPrivateKeyTempFile.setAutoDelete(true);
	TQFile* caPrivateKeyFile = caPrivateKeyTempFile.file();
	if (!caPrivateKeyFile) {
		KMessageBox::sorry(this, i18n("<qt><b>Unable to obtain root certificate for realm %1!</b><p>Details: %2</qt>").arg(realms[m_ldapconfig->m_ldapmanager->realm()].name.upper()).arg(i18n("Unable to create or open temporary file '%s'").arg(caPrivateKeyTempFile.name())), i18n("Unable to Obtain Certificate"));
		return;
	}
	if (m_ldapconfig->m_ldapmanager->getTDECertificate("privateRootCertificateKey", caPrivateKeyFile, &errorstring) != 0) {
		KMessageBox::sorry(this, i18n("<qt><b>Unable to obtain root certificate for realm %1!</b><p>Details: %2</qt>").arg(realms[m_ldapconfig->m_ldapmanager->realm()].name.upper()).arg(errorstring), i18n("Unable to Obtain Certificate"));
		return;
	}
	caPrivateKeyTempFile.sync();

	ret = LDAPManager::generateClientCertificatePublicCertificate(expirydays, m_user, realms[m_ldapconfig->m_ldapmanager->realm()], caPrivateKeyTempFile.name(), m_base->certPrivateKeyFileName->url(), m_base->certPublicCertFileName->url());

	// Delete the private key as soon as possible after certificate signing
	caPrivateKeyTempFile.unlink();

	if (ret != 0) {
		KMessageBox::sorry(this, i18n("<qt><b>Unable to generate or sign certificate</b><p>Details: %1</qt>").arg(errorstring), i18n("Unable to Create Certificate"));
	}

	// Upload new certificate to LDAP server
	PKICertificateEntry certEntry;
	certEntry.first = PKICertificateStatus::Valid;
	TQFile certfile(m_base->certPublicCertFileName->url());
	if (certfile.open(IO_ReadOnly)) {
		certEntry.second = certfile.readAll();
		m_user.pkiCertificates.append(certEntry);
		if (m_ldapconfig->m_ldapmanager->writePKICertificateFilesIntoDirectory(m_user, "pkiCertificate", &errorstring) != 0) {
			m_user.pkiCertificates.remove(certEntry);
			KMessageBox::sorry(this, i18n("<qt><b>Unable to upload certificate to server</b><p>Details: %1</qt>").arg(errorstring), i18n("Unable to Upload Certificate"));
		}
	}
	else {
		KMessageBox::sorry(this, i18n("<qt><b>Unable to upload certificate to server</b><p>Details: %1</qt>").arg(i18n("Unable to open certificate file")), i18n("Unable to Upload Certificate"));
	}

	updatePKICertificateList();
}

void UserConfigDialog::downloadPKICertificate() {
	TQString errorstring;
	PKICertificateEntryList originalCertList = m_user.pkiCertificates;

	TQListViewItem* lvi = m_base->certPKIDatabaseList->selectedItem();
	if (lvi) {
		TQString fileName = KFileDialog::getSaveFileName(TQString::null, "*.pem", 0, i18n("Save Certificate"));
		if (fileName != "") {
			// Find the certificate
			PKICertificateEntryList::Iterator it;
			for (it = m_user.pkiCertificates.begin(); it != m_user.pkiCertificates.end(); ++it) {
				PKICertificateEntry certificateData = *it;

				TQCString ssldata(certificateData.second);
				ssldata[certificateData.second.size()] = 0;
				ssldata.replace("-----BEGIN CERTIFICATE-----", "");
				ssldata.replace("-----END CERTIFICATE-----", "");
				ssldata.replace("\n", "");
				KSSLCertificate* cert = KSSLCertificate::fromString(ssldata);
				if (cert) {
					if ((cert->getSerialNumber() == lvi->text(0))
						&& (cert->getQDTNotBefore().toString() == lvi->text(2))
						&& (cert->getQDTNotAfter().toString() == lvi->text(3))) {
						TQFile certfile(fileName);
						if (certfile.open(IO_WriteOnly)) {
							certfile.writeBlock(certificateData.second);
						}
						else {
							KMessageBox::sorry(this, i18n("<qt><b>Unable to download certificate</b><p>Details: %1</qt>").arg(i18n("Could not open file '%s' for writing").arg(fileName)), i18n("Unable to Download Certificate"));
						}
						break;
					}
				}
			}
		}
	}
}

void UserConfigDialog::revokePKICertificate() {
	int ret;
	TQString errorstring;
	PKICertificateEntryList originalCertList = m_user.pkiCertificates;
	LDAPRealmConfigList realms = LDAPManager::fetchAndReadTDERealmList();

	KTempDir tempDir = KTempDir(locateLocal("tmp", "tdekrb"));
	tempDir.setAutoDelete(true);

	TQListViewItem* lvi = m_base->certPKIDatabaseList->selectedItem();
	if (lvi) {
		if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to revoke the certificate with serial number %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(lvi->text(0)), i18n("Confirmation Required")) == KMessageBox::Yes) {
			// Find the certificate
			PKICertificateEntryList::Iterator it;
			for (it = m_user.pkiCertificates.begin(); it != m_user.pkiCertificates.end(); ++it) {
				PKICertificateEntry certificateData = *it;

				TQCString ssldata(certificateData.second);
				ssldata[certificateData.second.size()] = 0;
				ssldata.replace("-----BEGIN CERTIFICATE-----", "");
				ssldata.replace("-----END CERTIFICATE-----", "");
				ssldata.replace("\n", "");
				KSSLCertificate* cert = KSSLCertificate::fromString(ssldata);
				if (cert) {
					if ((cert->getSerialNumber() == lvi->text(0))
						&& (cert->getQDTNotBefore().toString() == lvi->text(2))
						&& (cert->getQDTNotAfter().toString() == lvi->text(3))) {
						(*it).first = PKICertificateStatus::Revoked;
						break;
					}
				}
			}

			// Commit updates to the LDAP database
			if (m_ldapconfig->m_ldapmanager->writePKICertificateFilesIntoDirectory(m_user, "pkiCertificate", &errorstring) == 0) {
				// Get the configured CRL validity duration from LDAP
				int expiryDays;
				TQString expiryString;
				if (m_ldapconfig->m_ldapmanager->getLdapCertificateStoreAttribute("publicRootCRLIntervalDays", &expiryString, &errorstring) == 0) {
					expiryDays = expiryString.toInt();
					if (expiryDays < 1) {
						expiryDays = KERBEROS_PKI_CRL_EXPIRY_DAYS;
					}

					// Get the CA root private key from LDAP
					// WARNING
					// Anyone with access to this key would be able to create accounts that could access any resource on the realm!
					// Secure the key file accordingly...
					KTempFile caPrivateKeyTempFile(locateLocal("tmp", "krbcakey"), ".key.pem", 0600);
					caPrivateKeyTempFile.setAutoDelete(true);
					TQFile* caPrivateKeyFile = caPrivateKeyTempFile.file();
					if (!caPrivateKeyFile) {
						KMessageBox::sorry(this, i18n("<qt><b>Unable to obtain root certificate for realm %1!</b><p>Details: %2</qt>").arg(realms[m_ldapconfig->m_ldapmanager->realm()].name.upper()).arg(i18n("Unable to create or open temporary file '%s'").arg(caPrivateKeyTempFile.name())), i18n("Unable to Obtain Certificate"));
						return;
					}
					if (m_ldapconfig->m_ldapmanager->getTDECertificate("privateRootCertificateKey", caPrivateKeyFile, &errorstring) != 0) {
						KMessageBox::sorry(this, i18n("<qt><b>Unable to obtain root certificate for realm %1!</b><p>Details: %2</qt>").arg(realms[m_ldapconfig->m_ldapmanager->realm()].name.upper()).arg(errorstring), i18n("Unable to Obtain Certificate"));
						return;
					}
					caPrivateKeyTempFile.sync();

					// Regenerate CRL
					ret = m_ldapconfig->m_ldapmanager->generatePKICRL(expiryDays, realms[m_ldapconfig->m_ldapmanager->realm()], tempDir.name() + "crl.pem", caPrivateKeyTempFile.name(), tempDir.name() + "ca.db", &errorstring);

					// Delete the private key as soon as possible after certificate signing
					caPrivateKeyTempFile.unlink();

					if (ret != 0) {
						KMessageBox::error(this, i18n("<qt><b>Unable to regenerate CRL</b><br>The revoked certificate may still be able to access resources on the realm<p>Details: %1</qt>").arg(errorstring), i18n("Unable to Regenerate CRL"));
					}
				}
				else {
					KMessageBox::error(this, i18n("<qt><b>Unable to regenerate CRL</b><br>The revoked certificate may still be able to access resources on the realm<p>Details: %1</qt>").arg(errorstring), i18n("Unable to Regenerate CRL"));
				}
			}
			else {
				m_user.pkiCertificates = originalCertList;
				KMessageBox::sorry(this, i18n("<qt><b>Unable to modify certificate status on server</b><p>Details: %1</qt>").arg(errorstring), i18n("Unable to Modify Certificate Status"));
			}
		}
	}

	updatePKICertificateList();
}

void UserConfigDialog::updatePKICertificateList() {
	m_base->certPKIDatabaseList->clear();

	PKICertificateEntryList::Iterator it;
	for (it = m_user.pkiCertificates.begin(); it != m_user.pkiCertificates.end(); ++it) {
		PKICertificateEntry certificateData = *it;

		TQCString ssldata(certificateData.second);
		ssldata[certificateData.second.size()] = 0;
		ssldata.replace("-----BEGIN CERTIFICATE-----", "");
		ssldata.replace("-----END CERTIFICATE-----", "");
		ssldata.replace("\n", "");
		KSSLCertificate* cert = KSSLCertificate::fromString(ssldata);
		if (cert) {
			TQString status = i18n("Invalid");
			if (certificateData.first == PKICertificateStatus::Valid) {
				if (TQDateTime::currentDateTime(Qt::UTC) > cert->getQDTNotAfter()) {
					status = i18n("Expired");
				}
				else {
					if (TQDateTime::currentDateTime(Qt::UTC) < cert->getQDTNotBefore()) {
						status = i18n("Future Valid");
					}
					else {
						status = i18n("Valid");
					}
				}
			}
			if (certificateData.first == PKICertificateStatus::Revoked) {
				status = i18n("Revoked");
			}
			new TQListViewItem(m_base->certPKIDatabaseList, cert->getSerialNumber(), status, cert->getQDTNotBefore().toString(), cert->getQDTNotAfter().toString());
			delete cert;
		}
	}

	processLockouts();
}

LDAPUserInfo UserConfigDialog::userProperties() {
	return m_user;
}

#include "userconfigdlg.moc"
