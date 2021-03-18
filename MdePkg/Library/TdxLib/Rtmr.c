/** @file

  Extends one of the RTMR measurement registers in TDCS with the provided
  extension data in memory.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TdxLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/Tdx.h>

#define RTMR_COUNT                  4
#define TD_EXTEND_BUFFER_LEN        (16 * 4 + 32)
#define EXTEND_BUFFER_ADDRESS_MASK  0x3f


#pragma pack(16)
typedef struct {
  UINT8   Buffer[TD_EXTEND_BUFFER_LEN];
} TDX_EXTEND_BUFFER;
#pragma pack()

UINT8               *mExtendBufferAddress = NULL;
TDX_EXTEND_BUFFER   mExtendBuffer;

/**
  TD.RTMR.EXTEND requires 64B-aligned guest physical address of
  48B-extension data.
  #pragma pack() only supports 1/2/4/8/16, So we pre-allocate a
  (16*4 + 32) length buffer in stack. In runtime we walk thru the
  Buffer to find out a 64B-aligned start address.

  @return Start address of the extend buffer

**/
UINT8 *
EFIAPI
GetExtendBuffer (
  VOID
  )
{
  UINT8     ExtendBufferStart;
  UINT8     *ExtendBufferAddress;

  if (mExtendBufferAddress != NULL) {
    return mExtendBufferAddress;
  }

  ExtendBufferStart = 0;
  ExtendBufferAddress = mExtendBuffer.Buffer;

  while (ExtendBufferStart < TD_EXTEND_BUFFER_LEN) {
    ExtendBufferAddress += ExtendBufferStart;
    if (((UINT64)(UINTN)ExtendBufferAddress & EXTEND_BUFFER_ADDRESS_MASK) == 0) {
      mExtendBufferAddress = ExtendBufferAddress;
      break;
    } else {
      ExtendBufferStart += 16;
    }
  }

  ASSERT (ExtendBufferStart < TD_EXTEND_BUFFER_LEN);
  DEBUG ((DEBUG_VERBOSE, "ExtendBufferAddress: 0x%p, 0x%x\n", ExtendBufferAddress, ExtendBufferStart));
  return mExtendBufferAddress;
}


/**
  This function extends one of the RTMR measurement register
  in TDCS with the provided extension data in memory.
  RTMR extending supports SHA384 which length is 48 bytes.

  @param[in]  Data      Point to the data to be extended
  @param[in]  DataLen   Length of the data. Must be 48
  @param[in]  Index     RTMR index

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER
  @return EFI_DEVICE_ERROR

**/
EFI_STATUS
EFIAPI
TdExtendRtmr (
  IN  UINT32  *Data,
  IN  UINT32  DataLen,
  IN  UINT8   Index
  )
{
  EFI_STATUS            Status;
  UINT64                TdCallStatus;
  UINT8                 *ExtendBuffer;

  Status = EFI_SUCCESS;

  ASSERT (Index >= 0 && Index < RTMR_COUNT);
  ASSERT (DataLen == SHA384_DIGEST_SIZE);

  ExtendBuffer = GetExtendBuffer();
  ASSERT (ExtendBuffer != NULL);
  ZeroMem (ExtendBuffer, SHA384_DIGEST_SIZE);
  CopyMem (ExtendBuffer, Data, SHA384_DIGEST_SIZE);

  TdCallStatus = TdCall (TDCALL_TDEXTENDRTMR, (UINT64)(UINTN)ExtendBuffer, Index, 0, 0);

  if (TdCallStatus == TDX_EXIT_REASON_SUCCESS) {
    Status = EFI_SUCCESS;
  } else if (TdCallStatus == TDX_EXIT_REASON_OPERAND_INVALID) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = EFI_DEVICE_ERROR;
  }

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Error returned from TdExtendRtmr call - 0x%lx\n", TdCallStatus));
  }

  return Status;
}
