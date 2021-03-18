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
#pragma pack()
#endif
