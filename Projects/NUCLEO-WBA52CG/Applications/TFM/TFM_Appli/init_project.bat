::******************************************************************************
::* @file    init_project.bat
::* @author  MCD Application Team
::* @brief   Script setting environment variable for IAR
::******************************************************************************
::* @attention
::*
::* Copyright (c) 2023 STMicroelectronics.
::* All rights reserved.
::*
::* This software is licensed under terms that can be found in the LICENSE file
::* in the root directory of this software component.
::* If no LICENSE file comes with this software, it is provided AS-IS.
::*
::******************************************************************************
@ECHO ON

set iar_environment_var_file="EWARM/Project.custom_argvars"

pushd %cd%
cd ../../../../..
set firmware_path=%cd%
popd

(
echo ^<?xml version=^"1.0^" encoding=^"UTF-8^"?^>
echo ^<iarUserArgVars^>
echo    ^<group name=^"TFM_RADIO^" active=^"true^"^>
echo        ^<variable^>
echo            ^<name^>FIRMWARE_BASE_DIR^</name^>
echo            ^<value^>%firmware_path%^</value^>
echo        ^</variable^>
echo    ^</group^>
echo ^</iarUserArgVars^>
) > %iar_environment_var_file%

%~dp0/EWARM/Secure/prebuild_s.cmd "%~dp0/EWARM/Secure" "STM32WBA52xx"
