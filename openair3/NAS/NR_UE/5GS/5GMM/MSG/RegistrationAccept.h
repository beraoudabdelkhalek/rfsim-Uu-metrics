/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file RegistrationAccept.h

\brief 5GS registration accept procedures
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "SecurityHeaderType.h"
#include "MessageType.h"
#include "FGSRegistrationResult.h"
#include "FGSMobileIdentity.h"

#ifndef REGISTRATION_ACCEPT_H_
#define REGISTRATION_ACCEPT_H_

// 9.11.3.37 of 3GPP TS 24.501
typedef struct {
  int sst;
  int hplmn_sst;
  int sd;
  int hplmn_sd;
} nr_nas_msg_snssai_t;

/*
 * Message name: Registration accept
 * Description: The REGISTRATION ACCEPT message is sent by the AMF to the UE. See table 8.2.7.1.1.
 * Significance: dual
 * Direction: network to UE
 */

typedef struct registration_accept_msg_tag {
  /* Mandatory fields */
  FGSRegistrationResult fgsregistrationresult;
  /* Optional fields */
  FGSMobileIdentity *guti;
  // Allowed NSSAI (O)
  nr_nas_msg_snssai_t nas_allowed_nssai[8];
  // Configured NSSAI (O)
  nr_nas_msg_snssai_t config_nssai[8];
} registration_accept_msg;

int decode_registration_accept(registration_accept_msg *registrationaccept, const uint8_t *buffer, uint32_t len);

int encode_registration_accept(const registration_accept_msg *registrationaccept, uint8_t *buffer, uint32_t len);

#endif /* ! defined(REGISTRATION_ACCEPT_H_) */
