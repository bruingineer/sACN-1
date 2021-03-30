/******************************************************************************
 * Copyright 2021 ETC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************
 * This file is a part of sACN. For more information, go to:
 * https://github.com/ETCLabs/sACN
 *****************************************************************************/

#ifndef SACN_CPP_COMMON_H_
#define SACN_CPP_COMMON_H_

/**
 * @file sacn/cpp/common.h
 * @brief C++ wrapper for the sACN init/deinit functions
 */

#include "etcpal/cpp/error.h"
#include "etcpal/cpp/log.h"
#include "sacn/common.h"

/**
 * @defgroup sacn_cpp_api sACN C++ Language APIs
 * @brief Native C++ APIs for interfacing with the sACN library.
 *
 * These wrap the corresponding C modules in a nicer syntax for C++ application developers.
 */

/**
 * @defgroup sacn_cpp_common Common Definitions
 * @ingroup sacn_cpp_api
 * @brief Definitions shared by other APIs in this module.
 */

/**
 * @brief A namespace which contains all C++ language definitions in the sACN library.
 */
namespace sacn
{

/**
 * @ingroup sacn_cpp_common
 * @brief Initialize the sACN library.
 *
 * Wraps sacn_init(). Does all initialization required before the sACN API modules can be
 * used.
 *
 * @param log_params (optional) Log parameters for the sACN library to use to log messages. If
 *                   not provided, no logging will be performed.
 * @return etcpal::Error::Ok(): Initialization successful.
 * @return Errors from sacn_init().
 */
inline etcpal::Error Init(const EtcPalLogParams* log_params = nullptr)
{
  return sacn_init(log_params);
}

/**
 * @ingroup sacn_cpp_common
 * @brief Initialize the sACN library.
 *
 * Wraps sacn_init(). Does all initialization required before the sACN API modules can be
 * used.
 *
 * @param logger Logger instance for the sACN library to use to log messages.
 * @return etcpal::Error::Ok(): Initialization successful.
 * @return Errors from sacn_init().
 */
inline etcpal::Error Init(const etcpal::Logger& logger)
{
  return sacn_init(&logger.log_params());
}

/**
 * @ingroup sacn_cpp_common
 * @brief Deinitialize the sACN library.
 *
 * Closes all connections, deallocates all resources and joins the background thread. No sACN
 * API functions are usable after this function is called.
 *
 * This function is not thread safe with respect to other sACN API functions. Make sure to join your threads that use
 * the APIs before calling this.
 */
inline void Deinit()
{
  return sacn_deinit();
}

};  // namespace sacn

#endif  // SACN_CPP_COMMON_H_
