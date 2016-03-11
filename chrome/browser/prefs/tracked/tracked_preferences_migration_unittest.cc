// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/tracked/tracked_preferences_migration.h"

#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/prefs/testing_pref_service.h"
#include "base/values.h"
#include "chrome/browser/prefs/interceptable_pref_filter.h"
#include "chrome/browser/prefs/pref_hash_store.h"
#include "chrome/browser/prefs/pref_hash_store_impl.h"
#include "chrome/browser/prefs/pref_hash_store_transaction.h"
#include "chrome/browser/prefs/profile_pref_store_manager.h"
#include "chrome/browser/prefs/tracked/dictionary_hash_store_contents.h"
#include "chrome/browser/prefs/tracked/hash_store_contents.h"
#include "chrome/browser/prefs/tracked/pref_service_hash_store_contents.h"
#include "chrome/browser/prefs/tracked/tracked_preferences_migration.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// An unprotected pref.
const char kUnprotectedPref[] = "unprotected";
// A protected pref.
const char kProtectedPref[] = "protected";
// A protected pref which is initially stored in the unprotected store.
const char kPreviouslyUnprotectedPref[] = "previously.unprotected";
// An unprotected pref which is initially stored in the protected store.
const char kPreviouslyProtectedPref[] = "previously.protected";

const char kUnprotectedPrefValue[] = "unprotected_value";
const char kProtectedPrefValue[] = "protected_value";
const char kPreviouslyUnprotectedPrefValue[] = "previously_unprotected_value";
const char kPreviouslyProtectedPrefValue[] = "previously_protected_value";

// A simple InterceptablePrefFilter which doesn't do anything but hand the prefs
// back downstream in FinalizeFilterOnLoad.
class SimpleInterceptablePrefFilter : public InterceptablePrefFilter {
 public:
  // PrefFilter remaining implementation.
  virtual void FilterUpdate(const std::string& path) OVERRIDE {
    ADD_FAILURE();
  }
  virtual void FilterSerializeData(
      base::DictionaryValue* pref_store_contents) OVERRIDE {
    ADD_FAILURE();
  }

 private:
  // InterceptablePrefFilter implementation.
  virtual void FinalizeFilterOnLoad(
      const PostFilterOnLoadCallback& post_filter_on_load_callback,
      scoped_ptr<base::DictionaryValue> pref_store_contents,
      bool prefs_altered) OVERRIDE {
    post_filter_on_load_callback.Run(pref_store_contents.Pass(), prefs_altered);
  }
};

// A test fixture designed to be used like this:
//  1) Set up initial store prefs with PresetStoreValue().
//  2) Hand both sets of prefs to the migrator via HandPrefsToMigrator().
//  3) Migration completes synchronously when the second store hands its prefs
//     over.
//  4) Verifications can be made via various methods of this fixture.
// Call Reset() to perform a second migration.
class TrackedPreferencesMigrationTest : public testing::Test {
 public:
  enum MockPrefStoreID {
    MOCK_UNPROTECTED_PREF_STORE,
    MOCK_PROTECTED_PREF_STORE,
  };

  TrackedPreferencesMigrationTest()
      : unprotected_prefs_(new base::DictionaryValue),
        protected_prefs_(new base::DictionaryValue),
        migration_modified_unprotected_store_(false),
        migration_modified_protected_store_(false),
        unprotected_store_migration_complete_(false),
        protected_store_migration_complete_(false) {}

  virtual void SetUp() OVERRIDE {
    ProfilePrefStoreManager::RegisterPrefs(local_state_.registry());
    Reset();
  }

  void Reset() {
    std::set<std::string> unprotected_pref_names;
    std::set<std::string> protected_pref_names;
    unprotected_pref_names.insert(kUnprotectedPref);
    unprotected_pref_names.insert(kPreviouslyProtectedPref);
    protected_pref_names.insert(kProtectedPref);
    protected_pref_names.insert(kPreviouslyUnprotectedPref);

    migration_modified_unprotected_store_ = false;
    migration_modified_protected_store_ = false;
    unprotected_store_migration_complete_ = false;
    protected_store_migration_complete_ = false;

    unprotected_store_successful_write_callback_.Reset();
    protected_store_successful_write_callback_.Reset();

    SetupTrackedPreferencesMigration(
        unprotected_pref_names,
        protected_pref_names,
        base::Bind(&TrackedPreferencesMigrationTest::RemovePathFromStore,
                   base::Unretained(this),
                   MOCK_UNPROTECTED_PREF_STORE),
        base::Bind(&TrackedPreferencesMigrationTest::RemovePathFromStore,
                   base::Unretained(this),
                   MOCK_PROTECTED_PREF_STORE),
        base::Bind(
            &TrackedPreferencesMigrationTest::RegisterSuccessfulWriteClosure,
            base::Unretained(this),
            MOCK_UNPROTECTED_PREF_STORE),
        base::Bind(
            &TrackedPreferencesMigrationTest::RegisterSuccessfulWriteClosure,
            base::Unretained(this),
            MOCK_PROTECTED_PREF_STORE),
        scoped_ptr<PrefHashStore>(
            new PrefHashStoreImpl(kSeed, kDeviceId, false)),
        scoped_ptr<PrefHashStore>(
            new PrefHashStoreImpl(kSeed, kDeviceId, true)),
        scoped_ptr<HashStoreContents>(
            new PrefServiceHashStoreContents("hash-store-id", &local_state_)),

        &mock_unprotected_pref_filter_,
        &mock_protected_pref_filter_);

    // Verify initial expectations are met.
    EXPECT_TRUE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
    EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
    EXPECT_FALSE(
        WasOnSuccessfulWriteCallbackRegistered(MOCK_UNPROTECTED_PREF_STORE));
    EXPECT_FALSE(
        WasOnSuccessfulWriteCallbackRegistered(MOCK_PROTECTED_PREF_STORE));
  }

 protected:
  // Sets |key| to |value| in the test store identified by |store_id| before
  // migration begins.
  void PresetStoreValue(MockPrefStoreID store_id,
                        const std::string& key,
                        const std::string value) {
    base::DictionaryValue* store = NULL;
    scoped_ptr<PrefHashStore> pref_hash_store;
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        store = unprotected_prefs_.get();
        pref_hash_store.reset(new PrefHashStoreImpl(kSeed, kDeviceId, false));
        break;
      case MOCK_PROTECTED_PREF_STORE:
        store = protected_prefs_.get();
        pref_hash_store.reset(new PrefHashStoreImpl(kSeed, kDeviceId, true));
        break;
    }
    DCHECK(store);

    store->SetString(key, value);
    base::StringValue string_value(value);
    pref_hash_store->BeginTransaction(
        scoped_ptr<HashStoreContents>(
            new DictionaryHashStoreContents(store)))->StoreHash(
                key, &string_value);
  }

  // Returns true if the store opposite to |store_id| is observed for its next
  // successful write.
  bool WasOnSuccessfulWriteCallbackRegistered(MockPrefStoreID store_id) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        return !protected_store_successful_write_callback_.is_null();
      case MOCK_PROTECTED_PREF_STORE:
        return !unprotected_store_successful_write_callback_.is_null();
    }
    NOTREACHED();
    return false;
  }

  // Verifies that the (key, value) pairs in |expected_prefs_in_store| are found
  // in the store identified by |store_id|.
  void VerifyValuesStored(
      MockPrefStoreID store_id,
      const std::vector<std::pair<std::string, std::string> >&
          expected_prefs_in_store) {
    base::DictionaryValue* store = NULL;
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        store = unprotected_prefs_.get();
        break;
      case MOCK_PROTECTED_PREF_STORE:
        store = protected_prefs_.get();
        break;
    }
    DCHECK(store);

    for (std::vector<std::pair<std::string, std::string> >::const_iterator it =
             expected_prefs_in_store.begin();
         it != expected_prefs_in_store.end(); ++it) {
      std::string val;
      EXPECT_TRUE(store->GetString(it->first, &val));
      EXPECT_EQ(it->second, val);
    }
  }

  // Determines whether |expected_pref_in_hash_store| has a hash in the hash
  // store identified by |store_id|.
  bool ContainsHash(MockPrefStoreID store_id,
                    std::string expected_pref_in_hash_store) {
    base::DictionaryValue* store = NULL;
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        store = unprotected_prefs_.get();
        break;
      case MOCK_PROTECTED_PREF_STORE:
        store = protected_prefs_.get();
        break;
    }
    DCHECK(store);
    return DictionaryHashStoreContents(store).GetContents()->GetString(
        expected_pref_in_hash_store, static_cast<std::string*>(NULL));
  }

  // Both stores need to hand their prefs over in order for migration to kick
  // in.
  void HandPrefsToMigrator(MockPrefStoreID store_id) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        mock_unprotected_pref_filter_.FilterOnLoad(
            base::Bind(&TrackedPreferencesMigrationTest::GetPrefsBack,
                       base::Unretained(this),
                       MOCK_UNPROTECTED_PREF_STORE),
            unprotected_prefs_.Pass());
        break;
      case MOCK_PROTECTED_PREF_STORE:
        mock_protected_pref_filter_.FilterOnLoad(
            base::Bind(&TrackedPreferencesMigrationTest::GetPrefsBack,
                       base::Unretained(this),
                       MOCK_PROTECTED_PREF_STORE),
            protected_prefs_.Pass());
        break;
    }
  }

  bool HasPrefs(MockPrefStoreID store_id) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        return unprotected_prefs_;
      case MOCK_PROTECTED_PREF_STORE:
        return protected_prefs_;
    }
    NOTREACHED();
    return false;
  }

  bool StoreModifiedByMigration(MockPrefStoreID store_id) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        return migration_modified_unprotected_store_;
      case MOCK_PROTECTED_PREF_STORE:
        return migration_modified_protected_store_;
    }
    NOTREACHED();
    return false;
  }

  bool MigrationCompleted() {
    return unprotected_store_migration_complete_ &&
        protected_store_migration_complete_;
  }

  void SimulateSuccessfulWrite(MockPrefStoreID store_id) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        EXPECT_FALSE(unprotected_store_successful_write_callback_.is_null());
        unprotected_store_successful_write_callback_.Run();
        unprotected_store_successful_write_callback_.Reset();
        break;
      case MOCK_PROTECTED_PREF_STORE:
        EXPECT_FALSE(protected_store_successful_write_callback_.is_null());
        protected_store_successful_write_callback_.Run();
        protected_store_successful_write_callback_.Reset();
        break;
    }
  }

 private:
  void RegisterSuccessfulWriteClosure(
      MockPrefStoreID store_id,
      const base::Closure& successful_write_closure) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        EXPECT_TRUE(unprotected_store_successful_write_callback_.is_null());
        unprotected_store_successful_write_callback_ = successful_write_closure;
        break;
      case MOCK_PROTECTED_PREF_STORE:
        EXPECT_TRUE(protected_store_successful_write_callback_.is_null());
        protected_store_successful_write_callback_ = successful_write_closure;
        break;
    }
  }

  // Helper given as an InterceptablePrefFilter::FinalizeFilterOnLoadCallback
  // to the migrator to be invoked when it's done.
  void GetPrefsBack(MockPrefStoreID store_id,
                    scoped_ptr<base::DictionaryValue> prefs,
                    bool prefs_altered) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        EXPECT_FALSE(unprotected_prefs_);
        unprotected_prefs_ = prefs.Pass();
        migration_modified_unprotected_store_ = prefs_altered;
        unprotected_store_migration_complete_ = true;
        break;
      case MOCK_PROTECTED_PREF_STORE:
        EXPECT_FALSE(protected_prefs_);
        protected_prefs_ = prefs.Pass();
        migration_modified_protected_store_ = prefs_altered;
        protected_store_migration_complete_ = true;
        break;
    }
  }

  // Helper given as a cleaning callback to the migrator.
  void RemovePathFromStore(MockPrefStoreID store_id, const std::string& key) {
    switch (store_id) {
      case MOCK_UNPROTECTED_PREF_STORE:
        ASSERT_TRUE(unprotected_prefs_);
        unprotected_prefs_->RemovePath(key, NULL);
        break;
      case MOCK_PROTECTED_PREF_STORE:
        ASSERT_TRUE(protected_prefs_);
        protected_prefs_->RemovePath(key, NULL);
        break;
    }
  }

  static const char kSeed[];
  static const char kDeviceId[];

  scoped_ptr<base::DictionaryValue> unprotected_prefs_;
  scoped_ptr<base::DictionaryValue> protected_prefs_;

  SimpleInterceptablePrefFilter mock_unprotected_pref_filter_;
  SimpleInterceptablePrefFilter mock_protected_pref_filter_;

  base::Closure unprotected_store_successful_write_callback_;
  base::Closure protected_store_successful_write_callback_;

  bool migration_modified_unprotected_store_;
  bool migration_modified_protected_store_;

  bool unprotected_store_migration_complete_;
  bool protected_store_migration_complete_;

  TestingPrefServiceSimple local_state_;
};

// static
const char TrackedPreferencesMigrationTest::kSeed[] = "seed";

// static
const char TrackedPreferencesMigrationTest::kDeviceId[] = "device-id";

}  // namespace

TEST_F(TrackedPreferencesMigrationTest, NoMigrationRequired) {
  PresetStoreValue(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref,
                   kUnprotectedPrefValue);
  PresetStoreValue(MOCK_PROTECTED_PREF_STORE, kProtectedPref,
                   kProtectedPrefValue);

  EXPECT_TRUE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kProtectedPref));

  EXPECT_TRUE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kUnprotectedPref));

  // Hand unprotected prefs to the migrator which should wait for the protected
  // prefs.
  HandPrefsToMigrator(MOCK_UNPROTECTED_PREF_STORE);
  EXPECT_FALSE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(MigrationCompleted());

  // Hand protected prefs to the migrator which should proceed with the
  // migration synchronously.
  HandPrefsToMigrator(MOCK_PROTECTED_PREF_STORE);
  EXPECT_TRUE(MigrationCompleted());

  // Prefs should have been handed back over.
  EXPECT_TRUE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(
      WasOnSuccessfulWriteCallbackRegistered(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_FALSE(
      WasOnSuccessfulWriteCallbackRegistered(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(StoreModifiedByMigration(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_FALSE(StoreModifiedByMigration(MOCK_PROTECTED_PREF_STORE));

  std::vector<std::pair<std::string, std::string> > expected_unprotected_values;
  expected_unprotected_values.push_back(
      std::make_pair(kUnprotectedPref, kUnprotectedPrefValue));
  VerifyValuesStored(MOCK_UNPROTECTED_PREF_STORE, expected_unprotected_values);

  std::vector<std::pair<std::string, std::string> > expected_protected_values;
  expected_protected_values.push_back(
      std::make_pair(kProtectedPref, kProtectedPrefValue));
  VerifyValuesStored(MOCK_PROTECTED_PREF_STORE, expected_protected_values);

  EXPECT_TRUE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kProtectedPref));

  EXPECT_TRUE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kUnprotectedPref));
}

TEST_F(TrackedPreferencesMigrationTest, FullMigration) {
  PresetStoreValue(
      MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref, kUnprotectedPrefValue);
  PresetStoreValue(MOCK_UNPROTECTED_PREF_STORE,
                   kPreviouslyUnprotectedPref,
                   kPreviouslyUnprotectedPrefValue);
  PresetStoreValue(
      MOCK_PROTECTED_PREF_STORE, kProtectedPref, kProtectedPrefValue);
  PresetStoreValue(MOCK_PROTECTED_PREF_STORE,
                   kPreviouslyProtectedPref,
                   kPreviouslyProtectedPrefValue);

  EXPECT_TRUE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_FALSE(
      ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyProtectedPref));

  EXPECT_FALSE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_FALSE(
      ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
  EXPECT_TRUE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyProtectedPref));

  HandPrefsToMigrator(MOCK_UNPROTECTED_PREF_STORE);
  EXPECT_FALSE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(MigrationCompleted());

  HandPrefsToMigrator(MOCK_PROTECTED_PREF_STORE);
  EXPECT_TRUE(MigrationCompleted());

  EXPECT_TRUE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
  EXPECT_TRUE(
      WasOnSuccessfulWriteCallbackRegistered(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(
      WasOnSuccessfulWriteCallbackRegistered(MOCK_PROTECTED_PREF_STORE));
  EXPECT_TRUE(StoreModifiedByMigration(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(StoreModifiedByMigration(MOCK_PROTECTED_PREF_STORE));

  // Values should have been migrated to their store, but migrated values should
  // still remain in the source store until cleanup tasks are later invoked.
  {
    std::vector<std::pair<std::string, std::string> >
        expected_unprotected_values;
    expected_unprotected_values.push_back(std::make_pair(
        kUnprotectedPref, kUnprotectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyProtectedPref, kPreviouslyProtectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyUnprotectedPref, kPreviouslyUnprotectedPrefValue));
    VerifyValuesStored(MOCK_UNPROTECTED_PREF_STORE,
                       expected_unprotected_values);

    std::vector<std::pair<std::string, std::string> > expected_protected_values;
    expected_protected_values.push_back(std::make_pair(
        kProtectedPref, kProtectedPrefValue));
    expected_protected_values.push_back(std::make_pair(
        kPreviouslyUnprotectedPref, kPreviouslyUnprotectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyProtectedPref, kPreviouslyProtectedPrefValue));
    VerifyValuesStored(MOCK_PROTECTED_PREF_STORE, expected_protected_values);

    EXPECT_TRUE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref));
    EXPECT_TRUE(
        ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
    EXPECT_FALSE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kProtectedPref));
    EXPECT_TRUE(
        ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyProtectedPref));

    EXPECT_FALSE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kUnprotectedPref));
    EXPECT_TRUE(
        ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
    EXPECT_TRUE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kProtectedPref));
    EXPECT_TRUE(
        ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyProtectedPref));
  }

  // A successful write of the protected pref store should result in a clean up
  // of the unprotected store.
  SimulateSuccessfulWrite(MOCK_PROTECTED_PREF_STORE);

  {
    std::vector<std::pair<std::string, std::string> >
        expected_unprotected_values;
    expected_unprotected_values.push_back(std::make_pair(
        kUnprotectedPref, kUnprotectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyProtectedPref, kPreviouslyProtectedPrefValue));
    VerifyValuesStored(MOCK_UNPROTECTED_PREF_STORE,
                       expected_unprotected_values);

    std::vector<std::pair<std::string, std::string> > expected_protected_values;
    expected_protected_values.push_back(std::make_pair(
        kProtectedPref, kProtectedPrefValue));
    expected_protected_values.push_back(std::make_pair(
        kPreviouslyUnprotectedPref, kPreviouslyUnprotectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyProtectedPref, kPreviouslyProtectedPrefValue));
    VerifyValuesStored(MOCK_PROTECTED_PREF_STORE, expected_protected_values);
  }

  SimulateSuccessfulWrite(MOCK_UNPROTECTED_PREF_STORE);

  {
    std::vector<std::pair<std::string, std::string> >
        expected_unprotected_values;
    expected_unprotected_values.push_back(std::make_pair(
        kUnprotectedPref, kUnprotectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyProtectedPref, kPreviouslyProtectedPrefValue));
    VerifyValuesStored(MOCK_UNPROTECTED_PREF_STORE,
                       expected_unprotected_values);

    std::vector<std::pair<std::string, std::string> > expected_protected_values;
    expected_protected_values.push_back(std::make_pair(
        kProtectedPref, kProtectedPrefValue));
    expected_protected_values.push_back(std::make_pair(
        kPreviouslyUnprotectedPref, kPreviouslyUnprotectedPrefValue));
    VerifyValuesStored(MOCK_PROTECTED_PREF_STORE, expected_protected_values);
  }

  // Hashes are not cleaned up yet.
  EXPECT_TRUE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyProtectedPref));

  EXPECT_FALSE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
  EXPECT_TRUE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyProtectedPref));

  Reset();

  HandPrefsToMigrator(MOCK_UNPROTECTED_PREF_STORE);
  HandPrefsToMigrator(MOCK_PROTECTED_PREF_STORE);
  EXPECT_TRUE(MigrationCompleted());

  // Hashes are cleaned up.
  EXPECT_TRUE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_FALSE(
      ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
  EXPECT_FALSE(ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_UNPROTECTED_PREF_STORE, kPreviouslyProtectedPref));

  EXPECT_FALSE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kUnprotectedPref));
  EXPECT_TRUE(
      ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyUnprotectedPref));
  EXPECT_TRUE(ContainsHash(MOCK_PROTECTED_PREF_STORE, kProtectedPref));
  EXPECT_FALSE(
      ContainsHash(MOCK_PROTECTED_PREF_STORE, kPreviouslyProtectedPref));
}

TEST_F(TrackedPreferencesMigrationTest, CleanupOnly) {
  // Already migrated; only cleanup needed.
  PresetStoreValue(
      MOCK_UNPROTECTED_PREF_STORE, kUnprotectedPref, kUnprotectedPrefValue);
  PresetStoreValue(MOCK_UNPROTECTED_PREF_STORE,
                   kPreviouslyProtectedPref,
                   kPreviouslyProtectedPrefValue);
  PresetStoreValue(MOCK_UNPROTECTED_PREF_STORE,
                   kPreviouslyUnprotectedPref,
                   kPreviouslyUnprotectedPrefValue);
  PresetStoreValue(
      MOCK_PROTECTED_PREF_STORE, kProtectedPref, kProtectedPrefValue);
  PresetStoreValue(MOCK_PROTECTED_PREF_STORE,
                   kPreviouslyProtectedPref,
                   kPreviouslyProtectedPrefValue);
  PresetStoreValue(MOCK_PROTECTED_PREF_STORE,
                   kPreviouslyUnprotectedPref,
                   kPreviouslyUnprotectedPrefValue);

  HandPrefsToMigrator(MOCK_UNPROTECTED_PREF_STORE);
  EXPECT_FALSE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(MigrationCompleted());

  HandPrefsToMigrator(MOCK_PROTECTED_PREF_STORE);
  EXPECT_TRUE(MigrationCompleted());

  EXPECT_TRUE(HasPrefs(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_TRUE(HasPrefs(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(
      WasOnSuccessfulWriteCallbackRegistered(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_FALSE(
      WasOnSuccessfulWriteCallbackRegistered(MOCK_PROTECTED_PREF_STORE));
  EXPECT_FALSE(StoreModifiedByMigration(MOCK_UNPROTECTED_PREF_STORE));
  EXPECT_FALSE(StoreModifiedByMigration(MOCK_PROTECTED_PREF_STORE));

  // Cleanup should happen synchronously if the values were already present in
  // their destination stores.
  {
    std::vector<std::pair<std::string, std::string> >
        expected_unprotected_values;
    expected_unprotected_values.push_back(std::make_pair(
        kUnprotectedPref, kUnprotectedPrefValue));
    expected_unprotected_values.push_back(std::make_pair(
        kPreviouslyProtectedPref, kPreviouslyProtectedPrefValue));
    VerifyValuesStored(MOCK_UNPROTECTED_PREF_STORE,
                       expected_unprotected_values);

    std::vector<std::pair<std::string, std::string> > expected_protected_values;
    expected_protected_values.push_back(std::make_pair(
        kProtectedPref, kProtectedPrefValue));
    expected_protected_values.push_back(std::make_pair(
        kPreviouslyUnprotectedPref, kPreviouslyUnprotectedPrefValue));
    VerifyValuesStored(MOCK_PROTECTED_PREF_STORE, expected_protected_values);
  }
}