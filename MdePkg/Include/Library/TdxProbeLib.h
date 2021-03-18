/** @file
  TdxProbeLib definitions

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TDX_PROBE_LIB_H_
#define _TDX_PROBE_LIB_H_

#include <Library/BaseLib.h>

/**
  Check whether it is TD guest or Non-TD guest

  @return TRUE    TD guest
  @return FALSE   Non-TD guest
**/
BOOLEAN
EFIAPI
ProbeTdGuest (
  VOID);

#endif
