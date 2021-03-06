// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////

#include "tink/mac/mac_config.h"

#include "gtest/gtest.h"
#include "tink/config.h"
#include "tink/keyset_handle.h"
#include "tink/mac.h"
#include "tink/mac/hmac_key_manager.h"
#include "tink/mac/mac_key_templates.h"
#include "tink/registry.h"
#include "tink/util/status.h"
#include "tink/util/test_matchers.h"
#include "tink/util/test_util.h"

namespace crypto {
namespace tink {
namespace {

using ::crypto::tink::test::DummyMac;
using ::crypto::tink::test::IsOk;
using ::crypto::tink::test::StatusIs;

class MacConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Registry::Reset();
  }
};

TEST_F(MacConfigTest, Basic) {
  EXPECT_THAT(
      Registry::get_key_manager<Mac>(HmacKeyManager().get_key_type()).status(),
      StatusIs(util::error::NOT_FOUND));
  ASSERT_THAT(MacConfig::Register(), IsOk());
  EXPECT_THAT(
      Registry::get_key_manager<Mac>(HmacKeyManager().get_key_type()).status(),
      IsOk());
}

// Tests that the MacWrapper has been properly registered and we can wrap
// primitives.
TEST_F(MacConfigTest, WrappersRegistered) {
  ASSERT_TRUE(MacConfig::Register().ok());

  google::crypto::tink::Keyset::Key key;
  key.set_status(google::crypto::tink::KeyStatusType::ENABLED);
  key.set_key_id(1234);
  key.set_output_prefix_type(google::crypto::tink::OutputPrefixType::RAW);
  auto primitive_set = absl::make_unique<PrimitiveSet<Mac>>();
  ASSERT_TRUE(
      primitive_set
          ->set_primary(
              primitive_set
                  ->AddPrimitive(absl::make_unique<DummyMac>("dummy"), key)
                  .ValueOrDie())
          .ok());

  auto primitive_result = Registry::Wrap(std::move(primitive_set));

  ASSERT_TRUE(primitive_result.ok()) << primitive_result.status();
  auto mac_result =
      primitive_result.ValueOrDie()->ComputeMac("verified text");
  ASSERT_TRUE(mac_result.ok());

  EXPECT_TRUE(DummyMac("dummy")
                  .VerifyMac(mac_result.ValueOrDie(), "verified text")
                  .ok());
  EXPECT_FALSE(
      DummyMac("dummy").VerifyMac(mac_result.ValueOrDie(), "faked text").ok());
}

}  // namespace
}  // namespace tink
}  // namespace crypto
