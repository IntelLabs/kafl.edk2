/** @file
  Null instance of TdxProbeLib.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/BaseLib.h>
#include <Library/TdxProbeLib.h>

/**
  Probe whether it is TD guest or Non-TD guest.

  @return TRUE    TD guest
  @return FALSE   Non-TD guest
**/
BOOLEAN
EFIAPI
ProbeTdGuest (
  VOID )
{
  return FALSE;
}
