/** @file
  This library is BaseCrypto router. It will redirect hash request to each individual
  hash handler registered, such as SHA1, SHA256.
  Platform can use PcdTdxHashMask to mask some hash engines.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HashLib.h>
#include <Library/TdxLib.h>
#include <Library/TdxProbeLib.h>
#include <Protocol/Tdx.h>
#include <Library/TdxStartupLib.h>
#include "HashLibBaseCryptoRouterCommon.h"

HASH_INTERFACE   mHashInterface[HASH_COUNT] = {{{0}, NULL, NULL, NULL}};
UINTN            mHashInterfaceCount = 0;

UINT32           mSupportedHashMaskLast = 0;
UINT32           mSupportedHashMaskCurrent = 0;

/**
  Check mismatch of supported HashMask between modules
  that may link different HashInstanceLib instances.

**/
VOID
CheckSupportedHashMaskMismatch (
  VOID
  )
{
  if (mSupportedHashMaskCurrent != mSupportedHashMaskLast) {
    DEBUG ((
      DEBUG_WARN,
      "WARNING: There is mismatch of supported HashMask (0x%x - 0x%x) between modules\n",
      mSupportedHashMaskCurrent,
      mSupportedHashMaskLast
      ));
    DEBUG ((DEBUG_WARN, "that are linking different HashInstanceLib instances!\n"));
  }
}

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.
**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE    *HashHandle
  )
{
  HASH_HANDLE  *HashCtx;
  UINTN        Index;
  UINT32       HashMask;

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashCtx = AllocatePool (sizeof(*HashCtx) * mHashInterfaceCount);
  ASSERT (HashCtx != NULL);

  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    HashMask = Tpm2GetHashMaskFromAlgo (&mHashInterface[Index].HashGuid);
    if ((HashMask & PcdGet32 (PcdTdxHashMask)) != 0) {
      mHashInterface[Index].HashInit (&HashCtx[Index]);
    }
  }

  *HashHandle = (HASH_HANDLE)HashCtx;

  return EFI_SUCCESS;
}

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS     Hash sequence updated.
**/
EFI_STATUS
EFIAPI
HashUpdate (
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  )
{
  HASH_HANDLE  *HashCtx;
  UINTN        Index;
  UINT32       HashMask;

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashCtx = (HASH_HANDLE *)HashHandle;

  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    HashMask = Tpm2GetHashMaskFromAlgo (&mHashInterface[Index].HashGuid);
    if ((HashMask & PcdGet32 (PcdTdxHashMask)) != 0) {
      mHashInterface[Index].HashUpdate (HashCtx[Index], DataToHash, DataToHashLen);
    }
  }

  return EFI_SUCCESS;
}

/**
    MRTD     => PCR[0]
    RTMR[0]  => PCR[1,7]
    RTMR[1]  => PCR[2,3,4,5,6]
    RTMR[2]  => PCR[8~15]
    RTMR[3]  => NA

**/
UINT8 GetMappedRtmrIndex(UINT32 PCRIndex)
{
  UINT8  RtmrIndex;

  ASSERT (PCRIndex <= 16 && PCRIndex >= 0);
  RtmrIndex = 0;
  if (PCRIndex == 1 || PCRIndex == 7) {
    RtmrIndex = 0;
  } else if (PCRIndex >= 2 && PCRIndex <= 6) {
    RtmrIndex = 1;
  } else if (PCRIndex >= 8 && PCRIndex <= 15) {
    RtmrIndex = 2;
  }

  return RtmrIndex;
}

/**
  Hash sequence complete and extend to PCR.

  @param HashHandle    Hash handle.
  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.
**/
EFI_STATUS
EFIAPI
HashCompleteAndExtend (
  IN HASH_HANDLE         HashHandle,
  IN TPMI_DH_PCR         PcrIndex,
  IN VOID                *DataToHash,
  IN UINTN               DataToHashLen,
  OUT TPML_DIGEST_VALUES *DigestList
  )
{
  TPML_DIGEST_VALUES  Digest;
  HASH_HANDLE         *HashCtx;
  UINTN               Index;
  EFI_STATUS          Status;
  UINT32              HashMask;

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashCtx = (HASH_HANDLE *)HashHandle;
  ZeroMem (DigestList, sizeof(*DigestList));

  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    HashMask = Tpm2GetHashMaskFromAlgo (&mHashInterface[Index].HashGuid);
    if ((HashMask & PcdGet32 (PcdTdxHashMask)) != 0) {
      mHashInterface[Index].HashUpdate (HashCtx[Index], DataToHash, DataToHashLen);
      mHashInterface[Index].HashFinal (HashCtx[Index], &Digest);
      Tpm2SetHashToDigestList (DigestList, &Digest);
    }
  }

  FreePool (HashCtx);

  ASSERT(DigestList->count == 1 && DigestList->digests[0].hashAlg == TPM_ALG_SHA384);

  Status = TdExtendRtmr (
             (UINT32*)DigestList->digests[0].digest.sha384,
             SHA384_DIGEST_SIZE,
             GetMappedRtmrIndex(PcrIndex)
             );
  return Status;
}

/**
  Hash data and extend to PCR.

  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash data and DigestList is returned.
**/
EFI_STATUS
EFIAPI
HashAndExtend (
  IN TPMI_DH_PCR                    PcrIndex,
  IN VOID                           *DataToHash,
  IN UINTN                          DataToHashLen,
  OUT TPML_DIGEST_VALUES            *DigestList
  )
{
  HASH_HANDLE    HashHandle;
  EFI_STATUS     Status;

  DEBUG((DEBUG_INFO, "Td: HashAndExtend: %d, %p, 0x%x\n", PcrIndex, DataToHash, DataToHashLen));

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashStart (&HashHandle);
  HashUpdate (HashHandle, DataToHash, DataToHashLen);
  Status = HashCompleteAndExtend (HashHandle, PcrIndex, NULL, 0, DigestList);

  return Status;
}

/**
  This service register Hash.

  @param HashInterface  Hash interface

  @retval EFI_SUCCESS          This hash interface is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this interface.
  @retval EFI_ALREADY_STARTED  System already register this interface.
**/
EFI_STATUS
EFIAPI
RegisterHashInterfaceLib (
  IN HASH_INTERFACE   *HashInterface
  )
{
  UINTN              Index;
  UINT32             HashMask;
  EFI_STATUS         Status;

  //
  // Check allow
  //
  HashMask = Tpm2GetHashMaskFromAlgo (&HashInterface->HashGuid);
  if ((HashMask & PcdGet32 (PcdTdxHashMask)) == 0) {
    return EFI_UNSUPPORTED;
  }
  DEBUG((DEBUG_INFO, "TD: Hash is registered. mask = 0x%x, guid = %g\n", HashMask, HashInterface->HashGuid));

  if (mHashInterfaceCount >= sizeof(mHashInterface)/sizeof(mHashInterface[0])) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check duplication
  //
  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    if (CompareGuid (&mHashInterface[Index].HashGuid, &HashInterface->HashGuid)) {
      DEBUG ((DEBUG_ERROR, "Hash Interface (%g) has been registered\n", &HashInterface->HashGuid));
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Record hash algorithm bitmap of CURRENT module which consumes HashLib.
  //
  mSupportedHashMaskCurrent = PcdGet32 (PcdTdxHashAlgorithmBitmap) | HashMask;
  Status = PcdSet32S (PcdTdxHashAlgorithmBitmap, mSupportedHashMaskCurrent);
  ASSERT_EFI_ERROR (Status);

  CopyMem (&mHashInterface[mHashInterfaceCount], HashInterface, sizeof(*HashInterface));
  mHashInterfaceCount ++;

  return EFI_SUCCESS;
}

/**
  The constructor function of HashLibBaseCryptoRouterDxe.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
HashLibBaseCryptoRouterDxeConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;

  if (!ProbeTdGuest()) {
    return EFI_SUCCESS;
  }
 
  //
  // Record hash algorithm bitmap of LAST module which also consumes HashLib.
  //
  mSupportedHashMaskLast = PcdGet32 (PcdTdxHashAlgorithmBitmap);

  //
  // Set PcdTdxHashAlgorithmBitmap to 0 in CONSTRUCTOR for CURRENT module.
  //
  Status = PcdSet32S (PcdTdxHashAlgorithmBitmap, 0);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
