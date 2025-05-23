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

/*! \file rrc_gNB_reconfig.c
 * \brief rrc gNB RRCreconfiguration support routines
 * \author Raymond Knopp
 * \date 2019
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr
 */
#ifndef RRC_GNB_NSA_C
#define RRC_GNB_NSA_C

#include <stdlib.h>

#include "OCTET_STRING.h"
#include "RRC/NR/nr_rrc_proto.h"
#include "asn_codecs.h"
#include "assertions.h"
#include "constr_TYPE.h"
#include "oai_asn1.h"
#include "uper_encoder.h"

NR_RRCReconfiguration_IEs_t *get_default_reconfig(const NR_CellGroupConfig_t *secondaryCellGroup)
{
  NR_RRCReconfiguration_IEs_t *reconfig = calloc(1, sizeof(NR_RRCReconfiguration_IEs_t));
  AssertFatal(reconfig != NULL, "out of memory\n");
  AssertFatal(secondaryCellGroup!=NULL,"secondaryCellGroup is null\n");
  // radioBearerConfig
  reconfig->radioBearerConfig=NULL;

  char scg_buffer[1024];
  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig, NULL, (void *)secondaryCellGroup, scg_buffer, 1024);
  AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n", enc_rval.failed_type->name, enc_rval.encoded);
  reconfig->secondaryCellGroup = calloc(1,sizeof(*reconfig->secondaryCellGroup));
  OCTET_STRING_fromBuf(reconfig->secondaryCellGroup,
                       (const char *)scg_buffer,
                       (enc_rval.encoded+7)>>3);
  // measConfig
  reconfig->measConfig=NULL;
  // lateNonCriticalExtension
  reconfig->lateNonCriticalExtension = NULL;
  // nonCriticalExtension
  reconfig->nonCriticalExtension = NULL;
  return reconfig;
}

/* Function to set or overwrite PTRS DL RRC parameters */
void rrc_config_dl_ptrs_params(NR_BWP_Downlink_t *bwp, long *ptrsNrb, long *ptrsMcs, long *epre_Ratio, long *reOffset)
{
  int i=0;
  NR_DMRS_DownlinkConfig_t *tmp = bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
  // struct NR_SetupRelease_PTRS_DownlinkConfig *tmp=bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS;
  /* check for memory allocation  */
  if (tmp->phaseTrackingRS == NULL) {
    asn1cCalloc(tmp->phaseTrackingRS, TrackingRS);
    TrackingRS->present = NR_SetupRelease_PTRS_DownlinkConfig_PR_setup;
    asn1cCalloc(TrackingRS->choice.setup, setup);
    asn1cCalloc(setup->frequencyDensity, freqD);
    /* Fill the given values */
    for(i = 0; i < 2; i++) {
      asn1cSequenceAdd(freqD->list, long, nbr);
      *nbr = ptrsNrb[i];
    }
    asn1cCalloc(setup->timeDensity, timeD);
    for(i = 0; i < 3; i++) {
      asn1cSequenceAdd(timeD->list, long, mcs);
      *mcs = ptrsMcs[i];
    }
    asn1cCallocOne(setup->epre_Ratio, epre_Ratio[0]);
    asn1cCallocOne(setup->resourceElementOffset, reOffset[0]);
  } else {
    NR_PTRS_DownlinkConfig_t *TrackingRS = tmp->phaseTrackingRS->choice.setup;
    for(i = 0; i < 2; i++) {
      *TrackingRS->frequencyDensity->list.array[i] = ptrsNrb[i];
    }
    for(i = 0; i < 3; i++) {
      *TrackingRS->timeDensity->list.array[i] = ptrsMcs[i];
    }
    *TrackingRS->epre_Ratio = epre_Ratio[0];
    *TrackingRS->resourceElementOffset = reOffset[0];
  }
}
#endif
