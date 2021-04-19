#ifndef __TDX_STARTUP_LIB_H__
#define __TDX_STARTUP_LIB_H__

#include <Library/BaseLib.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Pi/PiPeiCis.h>
#include <Library/DebugLib.h>
#include <Protocol/DebugSupport.h>
#include <IndustryStandard/Tpm20.h>

#pragma pack(1)

typedef struct {
  UINT32            count;
  TPMI_ALG_HASH     hashAlg;
  BYTE              sha384[SHA384_DIGEST_SIZE];
} TDX_DIGEST_VALUE;

typedef struct {
  UINT32            Signature;
  UINT64            HashDataPtr;
  UINT64            HashDataLen;
} TDX_EVENT;

#pragma pack()


typedef
VOID
(EFIAPI * fProcessLibraryConstructorList)(
  IN EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES   **PeiServices
  );


VOID
EFIAPI
TdxStartup(
  IN VOID                           * Context,
  IN VOID                           * VmmHobList,
  IN UINTN                          Info,
  IN fProcessLibraryConstructorList Function
);

#endif
