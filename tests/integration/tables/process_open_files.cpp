/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under both the Apache 2.0 license (found in the
 *  LICENSE file in the root directory of this source tree) and the GPLv2 (found
 *  in the COPYING file in the root directory of this source tree).
 *  You may select, at your option, one of the above-listed licenses.
 */

// Sanity check integration test for process_open_files
// Spec file: specs/posix/process_open_files.table

#include <osquery/tests/integration/tables/helper.h>

#include <osquery/utils/info/platform_type.h>

#include <boost/filesystem.hpp>

#include <unistd.h>

namespace osquery {
namespace table_tests {

class ProcessOpenFilesTest : public testing::Test {
 public:
  boost::filesystem::path filepath;

 private:
  std::ofstream opened_file_;

  void SetUp() override {
    setUpEnvironment();
    filepath =
        boost::filesystem::temp_directory_path() /
        boost::filesystem::unique_path("test-process-open-files.%%%%-%%%%.txt");
    opened_file_.open(filepath.native(), std::ios::out);
    opened_file_ << "test";
  }

  void TearDown() override {
    opened_file_.close();
    boost::filesystem::remove(filepath);
  }
};

namespace {

bool checkProcessOpenFilePath(std::string const& value){
    // Some processes could have opened file with unlinked pathname
    if (value.find("(deleted)") != std::string::npos) {
      return true;
    }
    auto const path = boost::filesystem::path(value);
    // On macosx unlinked pathnames is not marked
    if (isPlatform(PlatformType::TYPE_OSX)) {
      return !path.empty() && path.is_absolute();
    }
    auto const status = boost::filesystem::status(path);
    return boost::filesystem::exists(status);
}

}

TEST_F(ProcessOpenFilesTest, test_sanity) {
  QueryData data = execute_query("select * from process_open_files");
  ASSERT_GT(data.size(), 0ul);
  ValidatatioMap row_map = {
      {"pid", NonNegativeInt},
      {"fd", NonNegativeInt},
      {"path", checkProcessOpenFilePath},
  };
  validate_rows(data, row_map);

  std::string self_pid = std::to_string(getpid());
  auto const test_filepath = boost::filesystem::canonical(filepath).string();
  bool found_self_file_open = false;
  for (const auto& row : data) {
    if (row.at("pid") == self_pid && row.at("path") == test_filepath) {
      found_self_file_open = true;
      break;
    }
  }
  ASSERT_TRUE(found_self_file_open)
      << "process_open_files tables could not find opened file by test";
}

} // namespace table_tests
} // namespace osquery
