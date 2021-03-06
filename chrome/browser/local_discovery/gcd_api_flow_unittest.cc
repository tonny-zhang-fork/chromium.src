// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/local_discovery/gcd_api_flow.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/values.h"
#include "chrome/browser/local_discovery/gcd_api_flow_impl.h"
#include "content/public/test/test_browser_thread.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Return;

namespace local_discovery {

namespace {

const char kSampleConfirmResponse[] = "{}";

const char kFailedConfirmResponseBadJson[] = "[]";

const char kAccountId[] = "account_id";

class MockDelegate : public CloudPrintApiFlowRequest {
 public:
  MOCK_METHOD1(OnGCDAPIFlowError, void(GCDApiFlow::Status));
  MOCK_METHOD1(OnGCDAPIFlowComplete, void(const base::DictionaryValue&));

  MOCK_METHOD0(GetURL, GURL());
};

class GCDApiFlowTest : public testing::Test {
 public:
  GCDApiFlowTest()
      : ui_thread_(content::BrowserThread::UI, &loop_),
        request_context_(new net::TestURLRequestContextGetter(
            base::MessageLoopProxy::current())),
        account_id_(kAccountId) {}

  ~GCDApiFlowTest() override {}

 protected:
  void SetUp() override {
    token_service_.set_request_context(request_context_.get());
    token_service_.AddAccount(account_id_);
    ui_thread_.Stop();  // HACK: Fake being on the UI thread

    scoped_ptr<MockDelegate> delegate(new MockDelegate);
    mock_delegate_ = delegate.get();
    EXPECT_CALL(*mock_delegate_, GetURL())
        .WillRepeatedly(Return(
            GURL("https://www.google.com/cloudprint/confirm?token=SomeToken")));
    gcd_flow_.reset(new GCDApiFlowImpl(
        request_context_.get(), &token_service_, account_id_));
    gcd_flow_->Start(delegate.Pass());
  }
  base::MessageLoopForUI loop_;
  content::TestBrowserThread ui_thread_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory fetcher_factory_;
  FakeOAuth2TokenService token_service_;
  std::string account_id_;
  scoped_ptr<GCDApiFlowImpl> gcd_flow_;
  MockDelegate* mock_delegate_;
};

TEST_F(GCDApiFlowTest, SuccessOAuth2) {
  gcd_flow_->OnGetTokenSuccess(NULL, "SomeToken", base::Time());
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);

  EXPECT_EQ(GURL("https://www.google.com/cloudprint/confirm?token=SomeToken"),
            fetcher->GetOriginalURL());

  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string oauth_header;
  std::string proxy;
  EXPECT_TRUE(headers.GetHeader("Authorization", &oauth_header));
  EXPECT_EQ("Bearer SomeToken", oauth_header);
  EXPECT_TRUE(headers.GetHeader("X-Cloudprint-Proxy", &proxy));
  EXPECT_EQ("Chrome", proxy);

  fetcher->SetResponseString(kSampleConfirmResponse);
  fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(*mock_delegate_, OnGCDAPIFlowComplete(_));

  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

TEST_F(GCDApiFlowTest, BadToken) {
  EXPECT_CALL(*mock_delegate_, OnGCDAPIFlowError(GCDApiFlow::ERROR_TOKEN));
  gcd_flow_->OnGetTokenFailure(
      NULL, GoogleServiceAuthError(GoogleServiceAuthError::USER_NOT_SIGNED_UP));
}

TEST_F(GCDApiFlowTest, BadJson) {
  gcd_flow_->OnGetTokenSuccess(NULL, "SomeToken", base::Time());
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);

  EXPECT_EQ(GURL("https://www.google.com/cloudprint/confirm?token=SomeToken"),
            fetcher->GetOriginalURL());

  fetcher->SetResponseString(kFailedConfirmResponseBadJson);
  fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(*mock_delegate_,
              OnGCDAPIFlowError(GCDApiFlow::ERROR_MALFORMED_RESPONSE));

  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

}  // namespace

}  // namespace local_discovery
