// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/system/main.h>

#include "apps/maxwell/context_service/context_service.mojom.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace {

using mojo::ApplicationImplBase;
using mojo::RunLoop;

using intelligence::ContextListener;
using intelligence::ContextPublisherPtr;
using intelligence::ContextSubscriberPtr;
using intelligence::ContextUpdatePtr;
using intelligence::OnUpdateCallback;
using intelligence::PublisherPipePtr;
using intelligence::Status;

class CarmenSandiego : public ApplicationImplBase, public ContextListener {
 public:
  void OnInitialize() override {
    ContextSubscriberPtr cxin;
    ContextPublisherPtr cxout;
    ConnectToService(shell(), "mojo:context_service", GetProxy(&cxin));
    ConnectToService(shell(), "mojo:context_service", GetProxy(&cxout));

    // TODO(rosswang): V0 does not support semantic differentiation by source,
    // so the labels have to be explicitly different. In the future, these could
    // all be refinements on "location"
    cxout->StartPublishing("agents/location", GetProxy(&location_));
    cxin->Subscribe({ "GPS location" }, mojo::InterfaceHandle<ContextListener> listener)
  }

  void OnUpdate(mojo::Map<mojo::String, ContextUpdatePtr> update,
                const OnUpdateCallback& callback) {
    location->Publish("high-level location", const mojo::String &value, const PublishCallback &callback)
  }

 private:
  PublisherPipePtr location_;
};

} // namespace

MojoResult MojoMain(MojoHandle request) {
  GpsAcquirer app;
  return mojo::RunApplication(request, &app);
}
