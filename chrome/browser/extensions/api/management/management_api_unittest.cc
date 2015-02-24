// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/test/base/test_browser_window.h"
#include "extensions/browser/api/management/management_api.h"
#include "extensions/browser/api/management/management_api_constants.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/test_management_policy.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/test_util.h"

namespace extensions {

namespace {

KeyedService* BuildManagementApi(content::BrowserContext* context) {
  return new ManagementAPI(context);
}

}  // namespace

namespace constants = extension_management_api_constants;

// TODO(devlin): Unittests are awesome. Test more with unittests and less with
// heavy api/browser tests.
class ManagementApiUnitTest : public ExtensionServiceTestBase {
 protected:
  ManagementApiUnitTest() {}
  ~ManagementApiUnitTest() override {}

  // A wrapper around extension_function_test_utils::RunFunction that runs with
  // the associated browser, no flags, and can take stack-allocated arguments.
  bool RunFunction(const scoped_refptr<UIThreadExtensionFunction>& function,
                   const base::ListValue& args);

  Browser* browser() { return browser_.get(); }

 private:
  // ExtensionServiceTestBase:
  void SetUp() override;
  void TearDown() override;

  // The browser (and accompanying window).
  scoped_ptr<TestBrowserWindow> browser_window_;
  scoped_ptr<Browser> browser_;

  DISALLOW_COPY_AND_ASSIGN(ManagementApiUnitTest);
};

bool ManagementApiUnitTest::RunFunction(
    const scoped_refptr<UIThreadExtensionFunction>& function,
    const base::ListValue& args) {
  return extension_function_test_utils::RunFunction(
      function.get(),
      make_scoped_ptr(args.DeepCopy()),
      browser(),
      extension_function_test_utils::NONE);
}

void ManagementApiUnitTest::SetUp() {
  ExtensionServiceTestBase::SetUp();
  InitializeEmptyExtensionService();
  ManagementAPI::GetFactoryInstance()->SetTestingFactory(profile(),
                                                         &BuildManagementApi);
  static_cast<TestExtensionSystem*>(ExtensionSystem::Get(profile()))->
      SetEventRouter(make_scoped_ptr(
          new EventRouter(profile(), ExtensionPrefs::Get(profile()))));

  browser_window_.reset(new TestBrowserWindow());
  Browser::CreateParams params(profile(), chrome::HOST_DESKTOP_TYPE_NATIVE);
  params.type = Browser::TYPE_TABBED;
  params.window = browser_window_.get();
  browser_.reset(new Browser(params));
}

void ManagementApiUnitTest::TearDown() {
  browser_.reset();
  browser_window_.reset();
  ExtensionServiceTestBase::TearDown();
}

// Test the basic parts of management.setEnabled.
TEST_F(ManagementApiUnitTest, ManagementSetEnabled) {
  scoped_refptr<const Extension> extension = test_util::CreateEmptyExtension();
  service()->AddExtension(extension.get());
  std::string extension_id = extension->id();
  scoped_refptr<ManagementSetEnabledFunction> function(
      new ManagementSetEnabledFunction());

  base::ListValue disable_args;
  disable_args.AppendString(extension_id);
  disable_args.AppendBoolean(false);

  // Test disabling an (enabled) extension.
  EXPECT_TRUE(registry()->enabled_extensions().Contains(extension_id));
  EXPECT_TRUE(RunFunction(function, disable_args));
  EXPECT_TRUE(registry()->disabled_extensions().Contains(extension_id));

  base::ListValue enable_args;
  enable_args.AppendString(extension_id);
  enable_args.AppendBoolean(true);

  // Test re-enabling it.
  function = new ManagementSetEnabledFunction();
  EXPECT_TRUE(RunFunction(function, enable_args));
  EXPECT_TRUE(registry()->enabled_extensions().Contains(extension_id));

  // Test that the enable function checks management policy, so that we can't
  // disable an extension that is required.
  TestManagementPolicyProvider provider(
      TestManagementPolicyProvider::PROHIBIT_MODIFY_STATUS);
  ManagementPolicy* policy =
      ExtensionSystem::Get(profile())->management_policy();
  policy->RegisterProvider(&provider);

  function = new ManagementSetEnabledFunction();
  EXPECT_FALSE(RunFunction(function, disable_args));
  EXPECT_EQ(ErrorUtils::FormatErrorMessage(constants::kUserCantModifyError,
                                           extension_id),
            function->GetError());
  policy->UnregisterProvider(&provider);

  // TODO(devlin): We should also test enabling an extenion that has escalated
  // permissions, but that needs a web contents (which is a bit of a pain in a
  // unit test).
}

}  // namespace extensions
