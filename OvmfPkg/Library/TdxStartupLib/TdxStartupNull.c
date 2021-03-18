#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TdxStartupLib.h>

VOID
EFIAPI
TdxStartup(
  IN VOID                           * Context,
  IN VOID                           * VmmHobList,
  IN UINTN                          Info,
  IN fProcessLibraryConstructorList Function
  )
{
  ASSERT (FALSE);
  CpuDeadLoop ();
}

