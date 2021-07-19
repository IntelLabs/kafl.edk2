/** @file
  Header file for Tdx IO library.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __IOLIB_TDX_H__
#define __IOLIB_TDX_H__

BOOLEAN
EFIAPI
IsTdxGuest (
  VOID
  );

UINT8
EFIAPI
TdIoRead8 (
  IN      UINTN                     Port
  );

UINT16
EFIAPI
TdIoRead16 (
  IN      UINTN                     Port
  );

UINT32
EFIAPI
TdIoRead32 (
  IN      UINTN                     Port
  );

UINT8
EFIAPI
TdIoWrite8 (
  IN      UINTN                     Port,
  IN      UINT8                     Value
  );

UINT16
EFIAPI
TdIoWrite16 (
  IN      UINTN                     Port,
  IN      UINT16                    Value
  );

UINT32
EFIAPI
TdIoWrite32 (
  IN      UINTN                     Port,
  IN      UINT32                    Value
  );

UINT8
EFIAPI
TdMmioRead8 (
  IN      UINTN                     Address
  );

UINT8
EFIAPI
TdMmioWrite8 (
  IN      UINTN                     Address,
  IN      UINT8                     Val
  );

UINT16
EFIAPI
TdMmioRead16 (
  IN      UINTN                     Address
  );

UINT16
EFIAPI
TdMmioWrite16 (
  IN      UINTN                     Address,
  IN      UINT16                    Val
  );

UINT32
EFIAPI
TdMmioRead32 (
  IN      UINTN                     Address
  );

UINT32
EFIAPI
TdMmioWrite32 (
  IN      UINTN                     Address,
  IN      UINT32                    Val
  );

UINT64
EFIAPI
TdMmioRead64 (
  IN      UINTN                     Address
  );

UINT64
EFIAPI
TdMmioWrite64 (
  IN      UINTN                     Address,
  IN      UINT64                    Value
  );

#endif
