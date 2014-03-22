// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/engine/connection_factory_impl.h"

#include <cmath>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/simple_test_tick_clock.h"
#include "net/base/backoff_entry.h"
#include "net/http/http_network_session.h"
#include "testing/gtest/include/gtest/gtest.h"

class Policy;

namespace gcm {
namespace {

const char kMCSEndpoint[] = "http://my.server";
const char kMCSEndpoint2[] = "http://my.alt.server";

const int kBackoffDelayMs = 1;
const int kBackoffMultiplier = 2;

// A backoff policy with small enough delays that tests aren't burdened.
const net::BackoffEntry::Policy kTestBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential back-off in ms.
  kBackoffDelayMs,

  // Factor by which the waiting time will be multiplied.
  kBackoffMultiplier,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0,

  // Maximum amount of time we are willing to delay our request in ms.
  10,

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

std::vector<GURL> BuildEndpoints() {
  std::vector<GURL> endpoints;
  endpoints.push_back(GURL(kMCSEndpoint));
  endpoints.push_back(GURL(kMCSEndpoint2));
  return endpoints;
}

// Helper for calculating total expected exponential backoff delay given an
// arbitrary number of failed attempts. See BackoffEntry::CalculateReleaseTime.
double CalculateBackoff(int num_attempts) {
  double delay = kBackoffDelayMs;
  for (int i = 1; i < num_attempts; ++i) {
    delay += kBackoffDelayMs * pow(static_cast<double>(kBackoffMultiplier),
                                   i - 1);
  }
  DVLOG(1) << "Expected backoff " << delay << " milliseconds.";
  return delay;
}

// Helper methods that should never actually be called due to real connections
// being stubbed out.
void ReadContinuation(
    scoped_ptr<google::protobuf::MessageLite> message) {
  ADD_FAILURE();
}

void WriteContinuation() {
  ADD_FAILURE();
}

class TestBackoffEntry : public net::BackoffEntry {
 public:
  explicit TestBackoffEntry(base::SimpleTestTickClock* tick_clock);
  virtual ~TestBackoffEntry();

  virtual base::TimeTicks ImplGetTimeNow() const OVERRIDE;

 private:
  base::SimpleTestTickClock* tick_clock_;
};

TestBackoffEntry::TestBackoffEntry(base::SimpleTestTickClock* tick_clock)
    : BackoffEntry(&kTestBackoffPolicy),
      tick_clock_(tick_clock) {
}

TestBackoffEntry::~TestBackoffEntry() {}

base::TimeTicks TestBackoffEntry::ImplGetTimeNow() const {
  return tick_clock_->NowTicks();
}

// A connection factory that stubs out network requests and overrides the
// backoff policy.
class TestConnectionFactoryImpl : public ConnectionFactoryImpl {
 public:
  TestConnectionFactoryImpl(const base::Closure& finished_callback);
  virtual ~TestConnectionFactoryImpl();

  // Overridden stubs.
  virtual void ConnectImpl() OVERRIDE;
  virtual void InitHandler() OVERRIDE;
  virtual scoped_ptr<net::BackoffEntry> CreateBackoffEntry(
      const net::BackoffEntry::Policy* const policy) OVERRIDE;
  virtual base::TimeTicks NowTicks() OVERRIDE;

  // Helpers for verifying connection attempts are made. Connection results
  // must be consumed.
  void SetConnectResult(int connect_result);
  void SetMultipleConnectResults(int connect_result, int num_expected_attempts);

  base::SimpleTestTickClock* tick_clock() { return &tick_clock_; }

 private:
  // Clock for controlling delay.
  base::SimpleTestTickClock tick_clock_;
  // The result to return on the next connect attempt.
  int connect_result_;
  // The number of expected connection attempts;
  int num_expected_attempts_;
  // Whether all expected connection attempts have been fulfilled since an
  // expectation was last set.
  bool connections_fulfilled_;
  // Callback to invoke when all connection attempts have been made.
  base::Closure finished_callback_;
};

TestConnectionFactoryImpl::TestConnectionFactoryImpl(
    const base::Closure& finished_callback)
    : ConnectionFactoryImpl(BuildEndpoints(),
                            net::BackoffEntry::Policy(),
                            NULL,
                            NULL),
      connect_result_(net::ERR_UNEXPECTED),
      num_expected_attempts_(0),
      connections_fulfilled_(true),
      finished_callback_(finished_callback) {
  // Set a non-null time.
  tick_clock_.Advance(base::TimeDelta::FromMilliseconds(1));
}

TestConnectionFactoryImpl::~TestConnectionFactoryImpl() {
  EXPECT_EQ(0, num_expected_attempts_);
}

void TestConnectionFactoryImpl::ConnectImpl() {
  ASSERT_GT(num_expected_attempts_, 0);

  OnConnectDone(connect_result_);
  if (!NextRetryAttempt().is_null()) {
    // Advance the time to the next retry time.
    base::TimeDelta time_till_retry =
        NextRetryAttempt() - tick_clock_.NowTicks();
    tick_clock_.Advance(time_till_retry);
  }
  --num_expected_attempts_;
  if (num_expected_attempts_ == 0) {
    connect_result_ = net::ERR_UNEXPECTED;
    connections_fulfilled_ = true;
    finished_callback_.Run();
  }
}

void TestConnectionFactoryImpl::InitHandler() {
  EXPECT_NE(connect_result_, net::ERR_UNEXPECTED);
  ConnectionHandlerCallback(net::OK);
}

scoped_ptr<net::BackoffEntry> TestConnectionFactoryImpl::CreateBackoffEntry(
    const net::BackoffEntry::Policy* const policy) {
  return scoped_ptr<net::BackoffEntry>(new TestBackoffEntry(&tick_clock_));
}

base::TimeTicks TestConnectionFactoryImpl::NowTicks() {
  return tick_clock_.NowTicks();
}

void TestConnectionFactoryImpl::SetConnectResult(int connect_result) {
  DCHECK_NE(connect_result, net::ERR_UNEXPECTED);
  ASSERT_EQ(0, num_expected_attempts_);
  connections_fulfilled_ = false;
  connect_result_ = connect_result;
  num_expected_attempts_ = 1;
}

void TestConnectionFactoryImpl::SetMultipleConnectResults(
    int connect_result,
    int num_expected_attempts) {
  DCHECK_NE(connect_result, net::ERR_UNEXPECTED);
  DCHECK_GT(num_expected_attempts, 0);
  ASSERT_EQ(0, num_expected_attempts_);
  connections_fulfilled_ = false;
  connect_result_ = connect_result;
  num_expected_attempts_ = num_expected_attempts;
}

class ConnectionFactoryImplTest : public testing::Test {
 public:
  ConnectionFactoryImplTest();
  virtual ~ConnectionFactoryImplTest();

  TestConnectionFactoryImpl* factory() { return &factory_; }

  void WaitForConnections();

 private:
  void ConnectionsComplete();

  TestConnectionFactoryImpl factory_;
  base::MessageLoop message_loop_;
  scoped_ptr<base::RunLoop> run_loop_;
};

ConnectionFactoryImplTest::ConnectionFactoryImplTest()
   : factory_(base::Bind(&ConnectionFactoryImplTest::ConnectionsComplete,
                         base::Unretained(this))),
     run_loop_(new base::RunLoop()) {}
ConnectionFactoryImplTest::~ConnectionFactoryImplTest() {}

void ConnectionFactoryImplTest::WaitForConnections() {
  run_loop_->Run();
  run_loop_.reset(new base::RunLoop());
}

void ConnectionFactoryImplTest::ConnectionsComplete() {
  if (!run_loop_)
    return;
  run_loop_->Quit();
}

// Verify building a connection handler works.
TEST_F(ConnectionFactoryImplTest, Initialize) {
  EXPECT_FALSE(factory()->IsEndpointReachable());
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      base::Bind(&ReadContinuation),
      base::Bind(&WriteContinuation));
  ConnectionHandler* handler = factory()->GetConnectionHandler();
  ASSERT_TRUE(handler);
  EXPECT_FALSE(factory()->IsEndpointReachable());
}

// An initial successful connection should not result in backoff.
TEST_F(ConnectionFactoryImplTest, ConnectSuccess) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::OK);
  factory()->Connect();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());
  EXPECT_EQ(factory()->GetCurrentEndpoint(), BuildEndpoints()[0]);
}

// A connection failure should result in backoff, and attempting the fallback
// endpoint next.
TEST_F(ConnectionFactoryImplTest, ConnectFail) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::ERR_CONNECTION_FAILED);
  factory()->Connect();
  EXPECT_FALSE(factory()->NextRetryAttempt().is_null());
  EXPECT_EQ(factory()->GetCurrentEndpoint(), BuildEndpoints()[1]);
}

// A connection success after a failure should reset backoff.
TEST_F(ConnectionFactoryImplTest, FailThenSucceed) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::ERR_CONNECTION_FAILED);
  base::TimeTicks connect_time = factory()->tick_clock()->NowTicks();
  factory()->Connect();
  WaitForConnections();
  base::TimeTicks retry_time = factory()->NextRetryAttempt();
  EXPECT_FALSE(retry_time.is_null());
  EXPECT_GE((retry_time - connect_time).InMilliseconds(), CalculateBackoff(1));
  factory()->SetConnectResult(net::OK);
  WaitForConnections();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());
}

// Multiple connection failures should retry with an exponentially increasing
// backoff, then reset on success.
TEST_F(ConnectionFactoryImplTest, MultipleFailuresThenSucceed) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());

  const int kNumAttempts = 5;
  factory()->SetMultipleConnectResults(net::ERR_CONNECTION_FAILED,
                                       kNumAttempts);

  base::TimeTicks connect_time = factory()->tick_clock()->NowTicks();
  factory()->Connect();
  WaitForConnections();
  base::TimeTicks retry_time = factory()->NextRetryAttempt();
  EXPECT_FALSE(retry_time.is_null());
  EXPECT_GE((retry_time - connect_time).InMilliseconds(),
            CalculateBackoff(kNumAttempts));

  factory()->SetConnectResult(net::OK);
  WaitForConnections();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());
}

// IP events should reset backoff.
TEST_F(ConnectionFactoryImplTest, FailThenIPEvent) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::ERR_CONNECTION_FAILED);
  factory()->Connect();
  WaitForConnections();
  EXPECT_FALSE(factory()->NextRetryAttempt().is_null());

  factory()->OnIPAddressChanged();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());
}

// Connection type events should reset backoff.
TEST_F(ConnectionFactoryImplTest, FailThenConnectionTypeEvent) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::ERR_CONNECTION_FAILED);
  factory()->Connect();
  WaitForConnections();
  EXPECT_FALSE(factory()->NextRetryAttempt().is_null());

  factory()->OnConnectionTypeChanged(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());
}

// Fail after successful connection via signal reset.
TEST_F(ConnectionFactoryImplTest, FailViaSignalReset) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::OK);
  factory()->Connect();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());

  factory()->SignalConnectionReset(ConnectionFactory::SOCKET_FAILURE);
  EXPECT_FALSE(factory()->NextRetryAttempt().is_null());
  EXPECT_FALSE(factory()->GetConnectionHandler()->CanSendMessage());
}

TEST_F(ConnectionFactoryImplTest, IgnoreResetWhileConnecting) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::OK);
  factory()->Connect();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());

  factory()->SignalConnectionReset(ConnectionFactory::SOCKET_FAILURE);
  base::TimeTicks retry_time = factory()->NextRetryAttempt();
  EXPECT_FALSE(retry_time.is_null());

  const int kNumAttempts = 5;
  for (int i = 0; i < kNumAttempts; ++i)
    factory()->SignalConnectionReset(ConnectionFactory::SOCKET_FAILURE);
  EXPECT_EQ(retry_time, factory()->NextRetryAttempt());
}

// Go into backoff due to connection failure. On successful connection, receive
// a signal reset. The original backoff should be restored and extended, rather
// than a new backoff starting from scratch.
TEST_F(ConnectionFactoryImplTest, SignalResetRestoresBackoff) {
  factory()->Initialize(
      ConnectionFactory::BuildLoginRequestCallback(),
      ConnectionHandler::ProtoReceivedCallback(),
      ConnectionHandler::ProtoSentCallback());
  factory()->SetConnectResult(net::ERR_CONNECTION_FAILED);
  base::TimeTicks connect_time = factory()->tick_clock()->NowTicks();
  factory()->Connect();
  WaitForConnections();
  base::TimeTicks retry_time = factory()->NextRetryAttempt();
  EXPECT_FALSE(retry_time.is_null());

  factory()->SetConnectResult(net::OK);
  connect_time = factory()->tick_clock()->NowTicks();
  WaitForConnections();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());

  factory()->SignalConnectionReset(ConnectionFactory::SOCKET_FAILURE);
  EXPECT_FALSE(factory()->GetConnectionHandler()->CanSendMessage());
  EXPECT_NE(retry_time, factory()->NextRetryAttempt());
  retry_time = factory()->NextRetryAttempt();
  EXPECT_FALSE(retry_time.is_null());
  EXPECT_GE((retry_time - connect_time).InMilliseconds(),
            CalculateBackoff(2));

  factory()->SetConnectResult(net::OK);
  connect_time = factory()->tick_clock()->NowTicks();
  factory()->tick_clock()->Advance(
      factory()->NextRetryAttempt() - connect_time);
  WaitForConnections();
  EXPECT_TRUE(factory()->NextRetryAttempt().is_null());

  factory()->SignalConnectionReset(ConnectionFactory::SOCKET_FAILURE);
  EXPECT_NE(retry_time, factory()->NextRetryAttempt());
  retry_time = factory()->NextRetryAttempt();
  EXPECT_FALSE(retry_time.is_null());
  EXPECT_GE((retry_time - connect_time).InMilliseconds(),
            CalculateBackoff(3));
}

}  // namespace
}  // namespace gcm
