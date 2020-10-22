/** @file
  Processor or Compiler specific defines and types x64 (Intel 64, AMD64).

  Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _ACPI_TDX_H_
#define _ACPI_TDX_H_

#define ACPI_MADT_MPWK_STRUCT_TYPE  0x10

#pragma pack(1)

typedef struct {
  UINT8                       Type;
  UINT8                       Length;
  UINT16                      MailBoxVersion;
  UINT32                      Reserved2;
  UINT64                      MailBoxAddress;
} ACPI_MADT_MPWK_STRUCT;

typedef struct{
  UINT16                      KeyType;
  UINT16                      KeyFormat;
  UINT32                      KeySize;
  UINT64                      KeyAddress;
}EFI_TDX_STORAGE_VOLUME_KEY_STRUCT;

typedef struct{
  EFI_ACPI_DESCRIPTION_HEADER Header;
  UINT32                      KeyCount;
  //EFI_TDX_STORAGE_VOLUME_KEY_STRUCT Keys[];
}EFI_TDX_STORAGE_VOLUME_KEY_ACPI_TABLE;
#pragma pack()

#define EFI_TDX_STORAGE_VOLUME_KEY_ACPI_TABLE_SIGNATURE   SIGNATURE_32('S', 'V', 'K', 'L')
#define EFI_TDX_STORAGE_VOLUME_KEY_ACPI_TABLE_REVISION    1

#endif

