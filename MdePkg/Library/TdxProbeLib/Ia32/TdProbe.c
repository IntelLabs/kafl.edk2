/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/BaseLib.h>
#include "InternalTdxProbe.h"

/**
  The internal Td Probe implementation.

  @return 0       TD guest
  @return others  Non-TD guest
**/
UINTN
EFIAPI
TdProbe (
  VOID
  )
{
  return -1;
}
