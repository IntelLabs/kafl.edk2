/** @file
Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/EventExitBootServiceFailed.h>

#include <Protocol/DevicePath.h>
#include <Protocol/MpService.h>
#include <Protocol/ResetNotification.h>
#include <Protocol/AcpiTable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/PerformanceLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/AcpiTdx.h>

typedef struct {
  EFI_TDX_STORAGE_VOLUME_KEY_ACPI_TABLE Table;
  EFI_TDX_STORAGE_VOLUME_KEY_STRUCT Key1;
  EFI_TDX_STORAGE_VOLUME_KEY_STRUCT Key2;
} EFI_TDVF_SVF_TEST;

EFI_TDVF_SVF_TEST mTdvfSVKAcpiTemplate = {
  {
    {
    EFI_TDX_STORAGE_VOLUME_KEY_ACPI_TABLE_SIGNATURE,
    sizeof(mTdvfSVKAcpiTemplate),
    EFI_TDX_STORAGE_VOLUME_KEY_ACPI_TABLE_REVISION,
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in production
    //
    },
    2,  // KeyCount
  },
  {
    0, // KeyType
    0, // KeyFormat
    3 * 1024, // KeySize
    0, // KeyAddress
  },
  {
    0, // KeyType
    0, // KeyFormat
    3 * 1024, // KeySize
    0, // KeyAddress
  }
};

/**
  Install TDVF ACPI Table when ACPI Table Protocol is available.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallAcpiTable (
  IN EFI_EVENT                      Event,
  IN VOID*                          Context
  )
{
  UINTN                             TableKey;
  EFI_STATUS                        Status;
  EFI_ACPI_TABLE_PROTOCOL           *AcpiTable;
  UINT64                            OemTableId;
  EFI_PHYSICAL_ADDRESS              Keys;


  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "TCG2: AcpiTableProtocol is not installed. %r\n", Status));
    return;
  }

  Status = gBS->AllocatePages (
                      AllocateAnyPages,
                      EfiACPIMemoryNVS,
                      2,
                      &Keys
                      );

  if (EFI_ERROR (Status)) {
    return;
  }

  SetMem ((VOID *) Keys, EFI_PAGE_SIZE, 0x11);
  mTdvfSVKAcpiTemplate.Key1.KeyAddress = Keys;

  Keys += EFI_PAGE_SIZE;
  SetMem ((VOID *) Keys, EFI_PAGE_SIZE, 0x22);
  mTdvfSVKAcpiTemplate.Key2.KeyAddress = Keys;

  CopyMem (mTdvfSVKAcpiTemplate.Table.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (mTdvfSVKAcpiTemplate.Table.Header.OemId));
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&mTdvfSVKAcpiTemplate.Table.Header.OemTableId, &OemTableId, sizeof (UINT64));
  mTdvfSVKAcpiTemplate.Table.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  mTdvfSVKAcpiTemplate.Table.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  mTdvfSVKAcpiTemplate.Table.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Construct ACPI Table
  Status = AcpiTable->InstallAcpiTable(
                        AcpiTable,
                        &mTdvfSVKAcpiTemplate,
                        mTdvfSVKAcpiTemplate.Table.Header.Length,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  DEBUG((DEBUG_INFO, "TDVF SVKL ACPI Table is installed.\n"));
}

/**
  The driver's entry point. It publishes EFI Tcg2 Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
DriverEntry (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  VOID                              *Registration;
  // 
  // Create event callback to install TDVF ACPI Table
  EfiCreateProtocolNotifyEvent (&gEfiAcpiTableProtocolGuid, TPL_CALLBACK, InstallAcpiTable, NULL, &Registration);
  return EFI_SUCCESS;
}
