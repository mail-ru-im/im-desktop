#include "stdafx.h"
#include "NotificationCenterManager.h"
#include "../../../contact_list/ContactListModel.h"
#include "../../../../cache/avatars/AvatarStorage.h"
#include "../../../../main_window/MainWindow.h"
#include "../../../../utils/InterConnector.h"
#include "../../../../utils/utils.h"
#include "../../RecentMessagesAlert.h"
#include "main_window/LocalPIN.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSDockTile.h>

namespace
{
    constexpr std::chrono::milliseconds avatarTimeout = std::chrono::seconds(5);
    constexpr std::chrono::milliseconds displayTimeout = std::chrono::seconds(1);
}

static Ui::NotificationCenterManager * sharedCenter;

@interface ICQNotificationDelegate : NSObject<NSUserNotificationCenterDelegate>
@end

@implementation ICQNotificationDelegate

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didDeliverNotification:(NSUserNotification *)notification
{

}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    if (sharedCenter)
    {
        NSString * aimId = notification.userInfo[@"aimId"];
        NSString * mailId = notification.userInfo[@"mailId"];
        NSString * alertType = notification.userInfo[@"alertType"];
        NSString * messageId = notification.userInfo[@"messageId"];

        QString aimIdStr = QString::fromCFString((__bridge CFStringRef)aimId);
        QString mailIdStr = QString::fromCFString((__bridge CFStringRef)mailId);
        QString alertTypeStr = QString::fromCFString((__bridge CFStringRef)alertType);
        QString messageIdStr = QString::fromCFString((__bridge CFStringRef)messageId);

        sharedCenter->Activated(alertTypeStr, aimIdStr, mailIdStr, messageIdStr);
    }
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    return YES;
}

- (void)themeChanged:(NSNotification *)notification
{
    if (sharedCenter)
        sharedCenter->themeChanged();
}

- (void)screenLocked
{
    if (sharedCenter)
        sharedCenter->setScreenLocked(true);
}

- (void)screenUnlocked
{
    if (sharedCenter)
        sharedCenter->setScreenLocked(false);
}

@end

static ICQNotificationDelegate * notificationDelegate = nil;


static NSMutableArray* toDisplay_ = nil;

namespace Ui
{
    NotificationCenterManager::NotificationCenterManager()
        : avatarTimer_(new QTimer(this))
        , displayTimer_(new QTimer(this))
        , isScreenLocked_(false)
    {
        sharedCenter = this;

        notificationDelegate = [[ICQNotificationDelegate alloc] init];

        [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:notificationDelegate];

        NSDistributedNotificationCenter *center = [NSDistributedNotificationCenter defaultCenter];
        [center addObserver:notificationDelegate
                   selector: @selector(themeChanged:)
                       name:@"AppleInterfaceThemeChangedNotification"
                     object: nil];
        [center addObserver:notificationDelegate
                   selector:@selector(screenLocked)
                       name:@"com.apple.screenIsLocked"
                     object:nil];
        [center addObserver:notificationDelegate
                   selector:@selector(screenUnlocked)
                       name:@"com.apple.screenIsUnlocked"
                     object:nil];

        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &NotificationCenterManager::avatarChanged, Qt::QueuedConnection);

        connect(avatarTimer_, &QTimer::timeout, this, &NotificationCenterManager::avatarTimer);
        avatarTimer_->start(avatarTimeout.count());

        displayTimer_->setSingleShot(true);
        displayTimer_->setInterval(displayTimeout.count());
        connect(displayTimer_, &QTimer::timeout, this, &NotificationCenterManager::displayTimer);
    }

    NotificationCenterManager::~NotificationCenterManager()
    {
        sharedCenter = NULL;

        [[NSDistributedNotificationCenter defaultCenter] removeObserver:notificationDelegate];

        [notificationDelegate release];
        notificationDelegate = nil;

        if (toDisplay_ != nil)
        {
            [toDisplay_ release];
            toDisplay_ = nil;
        }
    }

    void NotificationCenterManager::HideNotifications(const QString& aimId)
    {
        if (aimId.isEmpty())
            return;

        NSUserNotificationCenter* center = [NSUserNotificationCenter defaultUserNotificationCenter];
        NSArray * notifications = [center deliveredNotifications];

        for (NSUserNotification* notif in notifications)
        {
            NSString * aimId_ = notif.userInfo[@"aimId"];
            QString str = QString::fromCFString((__bridge CFStringRef)aimId_);

            if (str == aimId)
            {
                [center removeDeliveredNotification:notif];
            }
        }

       HideQueuedNotifications(aimId);
    }

    void updateNotificationAvatar(NSUserNotification * notification, bool & isDefault)
    {
        if (QSysInfo().macVersion() <= QSysInfo::MV_10_8)
            return;

        QString aimId = QString::fromCFString((__bridge CFStringRef)notification.userInfo[@"aimId"]);
        QString displayName = QString::fromCFString((__bridge CFStringRef)notification.userInfo[@"displayName"]);

        if (aimId == ql1s("mail"))
        {
            auto i1 = displayName.indexOf(ql1c('<'));
            auto i2 = displayName.indexOf(ql1c('>'));
            if (i1 != -1 && i2 != -1)
                aimId = displayName.mid(i1 + 1, displayName.length() - i1 - (displayName.length() - i2 + 1));
        }

        // images in notifications must be 44x44
        const auto avaHeight = Utils::scale_value(44);
        const auto fullSize = Utils::avatarWithBadgeSize(avaHeight);
        QPixmap avatar(Utils::scale_bitmap(fullSize));
        avatar.fill(Qt::transparent);
        Utils::check_pixel_ratio(avatar);
        {
            QPainter p(&avatar);
            auto a = *Logic::GetAvatarStorage()->GetRounded(aimId, displayName, Utils::scale_bitmap(avaHeight), QString(), isDefault, false, false);
            if (a.isNull())
                return;
            Utils::check_pixel_ratio(a);
            Utils::drawAvatarWithBadge(p, QPoint(), a, aimId, true);
        }

        QByteArray bArray;
        QBuffer buffer(&bArray);
        buffer.open(QIODevice::WriteOnly);
        avatar.save(&buffer, "PNG");

        NSData* data = [NSData dataWithBytes:bArray.constData() length:bArray.size()];
        NSImage* ava = [[[NSImage alloc] initWithData: data] autorelease];

        NSImage* composedImage = [[[NSImage alloc] initWithSize:ava.size] autorelease];
        [composedImage lockFocus];
        NSRect imageFrame = NSMakeRect(0.f, 0.f, ava.size.width, ava.size.height);
        [ava drawInRect:imageFrame fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.f];
        [composedImage unlockFocus];

        notification.contentImage = composedImage;
    }

    void NotificationCenterManager::DisplayNotification(
        const QString& alertType,
        const QString& aimId,
        const QString& senderNick,
        const QString& message,
        const QString& mailId,
        const QString& displayName,
        const QString& messageId)
    {
        NSString * aimId_ = (NSString *)CFBridgingRelease(aimId.toCFString());
        NSString * displayName_ = (NSString *)CFBridgingRelease(displayName.toCFString());
        NSString * mailId_ = (NSString *)CFBridgingRelease(mailId.toCFString());
        NSString * alertType_ = (NSString *)CFBridgingRelease(alertType.toCFString());
        NSString * messageId_ = (NSString *)CFBridgingRelease(messageId.toCFString());

        NSUserNotification * notification = [[[NSUserNotification alloc] init] autorelease];

        notification.title = (NSString *)CFBridgingRelease(displayName.toCFString());
        notification.subtitle = (NSString *)CFBridgingRelease(senderNick.toCFString());
        notification.informativeText = (NSString *)CFBridgingRelease(message.toCFString());

        NSMutableDictionary * userInfo = [[@{@"aimId": aimId_, @"displayName": displayName_, @"mailId" : mailId_, @"alertType": alertType_, @"messageId": messageId_} mutableCopy] autorelease];

        notification.userInfo = userInfo;

        bool isDefault = false;
        updateNotificationAvatar(notification, isDefault);

        userInfo[@"isDefault"] = @(isDefault?YES:NO);
        notification.userInfo = userInfo;

        if (isDefault)
        {
            if (toDisplay_ == nil)
                toDisplay_ = [[NSMutableArray alloc] init];

            [toDisplay_ addObject: notification];

            displayTimer_->start();
            return;
        }

        reinstallDelegate();
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
    }

    void NotificationCenterManager::HideQueuedNotifications(const QString& _aimId)
    {
        NSMutableIndexSet* indices = [[NSMutableIndexSet alloc] init];
        for (int i = 0; i < [toDisplay_ count]; ++i)
        {
            NSUserNotification* notif  = [toDisplay_ objectAtIndex: i];
            NSString * aimId_ = notif.userInfo[@"aimId"];
            QString str = QString::fromCFString((__bridge CFStringRef) aimId_);

            if (str == _aimId)
            {
                [indices addIndex: i];
            }
        }

        [toDisplay_ removeObjectsAtIndexes: indices];
        [indices release];
    }

    void NotificationCenterManager::themeChanged()
    {
        emit osxThemeChanged();
    }

    void NotificationCenterManager::setScreenLocked(const bool _locked)
    {
        isScreenLocked_ = _locked;
        qDebug() << "screen is now" << (_locked? "locked" : "unlocked");

        if (LocalPIN::instance()->autoLockEnabled())
            LocalPIN::instance()->lock();
    }

    void NotificationCenterManager::reinstallDelegate()
    {
        auto notifCenter = [NSUserNotificationCenter defaultUserNotificationCenter];
        if (notificationDelegate && notifCenter && notifCenter.delegate != notificationDelegate)
            [notifCenter setDelegate:notificationDelegate];
    }

    AlertType string2AlertType(const QString& _type)
    {
        if (_type == ql1s("message"))
            return AlertType::alertTypeMessage;
        else if (_type == ql1s("mention"))
            return AlertType::alertTypeMentionMe;
        else if (_type == ql1s("mail"))
            return AlertType::alertTypeEmail;
        else
        {
            assert(false);
            return AlertType::alertTypeMessage;
        }
    }

    void NotificationCenterManager::Activated(
        const QString& alertType,
        const QString& aimId,
        const QString& mailId,
        const QString& messageId)
    {
        Utils::InterConnector::instance().getMainWindow()->closeGallery();

        emit messageClicked(aimId, mailId, messageId.toLongLong(), string2AlertType(alertType));

        HideNotifications(aimId);
    }

    void NotificationCenterManager::updateBadgeIcon(int _unreads)
    {
        const auto text = Utils::getUnreadsBadgeStr(_unreads);
        NSString* str = _unreads == 0 ? @"" : (NSString*)CFBridgingRelease(text.toCFString());

        NSDockTile* tile = [[NSApplication sharedApplication] dockTile];
        [tile setBadgeLabel:str];
    }

    void NotificationCenterManager::avatarChanged(QString aimId)
    {
        changedAvatars_.insert(aimId);
        if ([toDisplay_ count] == 0)
            return;

        for (NSUserNotification * notif in toDisplay_)
        {
            if (QString::fromCFString((__bridge CFStringRef)notif.userInfo[@"aimId"]) != aimId)
                continue;

            bool isDefault;
            updateNotificationAvatar(notif, isDefault);

            NSMutableDictionary * userInfo = [notif.userInfo.mutableCopy autorelease];

            userInfo[@"isDefault"] = @(isDefault?YES:NO);

            notif.userInfo = userInfo;
            [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notif];
            [toDisplay_ removeObject: notif];
        }
    }

    void NotificationCenterManager::avatarTimer()
    {
        if (changedAvatars_.empty())
            return;

        NSUserNotificationCenter* center = [NSUserNotificationCenter defaultUserNotificationCenter];
        NSArray * notifications = [center deliveredNotifications];

        for (NSUserNotification * notif in notifications)
        {
            bool isDefault = [notif.userInfo[@"isDefault"] boolValue] ? true : false;
            if (!isDefault)
            {
                continue;
            }

            QString aimId_ = QString::fromCFString((__bridge CFStringRef)notif.userInfo[@"aimId"]);

            auto iter_avatar = changedAvatars_.find(aimId_);
            if (iter_avatar == changedAvatars_.end())
            {
                continue;
            }

            updateNotificationAvatar(notif, isDefault);

            NSMutableDictionary * userInfo = [notif.userInfo.mutableCopy autorelease];

            userInfo[@"isDefault"] = @(isDefault?YES:NO);

            [center removeDeliveredNotification:notif];

            notif.userInfo = userInfo;
            [center deliverNotification:notif];

            changedAvatars_.erase(iter_avatar);
        }

        changedAvatars_.clear();
    }

    void NotificationCenterManager::displayTimer()
    {
        if ([toDisplay_ count] == 0)
            return;

        NSUserNotification* notif = [toDisplay_ firstObject];
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notif];
        [toDisplay_ removeObject:notif];

        if ([toDisplay_ count] != 0)
            displayTimer_->start();
    }

    void NotificationCenterManager::removeAllNotifications()
    {
        NSUserNotificationCenter* center = [NSUserNotificationCenter defaultUserNotificationCenter];
        [center removeAllDeliveredNotifications];
    }
}
