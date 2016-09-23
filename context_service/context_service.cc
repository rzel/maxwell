// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/system/main.h>

#include "apps/maxwell/context_service/context_service.mojom.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

using mojo::ApplicationImplBase;
using mojo::BindingSet;
using mojo::InterfaceHandle;
using mojo::InterfaceRequest;
using mojo::ServiceProviderImpl;

using intelligence::ContextListener;
using intelligence::ContextListenerPtr;
using intelligence::ContextPublisher;
using intelligence::ContextSubscriber;
using intelligence::ContextUpdate;
using intelligence::ContextUpdatePtr;
using intelligence::PublisherPipe;
using intelligence::Status;

class PublisherPipeImpl : public PublisherPipe {
 public:
  PublisherPipeImpl(const mojo::String& whoami,
                    InterfaceRequest<PublisherPipe> handle)
      : strong_binding_(this, handle.Pass()), whoami_(whoami) {}

  ~PublisherPipeImpl() {
    MOJO_LOG(INFO) << "publisher " << whoami_ << " terminated";
  }

  void Publish(const mojo::String& label, const mojo::String& value,
               const PublishCallback& callback) override {
    MOJO_LOG(INFO) << "publisher " << whoami_ << " set value "
                   << label << ": " << value;
    callback.Run(Status::Ok);
  }

 private:
  mojo::StrongBinding<PublisherPipe> strong_binding_;
  mojo::String whoami_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(PublisherPipeImpl);
};

struct ContextEntry {
  ContextUpdate latest;
  std::vector<ContextListenerPtr*> listeners;
};

class ContextServiceImpl : public ContextPublisher, public ContextSubscriber {
 public:
  ContextServiceImpl() {}

  void StartPublishing(const mojo::String& whoami,
                       InterfaceRequest<PublisherPipe> pipe) override {
    MOJO_LOG(INFO) << "StartPublishing " << whoami;

    new PublisherPipeImpl(whoami, pipe.Pass());
  }

  // TODO(rosswang): additional backpressure modes. For now, just do
  // on-backpressure-buffer, which is the default for Mojo.
  void Subscribe(mojo::Array<mojo::String> labels,
                 InterfaceHandle<ContextListener> listener_handle) override {
    MOJO_LOG(INFO) << "Subscribe to " << labels.data();

    // TODO(rosswang): interface ptr lifecycle management (remove on error)
    ContextListenerPtr* listener = new ContextListenerPtr(
      ContextListenerPtr::Create(listener_handle.Pass()));
    mojo::Map<mojo::String, ContextUpdatePtr> initial_snapshot;

    for (mojo::String label : labels) {
      ContextEntry* entry = &repo[label];
      entry->listeners.push_back(listener);
      initial_snapshot[label] = entry->latest.Clone();
    }

    (*listener)->OnUpdate(initial_snapshot.Pass(), [](Status){});
  }

 private:
  std::map<std::string, ContextEntry> repo;
  MOJO_DISALLOW_COPY_AND_ASSIGN(ContextServiceImpl);
};

class ContextServiceApp : public ApplicationImplBase {
 public:
  ContextServiceApp() {}

  bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<ContextPublisher>(
        [this](const mojo::ConnectionContext& connection_context,
               mojo::InterfaceRequest<ContextPublisher> request) {
          // All channels will connect to this singleton object, so just
          // add the binding to our collection.
          bindings_.AddBinding(&cxs_impl_, request.Pass());
        });
    return true;
  }

 private:
  ContextServiceImpl cxs_impl_;
  mojo::BindingSet<ContextPublisher> bindings_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(ContextServiceApp);
};

} // namespace

MojoResult MojoMain(MojoHandle request) {
  ContextServiceApp app;
  return mojo::RunApplication(request, &app);
}
