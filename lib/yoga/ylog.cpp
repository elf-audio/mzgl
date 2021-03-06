/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ylog.h"

#include "Yoga.h"
#include "YGConfig.h"
#include "YGNode.h"

namespace facebook {
namespace yoga {
namespace detail {

namespace {

void vlog(
    YGConfig* config,
    YGNode* node,
    YGLogLevel level,
    void* context,
    const char* format,
    va_list args) {
  YGConfig* logConfig = config != nullptr ? config : YGConfigGetDefault();
  logConfig->log(logConfig, node, level, context, format, args);
}
} // namespace

YOGA_EXPORT void YLog::log(
    YGNode* node,
    YGLogLevel level,
    void* context,
    const char* format,
    ...) noexcept {
  va_list args;
  va_start(args, format);
  vlog(
      node == nullptr ? nullptr : node->getConfig(),
      node,
      level,
      context,
      format,
      args);
  va_end(args);
}

void YLog::log(
    YGConfig* config,
    YGLogLevel level,
    void* context,
    const char* format,
    ...) noexcept {
  va_list args;
  va_start(args, format);
  vlog(config, nullptr, level, context, format, args);
  va_end(args);
}

} // namespace detail
} // namespace yoga
} // namespace facebook
