// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/public/provider/chrome/browser/browser_state/chrome_browser_state.h"

namespace ios {

// static
ChromeBrowserState* ChromeBrowserState::FromBrowserState(
    web::BrowserState* browser_state) {
  // This is safe; this is the only implementation of BrowserState.
  return static_cast<ChromeBrowserState*>(browser_state);
}

}  // namespace ios
