/** @file
  TBD
  
  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __TDX_ACPI_H__
#define __TDX_ACPI_H__

#include <IndustryStandard/Acpi.h>

#define EFI_TDX_EVENTLOG_ACPI_TABLE_SIGNATURE  SIGNATURE_32('T', 'D', 'E', 'L')
#define EFI_TDX_EVENTLOG_ACPIT_TABLE_REVISION  1

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  UINT32                      Rsvd;
  UINT64                      Laml;
  UINT64                      Lasa;
} EFI_TDX_EVENTLOG_ACPI_TABLE;

#pragma pack()

#endif