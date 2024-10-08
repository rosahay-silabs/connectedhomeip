/*
 *
 *    Copyright (c) 2022-2023 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "binding-handler.h"
#include <static-supported-modes-manager.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

AppTask AppTask::sAppTask;
chip::app::Clusters::ModeSelect::StaticSupportedModesManager sStaticSupportedModesManager;

CHIP_ERROR AppTask::Init(void)
{
    InitCommonParts();

    // Configure Bindings
    CHIP_ERROR err = InitBindingHandlers();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("InitBindingHandlers fail");
        return err;
    }

    chip::app::Clusters::ModeSelect::setSupportedModesManager(&sStaticSupportedModesManager);
    return CHIP_NO_ERROR;
}
