/** @file
  Defines the HOB GUID used to pass a TD_EVENT from SEC to
  a TDX DXE Driver. A GUIDed HOB is generated for each measurement
  made in the SEC Phase.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TD_EVENT_HOB_H_
#define TD_EVENT_HOB_H_

///
/// The Global ID of a GUIDed HOB used to pass a TD_EVENT from a SEC to a TDX DXE Driver.
///
#define EFI_TD_EVENT_HOB_GUID \
  { 0x20f8fd36, 0x6d00, 0x40fb, { 0xb7, 0x4, 0xd1, 0x2c, 0x15, 0x3c, 0x62, 0xeb }}

extern EFI_GUID gTdEventEntryHobGuid;


#endif
