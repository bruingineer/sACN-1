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

/*********** CHRISTIAN's BIG OL' TODO LIST: *************************************
 - Implement this & get documentation in place, including the overal C & C++ "how to use" comments.
 - Clean up any constants for this API in opts.h.  For example, any of the source detector thread priorities or
 receive timeouts.
 - Test c & c++
 - Make sure everything works with static & dynamic memory.
 - Example app for both C & C++.
 - Make the example detector use the new api.
 - This entire project should build without warnings!!
 - Make sure the functionality for create & reset_networking work with and without good_interfaces, in all combinations
 (nill, small array, large array, etc).
*/
////////////////////////////////////////////

#include "sacn/source_detector.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include "sacn/private/mem.h"
#include "sacn/private/receiver_state.h"
#include "sacn/private/source_detector.h"

#if SACN_DYNAMIC_MEM
#include <stdlib.h>
#else
#include "etcpal/mempool.h"
#endif

/***************************** Private constants *****************************/

static const EtcPalThreadParams kSourceDetectorThreadParams = {
    SACN_SOURCE_DETECTOR_THREAD_PRIORITY, SACN_SOURCE_DETECTOR_THREAD_STACK, "sACN Source Detector Thread", NULL};

/****************************** Private macros *******************************/

/**************************** Private variables ******************************/

/*********************** Private function prototypes *************************/

/*************************** Function definitions ****************************/

/**************************************************************************************************
 * API functions
 *************************************************************************************************/

/* Initialize the sACN Source Detector module. Internal function called from sacn_init(). */
etcpal_error_t sacn_source_detector_init(void)
{
  // TODO CHRISTIAN
  return kEtcPalErrOk;
}

/* Deinitialize the sACN Source Detector module. Internal function called from sacn_deinit(). */
void sacn_source_detector_deinit(void)
{
  // TODO CHRISTIAN
}

/**
 * @brief Initialize an sACN Source Detector Config struct to default values.
 *
 * @param[out] config Config struct to initialize.
 */
void sacn_source_detector_config_init(SacnSourceDetectorConfig* config)
{
  if (config)
  {
    memset(config, 0, sizeof(SacnSourceDetectorConfig));
  }
}

/**
 * @brief Create the sACN Source Detector.
 *
 * Note that the detector is considered as successfully created if it is able to successfully use any of the
 * network interfaces passed in.  This will only return #kEtcPalErrNoNetints if none of the interfaces work.
 *
 * @param[in] config Configuration parameters for the sACN source detector.
 * @param[in, out] netints Optional. If non-NULL, this is the list of interfaces the application wants to use, and the
 * status codes are filled in.  If NULL, all available interfaces are tried.
 * @param[in, out] num_netints Optional. The size of netints, or 0 if netints is NULL.
 * @return #kEtcPalErrOk: Detector created successfully.
 * @return #kEtcPalErrNoNetints: None of the network interfaces provided were usable by the library.
 * @return #kEtcPalErrInvalid: Invalid parameter provided.
 * @return #kEtcPalErrNotInit: Module not initialized.
 * @return #kEtcPalErrNoMem: No room to allocate memory for the detector.
 * @return #kEtcPalErrNotFound: A network interface ID given was not found on the system.
 * @return #kEtcPalErrSys: An internal library or system call error occurred.
 */
etcpal_error_t sacn_source_detector_create(const SacnSourceDetectorConfig* config, SacnMcastInterface* netints,
                                           size_t num_netints)
{
  etcpal_error_t res = kEtcPalErrOk;

  if (!sacn_initialized())
    res = kEtcPalErrNotInit;
  else if (!config || !config->callbacks.source_updated || !config->callbacks.source_expired)
    res = kEtcPalErrInvalid;

  if (sacn_lock())
  {
    SacnSourceDetector* source_detector = NULL;
    if (res == kEtcPalErrOk)
      res = add_sacn_source_detector(config, netints, num_netints, &source_detector);

    if (res == kEtcPalErrOk)
      res = assign_source_detector_to_thread(source_detector);

    if ((res != kEtcPalErrOk) && source_detector)
    {
      remove_source_detector_from_thread(source_detector, kCloseSocketNow);
      remove_sacn_source_detector();
    }

    sacn_unlock();
  }
  else
  {
    res = kEtcPalErrSys;
  }

  return res;
}

/**
 * @brief Destroy the sACN Source Detector.
 *
 */
void sacn_source_detector_destroy()
{
  if (sacn_initialized() && sacn_lock())
  {
    remove_source_detector_from_thread(get_sacn_source_detector(), kQueueSocketForClose);
    remove_sacn_source_detector();

    sacn_unlock();
  }
}

/**
 * @brief Resets the underlying network sockets and packet receipt state for the sACN Source Detector.
 *
 * This is typically used when the application detects that the list of networking interfaces has changed.
 *
 * After this call completes successfully, the detector will continue as if nothing had changed. New sources could be
 * discovered, or old sources could expire.
 * If this call fails, the caller must call sacn_source_detector_destroy, because the detector may be in an invalid
 * state.
 *
 * Note that the networking reset is considered successful if it is able to successfully use any of the
 * network interfaces passed in.  This will only return #kEtcPalErrNoNetints if none of the interfaces work.
 *
 * @param[in, out] netints Optional. If non-NULL, this is the list of interfaces the application wants to use, and the
 * status codes are filled in.  If NULL, all available interfaces are tried.
 * @param[in, out] num_netints Optional. The size of netints, or 0 if netints is NULL.
 * @return #kEtcPalErrOk: Network changed successfully.
 * @return #kEtcPalErrNoNetints: None of the network interfaces provided were usable by the library.
 * @return #kEtcPalErrInvalid: Invalid parameter provided.
 * @return #kEtcPalErrNotInit: Module not initialized.
 * @return #kEtcPalErrNotFound: The detector has not been created yet.
 * @return #kEtcPalErrSys: An internal library or system call error occurred.
 */
etcpal_error_t sacn_source_detector_reset_networking(SacnMcastInterface* netints, size_t num_netints)
{
  etcpal_error_t res = kEtcPalErrOk;

  if (!sacn_initialized())
    res = kEtcPalErrNotInit;

  if (sacn_lock())
  {
    if (res == kEtcPalErrOk)
      res = sacn_sockets_reset_source_detector();

    if (res == kEtcPalErrOk)
    {
      SacnSourceDetector* detector = get_sacn_source_detector();

      // All current sockets need to be removed before adding new ones.
      remove_source_detector_sockets(detector, kQueueSocketForClose);

      res = sacn_initialize_source_detector_netints(&detector->netints, netints, num_netints);
      if (res == kEtcPalErrOk)
        res = add_source_detector_sockets(detector);

      // TODO: Refresh source/universe tracking
    }

    sacn_unlock();
  }

  return res;
}

/**
 * @brief Obtain the statuses of the source detector's network interfaces.
 *
 * @param[out] netints A pointer to an application-owned array where the network interface list will be written.
 * @param[in] netints_size The size of the provided netints array.
 * @return The total number of network interfaces for the source detector. If this is greater than netints_size, then
 * only netints_size addresses were written to the netints array. If the source detector has not been created yet, 0 is
 * returned.
 */
size_t sacn_source_detector_get_network_interfaces(SacnMcastInterface* netints, size_t netints_size)
{
  ETCPAL_UNUSED_ARG(netints);
  ETCPAL_UNUSED_ARG(netints_size);

  return 0;  // TODO
}
