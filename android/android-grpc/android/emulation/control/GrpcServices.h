// Copyright (C) 2019 The Android Open Source Project
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

#include <string>
#include "android/console.h"
#include "android/emulation/control/EmulatorService.h"

namespace android {
namespace emulation {

namespace control {
// This class holds various utility functions for Windows host systems.
// All methods here must be static!
class GrpcServices {
public:
    static void setup(std::string args, const AndroidConsoleAgents* const consoleAgents);

    static void teardown();

private:
    static std::unique_ptr<EmulatorControllerService> g_controler_service;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
