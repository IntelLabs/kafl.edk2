;------------------------------------------------------------------------------
; @file
;   Initialize TDX_WORK_AREA
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS 32

;
; Modified:  EBX, ECX, EDX, EBP, EDI, ESP
;
; @param[in,out]  RAX/EAX  0
; @param[in]      RFLAGS   2
; @param[in]      RCX      [31:0] TDINITVP - Untrusted Configuration
;                          [63:32] 0
; @param[in]      RDX      [31:0] VCPUID
;                          [63:32] 0
; @param[in]      RBX      [6:0] CPU supported GPA width
;                          [7:7] 5 level page table support
;                          [63:8] 0
; @param[in]      RSI      [31:0] VCPU_Index
;                          [63:32] 0
; @param[in]      RDI/EDI  0
; @param[in]      RBP/EBP  0
; @param[in]      R8       Same as RCX
; @param[out]     RBP/EBP  Address of Boot Firmware Volume (BFV)
; @param[out]     DS       Selector allowing flat access to all addresses
; @param[out]     ES       Selector allowing flat access to all addresses
; @param[out]     FS       Selector allowing flat access to all addresses
; @param[out]     GS       Selector allowing flat access to all addresses
; @param[out]     SS       Selector allowing flat access to all addresses
;
; @return         None  This routine jumps to SEC and does not return

InitTdx:
    ;
    ; In Td guest, BSP/AP shares the same entry point
    ; BSP builds up the page table, while APs shouldn't do the same task.
    ; Instead, APs just leverage the page table which is built by BSP.
    ; APs will wait until the page table is ready.
    ; In Td guest, vCPU 0 is treated as the BSP, the others are APs.
    ; ESI indicates the vCPU ID.
    ;
    cmp     esi, 0
    je      tdBspEntry

apWait:
    cmp     byte[TDX_WORK_AREA_PGTBL_READY], 0
    je      apWait
    jmp     doneTdxInit

tdBspEntry:
    ;
    ; It is of Tdx Guest
    ; Save the Tdx info in TDX_WORK_AREA to pass to SEC phase
    ;
    mov     byte[TDX_WORK_AREA], 1

    ; check 5-level paging support
    and     ebp, 0x3f
    cmp     ebp, 52
    jl      NotPageLevel5
    mov     byte[TDX_WORK_AREA_PAGELEVEL5], 1

NotPageLevel5:
    mov     DWORD[TDX_WORK_AREA_INITVP], ecx
    mov     DWORD[TDX_WORK_AREA_INFO], ebp

doneTdxInit:
    OneTimeCallRet InitTdx
