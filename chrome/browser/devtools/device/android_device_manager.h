// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVICE_ANDROID_DEVICE_MANAGER_H_
#define CHROME_BROWSER_DEVTOOLS_DEVICE_ANDROID_DEVICE_MANAGER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/geometry/size.h"

namespace net {
class StreamSocket;
}

class AndroidDeviceManager : public base::NonThreadSafe {
 public:
  typedef base::Callback<void(int, const std::string&)> CommandCallback;
  typedef base::Callback<void(int result, scoped_ptr<net::StreamSocket>)>
      SocketCallback;
  typedef base::Callback<void(
      int result, const std::string& extensions, scoped_ptr<net::StreamSocket>)>
      HttpUpgradeCallback;
  typedef base::Callback<void(const std::vector<std::string>&)> SerialsCallback;

  struct BrowserInfo {
    BrowserInfo();

    enum Type {
      kTypeChrome,
      kTypeWebView,
      kTypeOther
    };

    std::string socket_name;
    std::string display_name;
    Type type;
  };

  struct DeviceInfo {
    DeviceInfo();
    ~DeviceInfo();

    std::string model;
    bool connected;
    gfx::Size screen_size;
    std::vector<BrowserInfo> browser_info;
  };

  typedef base::Callback<void(const DeviceInfo&)> DeviceInfoCallback;
  class Device;

  class AndroidWebSocket {
   public:
    class Delegate {
     public:
      virtual void OnSocketOpened() = 0;
      virtual void OnFrameRead(const std::string& message) = 0;
      virtual void OnSocketClosed() = 0;

     protected:
      virtual ~Delegate() {}
    };

    ~AndroidWebSocket();

    void SendFrame(const std::string& message);

   private:
    friend class Device;
    class WebSocketImpl;

    AndroidWebSocket(
        scoped_refptr<Device> device,
        const std::string& socket_name,
        const std::string& url,
        AndroidWebSocket::Delegate* delegate);
    void Connected(int result,
                   const std::string& extensions,
                   scoped_ptr<net::StreamSocket> socket);
    void OnFrameRead(const std::string& message);
    void OnSocketClosed();
    void Terminate();

    Device* device_;
    WebSocketImpl* socket_impl_;
    Delegate* delegate_;
    base::WeakPtrFactory<AndroidWebSocket> weak_factory_;
    DISALLOW_COPY_AND_ASSIGN(AndroidWebSocket);
  };

  class DeviceProvider;

  class Device : public base::RefCountedThreadSafe<Device>,
                 public base::NonThreadSafe {
   public:
    void QueryDeviceInfo(const DeviceInfoCallback& callback);

    void OpenSocket(const std::string& socket_name,
                    const SocketCallback& callback);

    void SendJsonRequest(const std::string& socket_name,
                         const std::string& request,
                         const CommandCallback& callback);

    void HttpUpgrade(const std::string& socket_name,
                     const std::string& url,
                     const std::string& extensions,
                     const HttpUpgradeCallback& callback);
    AndroidWebSocket* CreateWebSocket(
        const std::string& socket_name,
        const std::string& url,
        AndroidWebSocket::Delegate* delegate);

    std::string serial() { return serial_; }

   private:
    friend class base::RefCountedThreadSafe<Device>;
    friend class AndroidDeviceManager;
    friend class AndroidWebSocket;

    Device(scoped_refptr<base::MessageLoopProxy> device_message_loop,
           scoped_refptr<DeviceProvider> provider,
           const std::string& serial);

    virtual ~Device();

    scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;
    scoped_refptr<DeviceProvider> provider_;
    std::string serial_;
    std::set<AndroidWebSocket*> sockets_;
    base::WeakPtrFactory<Device> weak_factory_;

    DISALLOW_COPY_AND_ASSIGN(Device);
  };

  typedef std::vector<scoped_refptr<Device> > Devices;
  typedef base::Callback<void(const Devices&)> DevicesCallback;

  class DeviceProvider : public base::RefCountedThreadSafe<DeviceProvider> {
   public:
    typedef AndroidDeviceManager::SerialsCallback SerialsCallback;
    typedef AndroidDeviceManager::DeviceInfoCallback DeviceInfoCallback;
    typedef AndroidDeviceManager::SocketCallback SocketCallback;
    typedef AndroidDeviceManager::CommandCallback CommandCallback;

    virtual void QueryDevices(const SerialsCallback& callback) = 0;

    virtual void QueryDeviceInfo(const std::string& serial,
                                 const DeviceInfoCallback& callback) = 0;

    virtual void OpenSocket(const std::string& serial,
                            const std::string& socket_name,
                            const SocketCallback& callback) = 0;

    virtual void SendJsonRequest(const std::string& serial,
                                 const std::string& socket_name,
                                 const std::string& request,
                                 const CommandCallback& callback);

    virtual void HttpUpgrade(const std::string& serial,
                             const std::string& socket_name,
                             const std::string& url,
                             const std::string& extensions,
                             const HttpUpgradeCallback& callback);

    virtual void ReleaseDevice(const std::string& serial);

   protected:
    friend class base::RefCountedThreadSafe<DeviceProvider>;
    DeviceProvider();
    virtual ~DeviceProvider();
  };

  typedef std::vector<scoped_refptr<DeviceProvider> > DeviceProviders;

  virtual ~AndroidDeviceManager();

  static scoped_ptr<AndroidDeviceManager> Create();

  void SetDeviceProviders(const DeviceProviders& providers);

  void QueryDevices(const DevicesCallback& callback);

  struct DeviceDescriptor {
    DeviceDescriptor();
    ~DeviceDescriptor();

    scoped_refptr<DeviceProvider> provider;
    std::string serial;
  };

  typedef std::vector<DeviceDescriptor> DeviceDescriptors;

 private:
  class HandlerThread : public base::RefCountedThreadSafe<HandlerThread> {
   public:
    static scoped_refptr<HandlerThread> GetInstance();
    scoped_refptr<base::MessageLoopProxy> message_loop();

   private:
    friend class base::RefCountedThreadSafe<HandlerThread>;
    static HandlerThread* instance_;
    static void StopThread(base::Thread* thread);

    HandlerThread();
    virtual ~HandlerThread();
    base::Thread* thread_;
  };

  AndroidDeviceManager();

  void UpdateDevices(const DevicesCallback& callback,
                     scoped_ptr<DeviceDescriptors> descriptors);

  typedef std::map<std::string, base::WeakPtr<Device> > DeviceWeakMap;

  scoped_refptr<HandlerThread> handler_thread_;
  DeviceProviders providers_;
  DeviceWeakMap devices_;

  base::WeakPtrFactory<AndroidDeviceManager> weak_factory_;
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVICE_ANDROID_DEVICE_MANAGER_H_
