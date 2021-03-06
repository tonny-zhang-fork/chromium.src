// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

class FileSelectHelperTest : public testing::Test {
 public:
  FileSelectHelperTest() {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &data_dir_));
    data_dir_ = data_dir_.AppendASCII("file_select_helper");
    ASSERT_TRUE(base::PathExists(data_dir_));
  }

  // The path to input data used in tests.
  base::FilePath data_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSelectHelperTest);
};

TEST_F(FileSelectHelperTest, IsAcceptTypeValid) {
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid("a/b"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid("abc/def"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid("abc/*"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid(".a"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid(".abc"));

  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("."));
  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("/"));
  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("ABC/*"));
  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("abc/def "));
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
TEST_F(FileSelectHelperTest, ZipPackage) {
  // Zip the package.
  const char app_name[] = "CalculatorFake.app";
  base::FilePath src = data_dir_.Append(app_name);
  base::FilePath dest = FileSelectHelper::ZipPackage(src);
  ASSERT_FALSE(dest.empty());
  ASSERT_TRUE(base::PathExists(dest));

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Unzip the package into a temporary directory.
  base::CommandLine cl(base::FilePath("/usr/bin/unzip"));
  cl.AppendArg(dest.value().c_str());
  cl.AppendArg("-d");
  cl.AppendArg(temp_dir.path().value().c_str());
  std::string output;
  EXPECT_TRUE(base::GetAppOutput(cl, &output));

  // Verify that several key files haven't changed.
  const char* files_to_verify[] = {"Contents/Info.plist",
                                   "Contents/MacOS/Calculator",
                                   "Contents/_CodeSignature/CodeResources"};
  size_t file_count = arraysize(files_to_verify);
  for (size_t i = 0; i < file_count; i++) {
    const char* relative_path = files_to_verify[i];
    base::FilePath orig_file = src.Append(relative_path);
    base::FilePath final_file =
        temp_dir.path().Append(app_name).Append(relative_path);
    EXPECT_TRUE(base::ContentsEqual(orig_file, final_file));
  }
}
#endif  // defined(OS_MACOSX) && !defined(OS_IOS)
