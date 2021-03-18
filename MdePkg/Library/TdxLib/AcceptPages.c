/** @file

  There are 4 defined types in TD memory.
  Unaccepted memory is a special type of private memory. The guest
  firmware must invoke TDCALL [TDG.MEM.PAGE.ACCEPT] the unaccepted
  memory before use it.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Tdx.h>
#include <Library/TdxLib.h>
#include <Library/BaseMemoryLib.h>

UINT64  mNumberOfDuplicatedAcceptedPages;

/**
  This function accept a pending private page, and initialize the page to
  all-0 using the TD ephemeral private key.

  @param[in]  StartAddress      Guest physical address of the private
                                page to accept.
  @param[in]  NumberOfPages     Number of the pages to be accepted.
  @param[in]  PageSize          GPA page size. Only accept 1G/2M/4K size.

  @return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
TdAcceptPages (
  IN UINT64  StartAddress,
  IN UINT64  NumberOfPages,
  IN UINT64  PageSize
  )
{
  UINT64  Address;
  UINT64  Status;
  UINT64  Index;
  UINT64  GpaPageSize;

  Address = StartAddress;

  if (PageSize == SIZE_4KB) {
    GpaPageSize = TDCALL_ACCEPT_PAGE_SIZE_4K;
  } else if (PageSize == SIZE_2MB) {
    GpaPageSize = TDCALL_ACCEPT_PAGE_SIZE_2M;
  } else if (PageSize == SIZE_1GB) {
    GpaPageSize = TDCALL_ACCEPT_PAGE_SIZE_1G;
  } else {
    DEBUG ((DEBUG_ERROR, "Accept page size must be 4K/2M/1G. Invalid page size - 0x%llx\n", PageSize));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < NumberOfPages; Index++) {
    Status = TdCall (TDCALL_TDACCEPTPAGE,Address, GpaPageSize, 0, 0);
    if (Status != TDX_EXIT_REASON_SUCCESS) {
        if ((Status & ~0xFFULL) == TDX_EXIT_REASON_PAGE_ALREADY_ACCEPTED) {
          mNumberOfDuplicatedAcceptedPages++;
          DEBUG ((DEBUG_VERBOSE, "Address %llx already accepted. Total number of already accepted pages %ld\n",
            Address, mNumberOfDuplicatedAcceptedPages));
        } else {
          DEBUG ((DEBUG_ERROR, "Address %llx failed to be accepted. Error = %ld\n",
            Address, Status));
          ASSERT (Status == TDX_EXIT_REASON_SUCCESS);
        }
    }
    Address += PageSize;
  }
  return EFI_SUCCESS;
}
