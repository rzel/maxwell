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
using mojo::Binding;
using mojo::InterfaceHandle;
using mojo::InterfaceRequest;
using mojo::RunLoop;

using intelligence::ContextListener;
using intelligence::ContextListenerPtr;
using intelligence::ContextPublisherPtr;
using intelligence::ContextSubscriberPtr;
using intelligence::ContextUpdatePtr;
using intelligence::PublisherPipePtr;
using intelligence::Status;

class CarmenSandiego : public ApplicationImplBase, public ContextListener {
 public:
  CarmenSandiego(): listener_(this) {}

  void OnInitialize() override {
    ContextSubscriberPtr cxin;
    ContextPublisherPtr cxout;
    ConnectToService(shell(), "mojo:context_service", GetProxy(&cxin));
    ConnectToService(shell(), "mojo:context_service", GetProxy(&cxout));

    // TODO(rosswang): V0 does not support semantic differentiation by source,
    // so the labels have to be explicitly different. In the future, these could
    // all be refinements on "location"
    cxout->StartPublishing("agents/carmen_sandiego", GetProxy(&loc_out_));

    ContextListenerPtr listener_ptr;
    listener_.Bind(GetProxy(&listener_ptr).Pass());
    // TODO(rosswang): If we want to keep the label-array form of Subscribe as
    // canonical, we should probably work on supporting Mojo Array literals.
    mojo::Array<mojo::String> labels;
    labels.push_back("GPS location");
    cxin->Subscribe(labels.Pass(), listener_ptr.PassInterfaceHandle());
  }

  void OnUpdate(mojo::Map<mojo::String, ContextUpdatePtr> update,
                const OnUpdateCallback& callback) override {
    //location->Publish("high-level location", "", [](Status){});
    ContextUpdatePtr gps_location = update["GPS location"].Pass();
    MOJO_LOG(INFO) << "OnUpdate from " << gps_location->source << ": "
                   << gps_location->json_value;
  }

 private:
  PublisherPipePtr loc_out_;
  Binding<ContextListener> listener_;
};

} // namespace

MojoResult MojoMain(MojoHandle request) {
  CarmenSandiego app;
  return mojo::RunApplication(request, &app);
}
