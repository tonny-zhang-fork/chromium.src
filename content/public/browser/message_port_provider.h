// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_MESSAGE_PORT_PROVIDER_H_
#define CONTENT_PUBLIC_BROWSER_MESSAGE_PORT_PROVIDER_H_

#include <vector>

#include "base/basictypes.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"

namespace content {

class WebContents;

// An interface consisting of methods that can be called to use Message ports.
class CONTENT_EXPORT MessagePortProvider {
 public:
  // Posts a MessageEvent to the main frame using the given source and target
  // origins and data. The caller may also provide any message port ids as
  // part of the message.
  // See https://html.spec.whatwg.org/multipage/comms.html#messageevent for
  // further information on message events.
  // Should be called on IO thread.
  static void PostMessageToFrame(WebContents* web_contents,
                                 const base::string16& source_origin,
                                 const base::string16& target_origin,
                                 const base::string16& data,
                                 const std::vector<int>& ports);

  // Creates a message channel and provide the ids of the message ports that are
  // associated with this message channel.
  // See https://html.spec.whatwg.org/multipage/comms.html#messagechannel
  // Should be called on IO thread.
  static void CreateMessageChannel(WebContents* web_contents,
                                   int* port1,
                                   int* port2);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MessagePortProvider);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_MESSAGE_PORT_PROVIDER_H_
