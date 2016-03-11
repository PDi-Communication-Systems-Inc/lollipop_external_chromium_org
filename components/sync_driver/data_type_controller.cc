// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_driver/data_type_controller.h"

#include "sync/internal_api/public/base/model_type.h"
#include "sync/internal_api/public/user_share.h"
#include "sync/util/data_type_histogram.h"

namespace browser_sync {

DataTypeController::DataTypeController(
    scoped_refptr<base::MessageLoopProxy> ui_thread,
    const base::Closure& error_callback,
    const DisableTypeCallback& disable_callback)
    : base::RefCountedDeleteOnMessageLoop<DataTypeController>(ui_thread),
      error_callback_(error_callback),
      disable_callback_(disable_callback),
      user_share_(NULL) {
}

DataTypeController::~DataTypeController() {
}

bool DataTypeController::IsUnrecoverableResult(StartResult result) {
  return (result == UNRECOVERABLE_ERROR);
}

bool DataTypeController::IsSuccessfulResult(StartResult result) {
  return (result == OK || result == OK_FIRST_RUN);
}

syncer::SyncError DataTypeController::CreateAndUploadError(
    const tracked_objects::Location& location,
    const std::string& message,
    syncer::ModelType type) {
  if (!error_callback_.is_null())
    error_callback_.Run();
  return syncer::SyncError(location,
                           syncer::SyncError::DATATYPE_ERROR,
                           message,
                           type);
}

void DataTypeController::OnUserShareReady(syncer::UserShare* share) {
  user_share_ = share;
}

syncer::UserShare* DataTypeController::user_share() const {
  return user_share_;
}

DataTypeController::DisableTypeCallback
DataTypeController::disable_callback() {
  return disable_callback_;
}

bool DataTypeController::ReadyForStart() const {
  return true;
}

}  // namespace browser_sync