// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once
#include "android/emulation/control/keyboard/EventSender.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

// Class that sends Android events on the current looper.
class AndroidEventSender : public EventSender<AndroidEvent> {
public:
    AndroidEventSender(const AndroidConsoleAgents* const consoleAgents) : EventSender<AndroidEvent>(consoleAgents) {};
    ~AndroidEventSender() = default;

protected:
    void doSend(const AndroidEvent event) override;
};

}  // namespace control
}  // namespace emulation
}  // namespace android