/** @file
  instance of TdxProbeLib

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/BaseLib.h>
#include <Library/TdxProbeLib.h>
#include "InternalTdxProbe.h"

BOOLEAN mTdGuest = FALSE;
BOOLEAN mTdGuestProbed = FALSE;

/**
  Probe whether it is TD guest or Non-TD guest.

  @return TRUE    TD guest
  @return FALSE   Non-TD guest
**/
BOOLEAN
EFIAPI
ProbeTdGuest (
  VOID)
{
  if (mTdGuestProbed) {
    return mTdGuest;
  }

  mTdGuest = TdProbe() == 0;
  mTdGuestProbed = TRUE;
  return mTdGuest;
}
