/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptLib.h>
#include <Library/PcdLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Register/QemuSmramSaveStateMap.h>
#include <Register/SmramSaveStateMap.h>
#include <Uefi/UefiBaseType.h>

STATIC BOOLEAN mSevStatus = FALSE;
STATIC BOOLEAN mSevEsStatus = FALSE;
STATIC BOOLEAN mSevStatusChecked = FALSE;
STATIC UINTN   mSevStatusMask = 0;

/**
  Reads and sets the status of SEV features.

  **/
STATIC
VOID
EFIAPI
InternalMemEncryptSevStatus (
  VOID
  )
{
  UINT32                            RegEax;
  MSR_SEV_STATUS_REGISTER           Msr;
  CPUID_MEMORY_ENCRYPTION_INFO_EAX  Eax;

  //
  // Check if memory encryption leaf exist
  //
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_MEMORY_ENCRYPTION_INFO) {
    //
    // CPUID Fn8000_001F[EAX] Bit 1 (Sev supported)
    //
    AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, &Eax.Uint32, NULL, NULL, NULL);

    if (Eax.Bits.SevBit) {
      //
      // Check MSR_0xC0010131 Bit 0 (Sev Enabled)
      //
      Msr.Uint32 = AsmReadMsr32 (MSR_SEV_STATUS);
      if (Msr.Bits.SevBit) {
        mSevStatus = TRUE;
        mSevStatusMask |= MEM_ENCRYPT_SEV_ENABLED;
      }

      //
      // Check MSR_0xC0010131 Bit 1 (Sev-Es Enabled)
      //
      if (Msr.Bits.SevEsBit) {
        mSevEsStatus = TRUE;
        mSevStatusMask |= MEM_ENCRYPT_SEVES_ENABLED;
      }
    }
  }

  mSevStatusChecked = TRUE;
}


/**
  Returns boolean to indicate whether to indicate which, if any, memory encryption is enabled

  @param[in]  Type          Bitmask of encryption technologies to check is enabled

  @retval TRUE              The encryption type(s) are enabled
  @retval FALSE             The encryption type(s) are not enabled
**/
BOOLEAN
EFIAPI
MemEncryptIsEnabled (
  IN UINTN  Type
  )
{
  if (!mSevStatusChecked) {
    InternalMemEncryptSevStatus ();
  }
  //
  // If query any encrpytion type this library doesn't support, return FALSE
  //
  if (Type & ~(MEM_ENCRYPT_SEV_ENABLED | MEM_ENCRYPT_SEVES_ENABLED)) {
    return FALSE;
  }
  //
  // Determine if all interested type are enabled
  //
  return (((Type & mSevStatusMask) == Type) ? TRUE : FALSE);
}

/**
  Returns a boolean to indicate whether SEV is enabled.

  @retval TRUE           SEV is enabled
  @retval FALSE          SEV is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevIsEnabled (
  VOID
  )
{
  if (!mSevStatusChecked) {
    InternalMemEncryptSevStatus ();
  }

  return mSevStatus;
}


/**
  Locate the page range that covers the initial (pre-SMBASE-relocation) SMRAM
  Save State Map.

  @param[out] BaseAddress     The base address of the lowest-address page that
                              covers the initial SMRAM Save State Map.

  @param[out] NumberOfPages   The number of pages in the page range that covers
                              the initial SMRAM Save State Map.

  @retval RETURN_SUCCESS      BaseAddress and NumberOfPages have been set on
                              output.

  @retval RETURN_UNSUPPORTED  SMM is unavailable.
**/
RETURN_STATUS
EFIAPI
MemEncryptLocateInitialSmramSaveStateMapPages (
  OUT UINTN *BaseAddress,
  OUT UINTN *NumberOfPages
  )
{
  UINTN MapStart;
  UINTN MapEnd;
  UINTN MapPagesStart; // MapStart rounded down to page boundary
  UINTN MapPagesEnd;   // MapEnd rounded up to page boundary
  UINTN MapPagesSize;  // difference between MapPagesStart and MapPagesEnd

  if (!FeaturePcdGet (PcdSmmSmramRequire)) {
    return RETURN_UNSUPPORTED;
  }

  MapStart      = SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET;
  MapEnd        = MapStart + sizeof (QEMU_SMRAM_SAVE_STATE_MAP);
  MapPagesStart = MapStart & ~(UINTN)EFI_PAGE_MASK;
  MapPagesEnd   = ALIGN_VALUE (MapEnd, EFI_PAGE_SIZE);
  MapPagesSize  = MapPagesEnd - MapPagesStart;

  ASSERT ((MapPagesSize & EFI_PAGE_MASK) == 0);

  *BaseAddress   = MapPagesStart;
  *NumberOfPages = MapPagesSize >> EFI_PAGE_SHIFT;

  return RETURN_SUCCESS;
}
