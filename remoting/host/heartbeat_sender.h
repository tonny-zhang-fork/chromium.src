// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_HEARTBEAT_SENDER_H_
#define REMOTING_HOST_HEARTBEAT_SENDER_H_

#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "remoting/base/rsa_key_pair.h"
#include "remoting/signaling/signal_strategy.h"

namespace base {
class MessageLoopProxy;
}  // namespace base

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

class RsaKeyPair;
class IqRequest;
class IqSender;

// HeartbeatSender periodically sends heartbeat stanzas to the Chromoting Bot.
// Each heartbeat stanza looks as follows:
//
// <iq type="set" to="remoting@bot.talk.google.com"
//     from="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//   <rem:heartbeat rem:hostid="a1ddb11e-8aef-11df-bccf-18a905b9cb5a"
//                  rem:sequence-id="456"
//                  xmlns:rem="google:remoting">
//     <rem:signature>.signature.</rem:signature>
//   </rem:heartbeat>
// </iq>
//
// Normally the heartbeat indicates that the host is healthy and ready to
// accept new connections from a client, but the rem:heartbeat xml element can
// optionally include a rem:host-offline-reason attribute, which indicates that
// the host cannot accept connections from the client (and might possibly be
// shutting down).  The value of the host-offline-reason attribute can be a
// string from host_exit_codes.cc (i.e. "INVALID_HOST_CONFIGURATION" string).
//
// The sequence-id attribute of the heartbeat is a zero-based incrementally
// increasing integer unique to each heartbeat from a single host.
// The Bot checks the value, and if it is incorrect, includes the
// correct value in the result stanza. The host should then send another
// heartbeat, with the correct sequence-id, and increment the sequence-id in
// susbequent heartbeats.
// The signature is a base-64 encoded SHA-1 hash, signed with the host's
// private RSA key. The message being signed is the full Jid concatenated with
// the sequence-id, separated by one space. For example, for the heartbeat
// stanza above, the message that is signed is
// "user@gmail.com/chromoting123123 456".
//
// The Bot sends the following result stanza in response to each successful
// heartbeat:
//
//  <iq type="set" from="remoting@bot.talk.google.com"
//      to="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//    <rem:heartbeat-result xmlns:rem="google:remoting">
//      <rem:set-interval>300</rem:set-interval>
//    </rem:heartbeat-result>
//  </iq>
//
// The set-interval tag is used to specify desired heartbeat interval
// in seconds. The heartbeat-result and the set-interval tags are
// optional. Host uses default heartbeat interval if it doesn't find
// set-interval tag in the result Iq stanza it receives from the
// server.
// If the heartbeat's sequence-id was incorrect, the Bot sends a result
// stanza of this form:
//
//  <iq type="set" from="remoting@bot.talk.google.com"
//      to="user@gmail.com/chromoting123123" id="5" xmlns="jabber:client">
//    <rem:heartbeat-result xmlns:rem="google:remoting">
//      <rem:expected-sequence-id>654</rem:expected-sequence-id>
//    </rem:heartbeat-result>
//  </iq>
class HeartbeatSender : public SignalStrategy::Listener {
 public:
  // |signal_strategy| and |delegate| must outlive this
  // object. Heartbeats will start when the supplied SignalStrategy
  // enters the CONNECTED state.
  HeartbeatSender(const base::Closure& on_heartbeat_successful_callback,
                  const base::Closure& on_unknown_host_id_error,
                  const std::string& host_id,
                  SignalStrategy* signal_strategy,
                  scoped_refptr<RsaKeyPair> key_pair,
                  const std::string& directory_bot_jid);
  ~HeartbeatSender() override;

  // Sets host offline reason for future heartbeat stanzas,
  // as well as intiates sending a stanza right away.
  //
  // See rem:host-offline-reason class-level comments for discussion
  // of allowed values for |host_offline_reason| string.
  //
  // |ack_callback| will be called once, when the bot acks
  // receiving the |host_offline_reason|.
  void SetHostOfflineReason(
      const std::string& host_offline_reason,
      const base::Closure& ack_callback);

  // SignalStrategy::Listener interface.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(HeartbeatSenderTest,
                           DoSendStanzaWithExpectedSequenceId);
  FRIEND_TEST_ALL_PREFIXES(HeartbeatSenderTest, ProcessResponseSetInterval);
  FRIEND_TEST_ALL_PREFIXES(HeartbeatSenderTest,
                           ProcessResponseExpectedSequenceId);
  friend class HeartbeatSenderTest;

  void SendStanza();
  void ResendStanza();
  void DoSendStanza();
  void ProcessResponse(bool is_offline_heartbeat_response,
                       IqRequest* request,
                       const buzz::XmlElement* response);
  void SetInterval(int interval);
  void SetSequenceId(int sequence_id);

  // Helper methods used by DoSendStanza() to generate heartbeat stanzas.
  scoped_ptr<buzz::XmlElement> CreateHeartbeatMessage();
  scoped_ptr<buzz::XmlElement> CreateSignature();

  base::Closure on_heartbeat_successful_callback_;
  base::Closure on_unknown_host_id_error_;
  std::string host_id_;
  SignalStrategy* signal_strategy_;
  scoped_refptr<RsaKeyPair> key_pair_;
  std::string directory_bot_jid_;
  scoped_ptr<IqSender> iq_sender_;
  scoped_ptr<IqRequest> request_;
  int interval_ms_;
  base::RepeatingTimer<HeartbeatSender> timer_;
  base::OneShotTimer<HeartbeatSender> timer_resend_;
  int sequence_id_;
  bool sequence_id_was_set_;
  int sequence_id_recent_set_num_;
  bool heartbeat_succeeded_;
  int failed_startup_heartbeat_count_;
  std::string host_offline_reason_;
  base::Closure host_offline_reason_ack_callback_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(HeartbeatSender);
};

}  // namespace remoting

#endif  // REMOTING_HOST_HEARTBEAT_SENDER_H_
