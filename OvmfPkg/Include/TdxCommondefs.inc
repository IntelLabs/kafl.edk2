;------------------------------------------------------------------------------
; @file
; TDX Common defitions used by the APs in mailbox
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

CommandOffset                             equ       00h
ApicidOffset                              equ       04h
WakeupVectorOffset                        equ       08h
OSArgsOffset                              equ       10h
FirmwareArgsOffset                        equ       800h
WakeupArgsRelocatedMailBox                equ       800h
AcceptPageArgsPhysicalStart               equ       800h
AcceptPageArgsPhysicalEnd                 equ       808h
AcceptPageArgsAcceptSize                  equ       810h
;AcceptPageArgsTallies                     equ       818h
AcceptPageArgsPageSize                    equ       818h
CpuArrivalOffset                          equ       900h
CpusExitingOffset                         equ       0a00h
TalliesOffset                             equ       0a08h

SIZE_4KB                                  equ       1000h
SIZE_2MB                                  equ       200000h
SIZE_1GB                                  equ       40000000h

MpProtectedModeWakeupCommandNoop          equ       0
MpProtectedModeWakeupCommandWakeup        equ       1
MpProtectedModeWakeupCommandSleep         equ       2
MpProtectedModeWakeupCommandAcceptPages   equ       3

MailboxApicIdInvalid                      equ       0xffffffff
MailboxApicidBroadcast                    equ       0xfffffffe

%macro simple_spinlock 3
    mov        edx, %1
    mov        eax, 0
    mov        ebx, 1
%%testlock:
    lock       cmpxchg [edx], ebx
    jnz        %3
    mov        eax, 0
    mov        ebx, 1
    lock       cmpxchg [edx+4], ebx
    jnz        %2
%%firstone:
    pause
%endmacro

%macro simple_releaselock 3
%2:
    mov        eax, 1
    mov        edx, %1
    jmp        %%testlock
%3:
    pause
    mov        eax, 0
%%testlock:
    mov        ebx, 0
    lock       cmpxchg [edx], ebx
    jnz        %3
%endmacro

%define EFI_HOB_TYPE_HANDOFF              0x0001
%define EFI_HOB_TYPE_MEMORY_ALLOCATION    0x0002
%define EFI_HOB_TYPE_RESOURCE_DESCRIPTOR  0x0003
%define EFI_HOB_TYPE_GUID_EXTENSION       0x0004
%define EFI_HOB_TYPE_FV                   0x0005
%define EFI_HOB_TYPE_CPU                  0x0006
%define EFI_HOB_TYPE_MEMORY_POOL          0x0007
%define EFI_HOB_TYPE_FV2                  0x0009
%define EFI_HOB_TYPE_LOAD_PEIM_UNUSED     0x000A
%define EFI_HOB_TYPE_UEFI_CAPSULE         0x000B
%define EFI_HOB_TYPE_FV3                  0x000C
%define EFI_HOB_TYPE_UNUSED               0xFFFE
%define EFI_HOB_TYPE_END_OF_HOB_LIST      0xFFFF

%define EFI_RESOURCE_SYSTEM_MEMORY          0x00000000
%define EFI_RESOURCE_MEMORY_MAPPED_IO       0x00000001
%define EFI_RESOURCE_IO                     0x00000002
%define EFI_RESOURCE_FIRMWARE_DEVICE        0x00000003
%define EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  0x00000004
%define EFI_RESOURCE_MEMORY_RESERVED        0x00000005
%define EFI_RESOURCE_IO_RESERVED            0x00000006
%define EFI_RESOURCE_MAX_MEMORY_TYPE        0x00000007

%define EFI_RESOURCE_ATTRIBUTE_PRESENT                  0x00000001
%define EFI_RESOURCE_ATTRIBUTE_INITIALIZED              0x00000002
%define EFI_RESOURCE_ATTRIBUTE_TESTED                   0x00000004
%define EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED           0x00000080

%define EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE              0x00000400
%define EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE        0x00000800
%define EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE  0x00001000
%define EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE     0x00002000


%define EFI_IO_ATTR (EFI_RESOURCE_ATTRIBUTE_PRESENT + \
                          EFI_RESOURCE_ATTRIBUTE_INITIALIZED + \
                          EFI_RESOURCE_ATTRIBUTE_TESTED + \
                          EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE)

%define EFI_LOW_MEM_ATTR (EFI_RESOURCE_ATTRIBUTE_PRESENT + \
                          EFI_RESOURCE_ATTRIBUTE_INITIALIZED + \
                          EFI_RESOURCE_ATTRIBUTE_TESTED + \
                          EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE + \
                          EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE + \
                          EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE + \
                          EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE)

%define TDCALL_TDINFO                          0x1
%define TDCALL_TDACCEPTPAGE                    0x6
