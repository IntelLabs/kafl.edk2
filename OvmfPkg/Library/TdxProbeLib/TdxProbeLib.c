/** @file
  instance of TdxProbeLib in OvmfPkg.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/BaseLib.h>
#include <Library/TdxProbeLib.h>

BOOLEAN mTdGuest = FALSE;
BOOLEAN mTdGuestProbed = FALSE;

#define TDX_WORK_AREA_OFFSET 0x10

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
  UINT8  * TdxWorkArea;

  if (mTdGuestProbed) {
    return mTdGuest;
  }
  
  TdxWorkArea = (UINT8 *)((UINTN)(FixedPcdGet32 (PcdTdMailboxBase) + TDX_WORK_AREA_OFFSET));
  mTdGuest = *TdxWorkArea != 0;
  mTdGuestProbed = TRUE;

  return mTdGuest;
}
