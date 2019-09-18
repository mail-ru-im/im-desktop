//
//  mac_migration.m
//  ICQ
//
//  Created by Vladimir Kubyshev on 25/02/16.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>

#import <Quartz/Quartz.h>

#include "stdafx.h"

#import "mac_migration.h"
#import "mac_support.h"

#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include "qmpreferences.h"
#include "../../../common.shared/constants.h"
#include "../gui_coll_helper.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/InterConnector.h"
#include "../../main_window/MainWindow.h"

#import "SSKeychain.h"
#import "NSData+Base64.h"

#import <CommonCrypto/CommonCrypto.h>

namespace
{
    static QString toQString(NSString * src)
    {
        return QString::fromCFString((__bridge CFStringRef)src);
    }

    static NSString * fromQString(const QString & src)
    {
        return (NSString *)CFBridgingRelease(src.toCFString());
    }

    QStringList plistAccounts(QString accountsPath)
    {
        QStringList list;

        NSArray * accounts = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:fromQString(accountsPath) error:nil];

        for (NSString * account in accounts)
        {
            list.append(toQString(account));
        }

        return list;
    }

    bool checkExists(QString path, bool dir)
    {
        QFileInfo checkFile(path);

        if (!checkFile.exists())
        {
            return false;
        }

        if ((dir && !checkFile.isDir()) ||
            (!dir && !checkFile.isFile()))
        {
            return false;
        }

        return true;
    }

    bool checkAccountDirectory(QString accountDirectory, QString identifier)
    {
        if (!checkExists(accountDirectory, true))
        {
            return false;
        }

        return MacMigrationManager::profiles2(accountDirectory, identifier).size() > 0;
    }

    bool createProtolibProfile(QString protoSettings, QString profileId, QString uin, bool isMain, MacProfilesList & list)
    {
        QMPreferences prefs(protoSettings);

        if (prefs.load())
        {
            QString aimsid = prefs.get<QString>(qsl("wim.session.aimsid"));
            QString displayId = prefs.get<QString>(qsl("wim.session.displayId"));
            QString fetchUrl = prefs.get<QString>(qsl("wim.session.fetchUrl"));
            time_t localTimeSkew = prefs.get<int>(qsl("wim.localTimeSkew"));
            QString authToken = prefs.get<QString>(qsl("wim.session.authToken"));
            QString sessionKey = prefs.get<QString>(qsl("wim.session.sessionKey"));

            MacProfile newProfile(MacProfileType::ICQ, profileId, uin);

            newProfile.setName(displayId);
            newProfile.setToken(authToken);
            newProfile.setKey(sessionKey);
            newProfile.setAimsid(aimsid);
            newProfile.setFetchUrl(fetchUrl);
            newProfile.setTimeOffset(localTimeSkew);

            if (isMain)
            {
                list.insert(0, newProfile);
            }
            else
            {
                list.append(newProfile);
            }

            return true;
        }

        return false;
    }
}

MacProfile::MacProfile():
    type_(MacProfileType::ICQ),
    timeOffset_(0),
    selected_(false)
{
}

MacProfile::MacProfile(const MacProfileType &type, const QString &identifier, const QString &uin, const QString &pw):
    type_(type),
    uin_(uin),
    pw_(pw),
    identifier_(identifier),
    selected_(false)
{
}

MacProfile::MacProfile(const MacProfile &profile):
    type_(profile.type()),
    uin_(profile.uin()),
    pw_(profile.pw()),
    name_(profile.name()),
    token_(profile.token()),
    key_(profile.key()),
    aimsid_(profile.aimsid()),
    identifier_(profile.identifier()),
    fetchUrl_(profile.fetchUrl()),
    timeOffset_(profile.timeOffset()),
    selected_(profile.selected())
{
}

MacProfile::~MacProfile()
{
}

void MacProfile::setName(const QString &name)
{
    name_ = name;
}

const QString & MacProfile::name() const
{
    return name_;
}

void MacProfile::setToken(const QString &token)
{
    token_ = token;
}

void MacProfile::setAimsid(const QString &aimsid)
{
    aimsid_ = aimsid;
}

void MacProfile::setKey(const QString &key)
{
    key_ = key;
}

void MacProfile::setFetchUrl(const QString &fetchUrl)
{
    fetchUrl_ = fetchUrl;
}

void MacProfile::setTimeOffset(time_t timeOffset)
{
    timeOffset_ = timeOffset;
}

void MacProfile::setSelected(bool selected)
{
    selected_ = selected;
}

const QString &MacProfile::uin() const
{
    return uin_;
}

const QString &MacProfile::pw() const
{
    return pw_;
}

const QString &MacProfile::identifier() const
{
    return identifier_;
}

const QString &MacProfile::token() const
{
    return token_;
}

const QString &MacProfile::aimsid() const
{
    return aimsid_;
}

const QString &MacProfile::key() const
{
    return key_;
}

const QString &MacProfile::fetchUrl() const
{
    return fetchUrl_;
}

time_t MacProfile::timeOffset() const
{
    return timeOffset_;
}

bool MacProfile::selected() const
{
    return selected_;
}

MacMigrationManager::MacMigrationManager(QObject* _parent)
: QObject(_parent)
, loginSeq_(-1)
{
}

MacMigrationManager::~MacMigrationManager()
{
}

void MacMigrationManager::init(QString accountId)
{
    accountId_ = accountId;
    if (accountId == ql1s("account"))
    {
        QString profilesPath = MacSupport::settingsPath().append(ql1s("/settings/profiles.dat"));
        QString generalPath = MacSupport::settingsPath().append(ql1s("/settings/general.dat"));

        profiles_ = MacMigrationManager::profiles1(profilesPath, generalPath);
    }
    else
    {
        QString accountsDirectory = MacSupport::settingsPath() % ql1s("/Accounts/") % accountId;

        profiles_ = MacMigrationManager::profiles2(accountsDirectory, accountId);
    }
}

MacProfilesList MacMigrationManager::profiles1(QString profilesPath, QString generalPath)
{
    NSData * dataProfiles = [NSData dataWithContentsOfFile:fromQString(profilesPath)];
    NSData * dataGenerals = [NSData dataWithContentsOfFile:fromQString(generalPath)];

    MacProfilesList list;

    NSDictionary * profiles = dataProfiles.length?
        [NSPropertyListSerialization propertyListWithData:dataProfiles options:0 format:0 error:nil]:
        nil;

    NSDictionary * generals = dataGenerals.length?
        [NSPropertyListSerialization propertyListWithData:dataGenerals options:0 format:0 error:nil]:
        nil;

    if (profiles.count && generals.count)
    {
        NSString * account = generals[@"account"][@"account_id"];

        for (NSDictionary * profile in profiles.allValues)
        {
            QString profileId = toQString(profile[@"key"]);

            QString uin = toQString(profile[@"uid"]);
            BOOL isMain = [profile[@"order"] integerValue] == 0;
            NSString * proto = profile[@"protocol"];

            if ([proto isEqualToString:@"wim"] || [proto isEqualToString:@"phone-icq"])
            {
                QString aimsid;
                QString displayId;
                QString fetchUrl;
                time_t localTimeSkew = 0;
                QString authToken;
                QString sessionKey;

                NSDictionary * params = generals[[NSString stringWithFormat:@"profile_%@", profile[@"key"]]];
                if (params.count)
                {
                    aimsid = toQString(params[@"aims-id"]);
                    displayId = toQString(params[@"display-id"]);
                    if (displayId.isEmpty())
                    {
                        displayId = toQString(params[@"nickname"]);
                    }
                    if (displayId.isEmpty())
                    {
                        displayId = uin;
                    }
                    fetchUrl = toQString(params[@"fetch-url"]);
                    localTimeSkew = [params[@"time-skew"] intValue];
                }

                NSString * keychainAcc = [NSString stringWithFormat:@"%@#%@", account.length?account:profile[@"key"], profile[@"key"]];

                NSString * keychainPass = [SSKeychain passwordForService:[NSString stringWithUTF8String:(build::is_icq() ? product_name_icq_mac_a : product_name_agent_mac_a)] account:keychainAcc];

                NSRange delimiter = [keychainPass rangeOfString:@"-"];
                if (delimiter.location != NSNotFound)
                {
                    NSString *sessionKeyString = [keychainPass substringToIndex:delimiter.location];
                    NSString *authTokenString = [keychainPass substringFromIndex:delimiter.location + 1];

                    //check for keychain record is token, not password with "-"
                    if (keychainPass.length > 10 &&
                        authTokenString.length > 10 &&
                        sessionKeyString.length > 10)
                    {
                        NSData * d = [NSData dataWithBase64EncodedString:authTokenString];
                        authToken = toQString([[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding]);

                        d = [NSData dataWithBase64EncodedString:sessionKeyString];
                        sessionKey = toQString([[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding]);
                    }
                }

                if (authToken.isEmpty() && keychainPass.length > 0)
                {
                    authToken = toQString(keychainPass);
                }

                MacProfile newProfile(MacProfileType::ICQ, profileId, uin);

                newProfile.setName(displayId);
                newProfile.setToken(authToken);
                newProfile.setKey(sessionKey);
                newProfile.setAimsid(aimsid);
                newProfile.setFetchUrl(fetchUrl);
                newProfile.setTimeOffset(localTimeSkew);

                if (isMain)
                {
                    list.insert(0, newProfile);
                }
                else
                {
                    list.append(newProfile);
                }
            }
            else if ([proto isEqualToString:@"mmp"])
            {
                QString displayId;
                time_t localTimeSkew = 0;

                NSDictionary * params = generals[[NSString stringWithFormat:@"profile_%@", profile[@"key"]]];

                if (params.count)
                {
                    displayId = toQString(params[@"nickname"]);
                    localTimeSkew = [params[@"time-skew"] intValue];
                }

                NSString * keychainAcc = [NSString stringWithFormat:@"%@#%@", account.length?account:profile[@"key"], profile[@"key"]];

                NSString * keychainPass = [SSKeychain passwordForService:[NSString stringWithUTF8String:(build::is_icq() ? product_name_icq_mac_a : product_name_agent_mac_a)] account:keychainAcc];

                MacProfile newProfile(MacProfileType::Agent, profileId, uin, toQString(keychainPass));
                newProfile.setName(displayId);
                newProfile.setTimeOffset(localTimeSkew);

                if (isMain)
                {
                    list.insert(0, newProfile);
                }
                else
                {
                    list.append(newProfile);
                }
            }
        }
    }

    return list;
}

MacProfilesList MacMigrationManager::profiles2(QString accountDirectory, QString account)
{
    QString settingsPath = accountDirectory % ql1s("/Settings.plist");

    NSData * data = [NSData dataWithContentsOfFile:fromQString(settingsPath)];

    MacProfilesList list;

    if (data.length)
    {
        NSDictionary * json = [NSPropertyListSerialization propertyListWithData:data options:0 format:0 error:nil];

        if (json.count)
        {
            QString identifier = toQString(json[@"identifier"]);
            if (account == identifier)
            {
                NSArray * profiles = json[@"profiles"];

                for (NSDictionary * profile in profiles)
                {
                    QString profileId = toQString(profile[@"identifier"]);
                    QString uin = toQString(profile[@"uid"]);
                    BOOL isMain = [profile[@"is-main"] boolValue];
                    NSString * proto = profile[@"protocol"];

                    if ([proto isEqualToString:@"wim"] ||
                        [proto isEqualToString:@"phone-icq"])
                    {
                        QString settings = MacSupport::settingsPath() % ql1c('/') % uin % ql1s("_wim.dat");

                        if (!createProtolibProfile(settings, profileId, uin, isMain, list))
                        {
                            settings.append(ql1s(".old"));

                            createProtolibProfile(settings, profileId, uin, isMain, list);
                        }
                    }
                }
            }
        }
    }

    return list;
}

QString MacMigrationManager::passwordMD5(QString pass)
{
    NSString * pw = fromQString(pass);

    const char *cStr = [pw UTF8String];
    unsigned char result[CC_MD5_DIGEST_LENGTH];
    CC_MD5( cStr, (CC_LONG)strlen(cStr), result );

    NSString * md5 = [NSString stringWithFormat:
                      @"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                      result[0], result[1], result[2], result[3],
                      result[4], result[5], result[6], result[7],
                      result[8], result[9], result[10], result[11],
                      result[12], result[13], result[14], result[15]
                      ];

    return toQString(md5);
}

bool MacMigrationManager::migrateProfile(const MacProfile &profile, bool isFirst)
{
    Ui::get_gui_settings()->set_value<bool>(settings_mac_accounts_migrated, true);

    NSMutableDictionary * json = [NSMutableDictionary dictionary];

    json[@"login"] = fromQString(profile.uin());
    json[@"aimid"] = fromQString(profile.uin());

    if (profile.type() == MacProfileType::ICQ)
    {
        if (profile.key().length())
        {
            json[@"timeoffset"] = @(profile.timeOffset());
            json[@"devid"] = @"ic18eTwFBO7vAdt9";
            json[@"atoken"] = fromQString(profile.token());
            json[@"sessionkey"] = fromQString(profile.key());
            json[@"aimsid"] = fromQString(profile.aimsid());
            json[@"fetchurl"] = fromQString(profile.fetchUrl());
        } else
        {
            json[@"password_md5"] = fromQString(profile.token());
        }
    }
    else if (profile.type() == MacProfileType::Agent)
    {
        json[@"password_md5"] = fromQString(passwordMD5(profile.pw()));
    }

    NSData * data = [NSJSONSerialization dataWithJSONObject:json options:NSJSONWritingPrettyPrinted error:nil];

    if (data.length)
    {
        QString settingsPath = MacSupport::settingsPath();

        settingsPath = settingsPath.append(ql1s("/0001/key/"));

        NSString * fileName = fromQString(settingsPath);
        [[NSFileManager defaultManager] createDirectoryAtPath:fileName withIntermediateDirectories:YES attributes:nil error:nil];

        settingsPath = settingsPath.append(isFirst ? ql1s(auth_export_file_name) : ql1s(auth_export_file_name_merge));

        fileName = fromQString(settingsPath);
        if ([data writeToFile:fileName atomically:YES])
        {
            if (isFirst)
            {
                Ui::GetDispatcher()->post_message_to_core("connect_after_migration", nullptr);
            }

            return true;
        }

        removePassword(profile);
    }

    return false;
}

void MacMigrationManager::loginProfile(const MacProfile &profile, const MacProfile& merge)
{
    QString pw = profile.pw();
    if (profile.type() == MacProfileType::ICQ && profile.key().isEmpty())
        pw = profile.token();

    if (!profile.uin().isEmpty() && !pw.isEmpty())
    {
        Ui::get_gui_settings()->set_value<bool>(settings_mac_accounts_migrated, true);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("login", profile.uin());
        collection.set_value_as_qstring("password", pw);
        collection.set_value_as_bool("save_auth_data", true);
        collection.set_value_as_bool("not_log", true);
        loginSeq_ = Ui::GetDispatcher()->post_message_to_core("login_by_password", collection.get());

        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResult(int64_t, int)), this, SLOT(loginResult(int64_t, int)), Qt::DirectConnection);

        profileToLogin_ = profile;
        profileToMerge_ = merge;
        return;
    }

    if (!profile.uin().isEmpty())
        migrateProfile(profile, true);
    if (!merge.uin().isEmpty())
        migrateProfile(merge, false);
}

QString MacMigrationManager::canMigrateAccount()
{
    //check new account settings (1.3.9, 2.x)

    QString checkPath = MacSupport::settingsPath().append(ql1s("/settings/profiles.dat"));

    if (checkExists(checkPath, false))
    {
        QString generalPath = MacSupport::settingsPath().append(ql1s("/settings/general.dat"));

        if (MacMigrationManager::profiles1(checkPath, generalPath).size() > 0)
        {
            return qsl("account");
        }
    }

    //check old account settings (1.3.x)

    checkPath = MacSupport::settingsPath().append(ql1s("/Accounts"));

    if (!checkExists(checkPath, true))
    {
        return QString();
    }

    QStringList accounts = plistAccounts(checkPath);

    for (int i = 0; i < accounts.size(); i++)
    {
        QString account = accounts[i];

        QString accPath = checkPath % ql1c('/') % account;

        if (checkAccountDirectory(accPath, account))
        {
            return account;
        }
    }

    return QString();
}

void MacMigrationManager::loginResult(int64_t _seq, int _result)
{
    if (loginSeq_ != _seq)
        return;

    bool withMerge = !profileToMerge_.uin().isEmpty();

    QObject::disconnect(Ui::GetDispatcher(), SIGNAL(loginResult(int64_t, int)), this, SLOT(loginResult(int64_t, int)));

    if (_result != 0)
    {
        migrateProfile(profileToLogin_, true);
        if (withMerge)
            migrateProfile(profileToMerge_, false);
    }
    else
    {
        Ui::get_gui_settings()->set_value<bool>(settings_mac_accounts_migrated, true);
        if (withMerge)
            mergeProfile(profileToMerge_);
    }

    removePassword(profileToLogin_);
    if (withMerge)
        removePassword(profileToMerge_);

    profileToLogin_ = MacProfile();
    profileToMerge_ = MacProfile();

    auto w = Utils::InterConnector::instance().getMainWindow();
    if (w)
        w->showMainPage();
}

void MacMigrationManager::removePassword(const MacProfile &profile)
{
    if (profile.identifier().isEmpty())
        return;

    NSString * keychainAcc = [NSString stringWithFormat:@"%@#%@", profile.identifier().toNSString(), profile.identifier().toNSString()];
    [SSKeychain deletePasswordForService: [NSString stringWithUTF8String:(build::is_icq() ? product_name_icq_mac_a : product_name_agent_mac_a)] account: keychainAcc];
}

void MacMigrationManager::mergeProfile(const MacProfile &profile)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("login", profile.uin());
    collection.set_value_as_qstring("aimid", profile.uin());

    if (profile.type() == MacProfileType::ICQ)
    {
        if (profile.key().length())
        {
            collection.set_value_as_int64("timeoffset", profile.timeOffset());
            collection.set_value_as_qstring("devid", ql1s("ic18eTwFBO7vAdt9"));
            collection.set_value_as_qstring("atoken", profile.token());
            collection.set_value_as_qstring("sessionkey", profile.key());
            collection.set_value_as_qstring("aimsid", profile.aimsid());
            collection.set_value_as_qstring("fetchurl", profile.fetchUrl());
        }
        else
        {
            collection.set_value_as_qstring("password_md5", profile.token());
        }
    }
    else if (profile.type() == MacProfileType::Agent)
    {
        collection.set_value_as_qstring("password_md5", passwordMD5(profile.pw()));
    }

    Ui::GetDispatcher()->post_message_to_core("merge_account", collection.get());
}
