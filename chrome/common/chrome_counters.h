// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Counters used within the browser.

#ifndef CHROME_COMMON_CHROME_COUNTERS_H__
#define CHROME_COMMON_CHROME_COUNTERS_H__

#include "base/stats_counters.h"

namespace chrome {

class Counters {
 public:
  // The number of messages sent on IPC channels.
  static StatsCounter& ipc_send_counter();

  // The amount of time spent in chrome initialization.
  static StatsCounterTimer& chrome_main();

  // The amount of time spent in renderer initialization.
  static StatsCounterTimer& renderer_main();

  // Time spent in spellchecker initialization.
  static StatsCounterTimer& spellcheck_init();

  // Time/Count of spellcheck lookups.
  static StatsRate& spellcheck_lookup();

  // Time spent loading the Chrome plugins.
  static StatsCounterTimer& plugin_load();

  // Time/Count of plugin network interception.
  static StatsRate& plugin_intercept();

};

}  // namespace chrome

#endif  // CHROME_COMMON_CHROME_COUNTERS_H__
