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
// Tests exercising the Chrome Plugin API.

#include "base/file_util.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/browser/chrome_plugin_host.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/url_fetcher.h"
#include "chrome/common/chrome_plugin_lib.h"
#include "chrome/test/chrome_plugin/test_chrome_plugin.h"
#include "net/url_request/url_request_test_job.h"
#include "net/url_request/url_request_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const wchar_t kDocRoot[] = L"chrome/test/data";
const wchar_t kPluginFilename[] = L"test_chrome_plugin.dll";

class ChromePluginTest : public testing::Test, public URLRequest::Delegate {
 public:
  ChromePluginTest()
      : request_(NULL),
        plugin_(NULL),
        expected_payload_(NULL),
        request_context_(new TestURLRequestContext()) {
  }

  // Loads/unloads the chrome test plugin.
  void LoadPlugin();
  void UnloadPlugin();

  // Runs the test and expects the given payload as a response.  If expectation
  // is NULL, the request is expected to fail.
  void RunTest(const GURL& url, const TestResponsePayload* expected_payload);

  // URLRequest::Delegate implementations
  virtual void OnReceivedRedirect(URLRequest* request,
                                  const GURL& new_url) { }
  virtual void OnResponseStarted(URLRequest* request);
  virtual void OnReadCompleted(URLRequest* request, int bytes_read);

  // Helper called when the URLRequest is done.
  void OnURLRequestComplete();

  // testing::Test
  virtual void SetUp() {
    LoadPlugin();
    URLRequest::RegisterProtocolFactory("test", &URLRequestTestJob::Factory);

    // We need to setup a default request context in order to issue HTTP
    // requests.
    DCHECK(!Profile::GetDefaultRequestContext());
    Profile::set_default_request_context(request_context_.get());
  }
  virtual void TearDown() {
    UnloadPlugin();
    URLRequest::RegisterProtocolFactory("test", NULL);

    Profile::set_default_request_context(NULL);
  }
 protected:
  // Note: we use URLRequest (instead of URLFetcher) because this allows the
  // request to be intercepted.
  scoped_ptr<URLRequest> request_;
  char response_buffer_[4096];
  std::string response_data_;

  ChromePluginLib* plugin_;
  TestFuncParams::PluginFuncs test_funcs_;
  const TestResponsePayload* expected_payload_;
  scoped_refptr<URLRequestContext> request_context_;
};

static void STDCALL CPT_Complete(CPRequest* request, bool success,
                                 const std::string& raw_headers,
                                 const std::string& body) {
  GURL url(request->url);
  if (url == GURL(kChromeTestPluginPayloads[0].url)) {
    // This URL should fail, because the plugin should not have intercepted it.
    EXPECT_FALSE(success);
    MessageLoop::current()->Quit();
    return;
  }

  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(raw_headers));
  EXPECT_TRUE(success);
  EXPECT_EQ(200, headers->response_code());

  if (url == URLRequestTestJob::test_url_1()) {
    EXPECT_EQ(URLRequestTestJob::test_data_1(), body);
  } else if (url == URLRequestTestJob::test_url_2()) {
    EXPECT_EQ(URLRequestTestJob::test_data_2(), body);
  } else if (url.spec().find("echo") != std::string::npos) {
    EXPECT_EQ(kChromeTestPluginPostData, body);
  }

  MessageLoop::current()->Quit();
}

static void STDCALL CPT_InvokeLater(TestFuncParams::CallbackFunc callback,
                                    void* callback_data, int delay_ms) {
  MessageLoop::current()->PostDelayedTask(FROM_HERE,
      NewRunnableFunction(callback, callback_data), delay_ms);
}

void ChromePluginTest::LoadPlugin() {
  std::wstring path;
  PathService::Get(base::DIR_EXE, &path);
  file_util::AppendToPath(&path, kPluginFilename);
  plugin_ = ChromePluginLib::Create(path, GetCPBrowserFuncsForBrowser());

  // Exchange test APIs with the plugin.
  TestFuncParams params;
  params.bfuncs.test_complete = CPT_Complete;
  params.bfuncs.invoke_later = CPT_InvokeLater;
  EXPECT_EQ(CPERR_SUCCESS, plugin_->CP_Test(&params));
  test_funcs_ = params.pfuncs;

  EXPECT_TRUE(plugin_);
}

void ChromePluginTest::UnloadPlugin() {
  ChromePluginLib::UnloadAllPlugins();
  plugin_ = NULL;
}

void ChromePluginTest::RunTest(const GURL& url,
                               const TestResponsePayload* expected_payload) {
  expected_payload_ = expected_payload;

  response_data_.clear();
  request_.reset(new URLRequest(url, this));
  request_->set_context(new TestURLRequestContext());
  request_->Start();

  MessageLoop::current()->Run();
}

void ChromePluginTest::OnResponseStarted(URLRequest* request) {
  DCHECK(request == request_);

  int bytes_read = 0;
  if (request_->status().is_success())
    request_->Read(response_buffer_, sizeof(response_buffer_), &bytes_read);
  OnReadCompleted(request_.get(), bytes_read);
}

void ChromePluginTest::OnReadCompleted(URLRequest* request, int bytes_read) {
  DCHECK(request == request_);

  do {
    if (!request_->status().is_success() || bytes_read <= 0)
      break;
    response_data_.append(response_buffer_, bytes_read);
  } while (request_->Read(response_buffer_, sizeof(response_buffer_),
                          &bytes_read));

  if (!request_->status().is_io_pending()) {
    OnURLRequestComplete();
  }
}

void ChromePluginTest::OnURLRequestComplete() {
  if (expected_payload_) {
    EXPECT_TRUE(request_->status().is_success());

    EXPECT_EQ(expected_payload_->status, request_->GetResponseCode());
    if (expected_payload_->mime_type) {
      std::string mime_type;
      EXPECT_TRUE(request_->response_headers()->GetMimeType(&mime_type));
      EXPECT_EQ(expected_payload_->mime_type, mime_type);
    }
    if (expected_payload_->body) {
      EXPECT_EQ(expected_payload_->body, response_data_);
    }
  } else {
    EXPECT_FALSE(request_->status().is_success());
  }

  MessageLoop::current()->Quit();
  // If MessageLoop::current() != main_loop_, it will be shut down when the
  // main loop returns and this thread subsequently goes out of scope.
}

};  // namespace

// Tests that the plugin can intercept URLs.
TEST_F(ChromePluginTest, DoesIntercept) {
  for (int i = 0; i < arraysize(kChromeTestPluginPayloads); ++i) {
    RunTest(GURL(kChromeTestPluginPayloads[i].url),
            &kChromeTestPluginPayloads[i]);
  }
}

// Tests that non-intercepted URLs are handled normally.
TEST_F(ChromePluginTest, DoesNotIntercept) {
  TestResponsePayload about_blank = {
    "about:blank",
    false,
    -1,
    NULL,
    ""
  };
  RunTest(GURL(about_blank.url), &about_blank);
}

// Tests that unloading the plugin correctly unregisters URL interception.
TEST_F(ChromePluginTest, UnregisterIntercept) {
  UnloadPlugin();

  RunTest(GURL(kChromeTestPluginPayloads[0].url), NULL);
}

static void ProcessAllPendingMessages() {
  while (URLRequestTestJob::ProcessOnePendingMessage());
}

// Tests that the plugin can issue a GET request and receives the data when
// it comes back synchronously.
TEST_F(ChromePluginTest, CanMakeGETRequestSync) {
  // test_url_1 has a synchronous response
  EXPECT_EQ(CPERR_SUCCESS, test_funcs_.test_make_request(
      "GET", URLRequestTestJob::test_url_1()));

  // Note: we must add this task after we make the request, so that
  // URLRequestTestJob's StartAsync task is added and run first.
  MessageLoop::current()->PostTask(FROM_HERE,
      NewRunnableFunction(&ProcessAllPendingMessages));
  MessageLoop::current()->Run();
}

// Tests that the plugin can issue a GET request and receives the data when
// it comes back asynchronously.
TEST_F(ChromePluginTest, CanMakeGETRequestAsync) {
  // test_url_2 has an asynchronous response
  EXPECT_EQ(CPERR_SUCCESS, test_funcs_.test_make_request(
        "GET", URLRequestTestJob::test_url_2()));

  // Note: we must add this task after we make the request, so that
  // URLRequestTestJob's StartAsync task is added and run first.
  MessageLoop::current()->PostTask(FROM_HERE,
      NewRunnableFunction(&ProcessAllPendingMessages));
  MessageLoop::current()->Run();
}

// Tests that the plugin can issue a POST request.
TEST_F(ChromePluginTest, CanMakePOSTRequest) {
  TestServer server(kDocRoot);
  GURL url = server.TestServerPage("echo");

  EXPECT_EQ(CPERR_SUCCESS, test_funcs_.test_make_request("POST", url));

  // Note: we must add this task after we make the request, so that
  // URLRequestTestJob's StartAsync task is added and run first.
  MessageLoop::current()->PostTask(FROM_HERE,
      NewRunnableFunction(&ProcessAllPendingMessages));
  MessageLoop::current()->Run();
}

// Tests that the plugin does not intercept its own requests.
TEST_F(ChromePluginTest, DoesNotInterceptOwnRequest) {
  const TestResponsePayload& payload = kChromeTestPluginPayloads[0];

  EXPECT_EQ(CPERR_SUCCESS, test_funcs_.test_make_request(
      "GET", GURL(payload.url)));

  MessageLoop::current()->Run();
}
