/** @file
  Load Key protocol

Copyright (c) 2006 - 20120, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LOAD_KEY_PROTOCOL_H__
#define __LOAD_KEY_PROTOCOL_H__

#define LOAD_KEY_PROTOCOL_GUID \
  { \
    0xCF83FB30, 0xF2BF, 0x11EA, {0x8B, 0x6E, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66 } \
  }

typedef struct _LOAD_KEY_PROTOCOL LOAD_KEY_PROTOCOL;

/**
  Causes the driver to load a specified key.

  @param  This       Protocol instance pointer.
  @param  KeyPath    The device specific path of the key to load.
  @param  BufferSize On input the size of Buffer in bytes. On output with a return
                     code of EFI_SUCCESS, the amount of data transferred to
                     Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                     the size of Buffer required to retrieve the requested key.
  @param  Buffer     The memory buffer to transfer the key to. IF Buffer is NULL,
                     then the size of the requested key is returned in
                     BufferSize.

  @retval EFI_SUCCESS           The key was loaded.
  @retval EFI_INVALID_PARAMETER KeyPath is not a valid device path, or
                                BufferSize is NULL.
  @retval EFI_DEVICE_ERROR      The key was not loaded due to a device error.
  @retval EFI_NO_RESPONSE       The remote system did not respond.
  @retval EFI_NOT_FOUND         The key was not found.
  @retval EFI_ABORTED           The key load process was manually cancelled.
**/
typedef
EFI_STATUS
(EFIAPI *LOAD_KEY)(
  IN LOAD_KEY_PROTOCOL           *This,
  IN CHAR16         *KeyPath,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer OPTIONAL
  );

///
/// The LOAD_KEY_PROTOCOL is a simple protocol used to obtain files from arbitrary devices.
///
struct _LOAD_KEY_PROTOCOL {
  LOAD_KEY LoadKey;
};

extern EFI_GUID gLoadKeyProtocolGuid;

#endif
