#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Guid/ShellLibHiiGuid.h>
#include <Uefi.h>
#include <Protocol/Cpu.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Ip4.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Arp.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Protocol/HiiPackageList.h>
#include <Library/HttpLib.h>
#include <Protocol/LoadKey.h>
#include <Library/TimerLib.h>


typedef struct _NETAGENT_PRIVATE_DATA {
  BOOLEAN                       Verbose;
  CHAR16                        *EndPoint;
  VOID                          *Buffer;
  UINTN                          BufferSize;
  UINTN                          Delay;
} NETAGENT_PRIVATE_DATA;

EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE                  ImageHandle
  );


VOID
Cleanup (
  NETAGENT_PRIVATE_DATA *Private
  );



STATIC EFI_HII_HANDLE gHiiHandle = NULL;

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-s", TypeValue},
  {L"-v", TypeFlag},
  {L"-d", TypeValue},
  {NULL , TypeMax}
  };

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE                  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  EFI_HII_HANDLE              HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  return HiiHandle;
}

EFI_STATUS
ProcessCommandLine(
  IN NETAGENT_PRIVATE_DATA           *Private
  )
{
  EFI_STATUS          Status;
  CHAR16              *ProblemParam;
  CONST CHAR16        *ValueStr;
  LIST_ENTRY          *ParamPackage;
  UINT64              Intermediate;

  //
  // Parse the parameters.
  //
  Status = ShellCommandLineParseEx (ParamList, &ParamPackage, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gHiiHandle, L"HttpTest", ProblemParam);
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-v")) {
    Private->Verbose = TRUE;
  }

  //
  // Parse the parameter of source ip address.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-s")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-s");
    if (ValueStr != NULL) {
      Private->EndPoint = AllocateCopyPool (StrSize (ValueStr), ValueStr);
      if (Private->EndPoint == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gHiiHandle, L"HttpTest");
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
    }
  }
  Status = EFI_SUCCESS;
  //
  // Parse the delay parameter.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-d")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-d");
    if (ValueStr != NULL) {
      Status = ShellConvertStringToUint64(ValueStr, &Intermediate, FALSE, TRUE);
      if (EFI_ERROR (Status)) {
       ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gHiiHandle, L"HttpTest");
       Status = EFI_OUT_OF_RESOURCES;
       goto ON_EXIT;
      }
      Private->Delay = (UINTN)Intermediate;
    }
  }
  Status = EFI_SUCCESS;
ON_EXIT:
  ShellCommandLineFreeVarList (ParamPackage);
  return Status;
}

VOID
Cleanup (
  IN NETAGENT_PRIVATE_DATA  *Private
  )
{
  ASSERT (Private != NULL);

  if (Private->EndPoint != NULL) {
    FreePool (Private->EndPoint);
  }

  if (Private->Buffer) {
    FreePool (Private->Buffer);
  }
  ZeroMem (Private, sizeof (NETAGENT_PRIVATE_DATA));
}

/**
  The entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  NETAGENT_PRIVATE_DATA           Private;
  EFI_HANDLE                      *Handles;
  UINTN                           HandleCount;
  LOAD_KEY_PROTOCOL              *LoadKey;
  UINT64                          Temp;
  UINT64                          Start;

  ZeroMem (&Private, sizeof (NETAGENT_PRIVATE_DATA));

  gHiiHandle = InitializeHiiPackage(ImageHandle);
  if (gHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }
  do {
    //
    // Clear the screen
    //
    Status = gST->ConOut->ClearScreen(gST->ConOut);
    if (EFI_ERROR(Status)) {
      break;
    }

    //
    // Check the command line
    //
    Status = ProcessCommandLine (&Private);
    if (EFI_ERROR (Status)) {
      break;
    }

    Temp = GetPerformanceCounterProperties(&Start,NULL);
    DEBUG((DEBUG_INFO, "%a:%d:  GetPerformanceCounterProperties Start 0x%x Result 0x%x\n", __func__, __LINE__, Start, Temp));
    Temp = GetPerformanceCounter(); 
    DEBUG((DEBUG_INFO, "%a:%d:  GetPerformanceCounter  Result 0x%x\n", __func__, __LINE__, Temp));

    if (Private.Delay) {
      DEBUG((DEBUG_INFO, "%a:%d: Delaying for %d\n", __func__, __LINE__, Private.Delay));
      MicroSecondDelay (Private.Delay);
      DEBUG((DEBUG_INFO, "%a:%d: Delaying done\n", __func__, __LINE__));
    }
    Temp = GetPerformanceCounter(); 
    DEBUG((DEBUG_INFO, "%a:%d:  GetPerformanceCounter  Result 0x%x\n", __func__, __LINE__, Temp));

    //
    // Use wide match algorithm to find one when
    //  cannot find a LoadKey instance to exactly match the FilePath
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gLoadKeyProtocolGuid,
                    NULL,
                    &HandleCount,
                    &Handles
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    DEBUG((DEBUG_INFO, "%a:%d: gTdvfLoadKeyProtocolGuid  handlecnt %d %p\n", __func__, __LINE__,
      HandleCount, Handles));

    Status = gBS->OpenProtocol (
                  Handles[0],
                  &gLoadKeyProtocolGuid,
                  (VOID **) &LoadKey,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
    if (EFI_ERROR (Status)) {
      break;
    }


    if (Private.EndPoint) {
      Private.BufferSize = 1024;
      //Status = LoadKey->LoadKey (LoadKey, Private.EndPoint, &Private.BufferSize, FileBuffer);
      Status = EFI_BUFFER_TOO_SMALL;
      if (Status == EFI_BUFFER_TOO_SMALL) {
        DEBUG((DEBUG_INFO, "%d: BufferSize %d\n", __LINE__, Private.BufferSize));
        Private.Buffer = AllocatePool(Private.BufferSize);
      }
      if (Private.Buffer != NULL) {
        Status = LoadKey->LoadKey (LoadKey, Private.EndPoint, &Private.BufferSize, Private.Buffer);
        if (!EFI_ERROR(Status)) {
          DEBUG((DEBUG_INFO, "%a:%d data %a\n", __func__, __LINE__, Private.Buffer));
        }
      } else {
        Status = EFI_OUT_OF_RESOURCES;
      }
    }

  } while (FALSE);

   gBS->CloseProtocol (
                  Handles[0],
                  &gLoadKeyProtocolGuid,
                  gImageHandle,
                  NULL
           );


  Cleanup(&Private);

  HiiRemovePackages (gHiiHandle);
  return (Status);
}
