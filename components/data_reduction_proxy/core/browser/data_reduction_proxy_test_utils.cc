// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_test_utils.h"

#include "base/message_loop/message_loop.h"
#include "base/prefs/testing_pref_service.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config_test_utils.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_configurator_test_utils.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_interceptor.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_prefs.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_statistics_prefs.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_store.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params_test_utils.h"
#include "net/url_request/url_request_test_util.h"

namespace data_reduction_proxy {

MockDataReductionProxyService::MockDataReductionProxyService(
    scoped_ptr<DataReductionProxyStatisticsPrefs> statistics_prefs,
    DataReductionProxySettings* settings,
    net::URLRequestContextGetter* request_context)
    : DataReductionProxyService(
        statistics_prefs.Pass(), settings, request_context) {
}

MockDataReductionProxyService::~MockDataReductionProxyService() {
}

TestDataReductionProxyIOData::TestDataReductionProxyIOData(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    scoped_ptr<TestDataReductionProxyConfig> config,
    scoped_ptr<DataReductionProxyEventStore> event_store,
    scoped_ptr<DataReductionProxyRequestOptions> request_options,
    scoped_ptr<DataReductionProxyConfigurator> configurator)
    : DataReductionProxyIOData() {
  io_task_runner_ = task_runner;
  ui_task_runner_ = task_runner;
  config_ = config.Pass();
  event_store_ = event_store.Pass();
  request_options_ = request_options.Pass();
  configurator_ = configurator.Pass();
  io_task_runner_ = task_runner;
  ui_task_runner_ = task_runner;
}

TestDataReductionProxyIOData::~TestDataReductionProxyIOData() {
  shutdown_on_ui_ = true;
}

DataReductionProxyTestContext::DataReductionProxyTestContext(
    int params_flags,
    unsigned int params_definitions,
    unsigned int test_context_flags,
    net::URLRequestContext* request_context)
    : test_context_flags_(test_context_flags),
      task_runner_(base::MessageLoopProxy::current()),
      request_context_getter_(scoped_refptr<net::URLRequestContextGetter>(
          new net::TrivialURLRequestContextGetter(request_context,
                                                  task_runner_))) {
  Init(params_flags, params_definitions);
}

DataReductionProxyTestContext::DataReductionProxyTestContext(
    int params_flags,
    unsigned int params_definitions,
    unsigned int test_context_flags)
    : test_context_flags_(test_context_flags),
      task_runner_(base::MessageLoopProxy::current()),
      request_context_getter_(scoped_refptr<net::URLRequestContextGetter>(
          new net::TestURLRequestContextGetter(task_runner_))) {
  Init(params_flags, params_definitions);
}

DataReductionProxyTestContext::~DataReductionProxyTestContext() {
}

void DataReductionProxyTestContext::Init(int params_flags,
                                         unsigned int params_definitions) {
  scoped_ptr<DataReductionProxyEventStore> event_store =
      make_scoped_ptr(new DataReductionProxyEventStore(task_runner_));
  scoped_ptr<DataReductionProxyConfigurator> configurator;
  if (test_context_flags_ &
      DataReductionProxyTestContext::USE_TEST_CONFIGURATOR) {
    configurator = make_scoped_ptr(new TestDataReductionProxyConfigurator(
        task_runner_, &net_log_, event_store.get()));
  } else {
    configurator = make_scoped_ptr(new DataReductionProxyConfigurator(
        task_runner_, &net_log_, event_store.get()));
  }

  scoped_ptr<TestDataReductionProxyConfig> config;
  if (test_context_flags_ & DataReductionProxyTestContext::USE_MOCK_CONFIG) {
    config.reset(new MockDataReductionProxyConfig(
        params_flags, params_definitions, task_runner_, &net_log_,
        configurator.get(), event_store.get()));
  } else {
    config.reset(new TestDataReductionProxyConfig(
        params_flags, params_definitions, task_runner_, &net_log_,
        configurator.get(), event_store.get()));
  }
  scoped_ptr<DataReductionProxyRequestOptions> request_options =
      make_scoped_ptr(new DataReductionProxyRequestOptions(
          Client::UNKNOWN, config.get(), task_runner_));
  settings_.reset(new DataReductionProxySettings());

  RegisterSimpleProfilePrefs(simple_pref_service_.registry());

  io_data_.reset(new TestDataReductionProxyIOData(
      task_runner_.get(), config.Pass(), event_store.Pass(),
      request_options.Pass(), configurator.Pass()));
  io_data_->InitOnUIThread(&simple_pref_service_);

  if (!(test_context_flags_ &
        DataReductionProxyTestContext::SKIP_SETTINGS_INITIALIZATION)) {
    settings_->InitDataReductionProxySettings(
        &simple_pref_service_, io_data_.get(),
        CreateDataReductionProxyServiceInternal());
    io_data_->SetDataReductionProxyService(
        settings_->data_reduction_proxy_service()->GetWeakPtr());
  }
}

void DataReductionProxyTestContext::RunUntilIdle() {
  base::MessageLoop::current()->RunUntilIdle();
}

void DataReductionProxyTestContext::InitSettings() {
  DCHECK(test_context_flags_ &
         DataReductionProxyTestContext::SKIP_SETTINGS_INITIALIZATION);
  settings_->InitDataReductionProxySettings(
      &simple_pref_service_, io_data_.get(), CreateDataReductionProxyService());
  io_data_->SetDataReductionProxyService(
      settings_->data_reduction_proxy_service()->GetWeakPtr());
}

scoped_ptr<DataReductionProxyService>
DataReductionProxyTestContext::CreateDataReductionProxyService() {
  DCHECK(test_context_flags_ &
         DataReductionProxyTestContext::SKIP_SETTINGS_INITIALIZATION);
  return CreateDataReductionProxyServiceInternal();
}

scoped_ptr<DataReductionProxyService>
DataReductionProxyTestContext::CreateDataReductionProxyServiceInternal() {
  scoped_ptr<DataReductionProxyStatisticsPrefs> statistics_prefs =
      make_scoped_ptr(new DataReductionProxyStatisticsPrefs(
          &simple_pref_service_, task_runner_, base::TimeDelta()));

  if (test_context_flags_ & DataReductionProxyTestContext::USE_MOCK_SERVICE) {
    return make_scoped_ptr(new MockDataReductionProxyService(
        statistics_prefs.Pass(), settings_.get(),
        request_context_getter_.get()));
  } else {
    return make_scoped_ptr(
        new DataReductionProxyService(statistics_prefs.Pass(), settings_.get(),
                                      request_context_getter_.get()));
  }
}

TestDataReductionProxyConfigurator*
DataReductionProxyTestContext::test_configurator() const {
  DCHECK(test_context_flags_ &
         DataReductionProxyTestContext::USE_TEST_CONFIGURATOR);
  return reinterpret_cast<TestDataReductionProxyConfigurator*>(
      io_data_->configurator());
}

MockDataReductionProxyConfig* DataReductionProxyTestContext::mock_config()
    const {
  DCHECK(test_context_flags_ & DataReductionProxyTestContext::USE_MOCK_CONFIG);
  return reinterpret_cast<MockDataReductionProxyConfig*>(io_data_->config());
}

DataReductionProxyService*
DataReductionProxyTestContext::data_reduction_proxy_service() const {
  return settings_->data_reduction_proxy_service();
}

MockDataReductionProxyService*
DataReductionProxyTestContext::mock_data_reduction_proxy_service()
    const {
  DCHECK(!(test_context_flags_ &
           DataReductionProxyTestContext::SKIP_SETTINGS_INITIALIZATION));
  DCHECK(test_context_flags_ & DataReductionProxyTestContext::USE_MOCK_SERVICE);
  return reinterpret_cast<MockDataReductionProxyService*>(
      data_reduction_proxy_service());
}

}  // namespace data_reduction_proxy
