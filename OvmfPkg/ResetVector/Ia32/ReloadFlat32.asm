;------------------------------------------------------------------------------
; @file
; Transition from 16 bit real mode into 32 bit flat protected mode
;
; Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

%define SEC_DEFAULT_CR0  0x00000023
%define SEC_DEFAULT_CR4  0x640

BITS    32

;
; Modified:  EAX, EBX
;
ReloadFlat32:

    cli
    mov     ebx, ADDR_OF(gdtr)
    lgdt    [ebx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

    jmp     LINEAR_CODE_SEL:dword ADDR_OF(jumpToFlat32BitAndLandHere)
BITS    32
jumpToFlat32BitAndLandHere:

    mov     eax, SEC_DEFAULT_CR4
    mov     cr4, eax

    debugShowPostCode POSTCODE_32BIT_MODE

    mov     ax, LINEAR_SEL
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    OneTimeCallRet ReloadFlat32