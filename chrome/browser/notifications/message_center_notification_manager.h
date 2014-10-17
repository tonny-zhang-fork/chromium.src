// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_MESSAGE_CENTER_NOTIFICATION_MANAGER_H_
#define CHROME_BROWSER_NOTIFICATIONS_MESSAGE_CENTER_NOTIFICATION_MANAGER_H_

#include <map>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "base/prefs/pref_member.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/notifications/google_now_notification_stats_collector.h"
#include "chrome/browser/notifications/message_center_stats_collector.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_system_observer.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_observer.h"
#include "ui/message_center/message_center_tray_delegate.h"
#include "ui/message_center/message_center_types.h"

class MessageCenterSettingsController;
class Notification;
class PrefRegistrySimple;
class PrefService;
class Profile;

namespace message_center {
class NotificationBlocker;
FORWARD_DECLARE_TEST(WebNotificationTrayTest, ManuallyCloseMessageCenter);
}

// This class extends NotificationUIManagerImpl and delegates actual display
// of notifications to MessageCenter, doing necessary conversions.
class MessageCenterNotificationManager
    : public NotificationUIManager,
      public message_center::MessageCenterObserver {
 public:
  MessageCenterNotificationManager(
      message_center::MessageCenter* message_center,
      PrefService* local_state,
      scoped_ptr<message_center::NotifierSettingsProvider> settings_provider);
  virtual ~MessageCenterNotificationManager();

  // Registers preferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // NotificationUIManager
  virtual void Add(const Notification& notification,
                   Profile* profile) override;
  virtual bool Update(const Notification& notification,
                      Profile* profile) override;
  virtual const Notification* FindById(const std::string& delegate_id,
                                       ProfileID profile_id) const override;
  virtual bool CancelById(const std::string& delegate_id,
                          ProfileID profile_id) override;
  virtual std::set<std::string> GetAllIdsByProfileAndSourceOrigin(
      Profile* profile,
      const GURL& source) override;
  virtual bool CancelAllBySourceOrigin(const GURL& source_origin) override;
  virtual bool CancelAllByProfile(ProfileID profile_id) override;
  virtual void CancelAll() override;

  // MessageCenterObserver
  virtual void OnNotificationRemoved(const std::string& notification_id,
                                     bool by_user) override;
  virtual void OnCenterVisibilityChanged(message_center::Visibility) override;
  virtual void OnNotificationUpdated(const std::string& notification_id)
      override;

  void EnsureMessageCenterClosed();

#if defined(OS_WIN)
  // Called when the pref changes for the first run balloon. The first run
  // balloon is only displayed on Windows, since the visibility of the tray
  // icon is limited.
  void DisplayFirstRunBalloon();

  void SetFirstRunTimeoutForTest(base::TimeDelta timeout);
  bool FirstRunTimerIsActive() const;
#endif

  // Takes ownership of |delegate|.
  void SetMessageCenterTrayDelegateForTest(
      message_center::MessageCenterTrayDelegate* delegate);

  // Returns the notification id which this manager will use to add to message
  // center, for this combination of delegate id and profile.
  std::string GetMessageCenterNotificationIdForTest(
      const std::string& delegate_id, Profile* profile);

 private:
  FRIEND_TEST_ALL_PREFIXES(message_center::WebNotificationTrayTest,
                           ManuallyCloseMessageCenter);

  // This class keeps a set of original Notification objects and corresponding
  // Profiles, so when MessageCenter calls back with a notification_id, this
  // class has necessary mapping to other source info - for example, it calls
  // NotificationDelegate supplied by client when someone clicks on a
  // Notification in MessageCenter. Likewise, if a Profile or Extension is
  // being removed, the  map makes it possible to revoke the notifications from
  // MessageCenter.   To keep that set, we use the private ProfileNotification
  // class that stores  a superset of all information about a notification.

  // TODO(dimich): Consider merging all 4 types (Notification,
  // QueuedNotification, ProfileNotification and NotificationList::Notification)
  // into a single class.
  class ProfileNotification {
   public:
    ProfileNotification(Profile* profile,
                        const Notification& notification,
                        message_center::MessageCenter* message_center);
    virtual ~ProfileNotification();

    Profile* profile() const { return profile_; }
    const Notification& notification() const { return notification_; }

    // Route a new notification to an app/extension.
    void AddToAlternateProvider(const std::string extension_id);

   private:
    // Weak, guaranteed not to be used after profile removal by parent class.
    Profile* profile_;
    Notification notification_;
  };

  scoped_ptr<message_center::MessageCenterTrayDelegate> tray_;
  message_center::MessageCenter* message_center_;  // Weak, global.

  // Use a map by notification_id since this mapping is the most often used.
  typedef std::map<std::string, ProfileNotification*> NotificationMap;
  NotificationMap profile_notifications_;

  // Helpers that add/remove the notification from local map.
  // The local map takes ownership of profile_notification object.
  void AddProfileNotification(ProfileNotification* profile_notification);
  void RemoveProfileNotification(ProfileNotification* profile_notification);

  // Returns the ProfileNotification for the |id|, or NULL if no such
  // notification is found.
  ProfileNotification* FindProfileNotification(const std::string& id) const;

  // Get the extension ID of the extension that the user chose to take over
  // Chorme Notification Center.
  std::string GetExtensionTakingOverNotifications(Profile* profile);

#if defined(OS_WIN)
  // This function is run on update to ensure that the notification balloon is
  // shown only when there are no popups present.
  void CheckFirstRunTimer();

  // |first_run_pref_| is used to keep track of whether we've ever shown the
  // first run balloon before, even across restarts.
  BooleanPrefMember first_run_pref_;

  // The timer after which we will show the first run balloon.  This timer is
  // restarted every time the message center is closed and every time the last
  // popup disappears from the screen.
  base::OneShotTimer<MessageCenterNotificationManager> first_run_balloon_timer_;

  // The first-run balloon will be shown |first_run_idle_timeout_| after all
  // popups go away and the user has notifications in the message center.
  base::TimeDelta first_run_idle_timeout_;

  // Provides weak pointers for the purpose of the first run timer.
  base::WeakPtrFactory<MessageCenterNotificationManager> weak_factory_;
#endif

  scoped_ptr<message_center::NotifierSettingsProvider> settings_provider_;

  // To own the blockers.
  ScopedVector<message_center::NotificationBlocker> blockers_;

  NotificationSystemObserver system_observer_;

  // Keeps track of all notification statistics for UMA purposes.
  MessageCenterStatsCollector stats_collector_;

  // Keeps track of notifications specific to Google Now for UMA purposes.
  GoogleNowNotificationStatsCollector google_now_stats_collector_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenterNotificationManager);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_MESSAGE_CENTER_NOTIFICATION_MANAGER_H_
