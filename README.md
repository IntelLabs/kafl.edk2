
# TDVF OVERVIEW

<b>Intel Trust Domain Extension (TDX)</b> is Intel Architecture extension to provide trusted, isolated VM execution by removing CSP software (hypervisor etc) from the TCB. <b>TDX Virtual Firmware (TDVF)</b> is an EDK II based project to enable UEFI support for TDX based Virtual Machines. It provides the capability to launch a TD.

The <b>Intel® TDX Virtual Firmware Design Guide</b> is at https://software.intel.com/content/dam/develop/external/us/en/documents/tdx-virtual-firmware-design-guide-rev-1.pdf.

More information can be found at:
https://software.intel.com/content/www/us/en/develop/articles/intel-trust-domain-extensions.html

The staging is at https://github.com/tianocore/edk2-staging/tree/TDVF

## Branch Description
Code before the tag of tdvf_poc_ww17.5 is a standalone package. It is deprecated.

Code after the tag of tdvf_poc_ww17.5 is of TDVF One Binary, which means the Tdvf features are merged into OvmfPkg. Its binary name is OVMF.fd. It can be run on both Td and Non-Td guest.

This branch owner: Jiewen Yao <jiewen.yao@intel.com>, Min Xu <min.m.xu@intel.com>

## STATUS

Current capabilities:
* Support X64 architecture only
* Boot with QEMU/KVM and Linux Guest only

## BUILDING TDVF

Pre-requisites:
* Setup EDK2 Build environment

Following the edk2 build process, you will find the TDVF binaries under the $WORKSPACE/Build/OvmfX64/*/FV directory. The actual path will depend on how your build is configured. You can expect to find these binary outputs:
* OVMF.fd
  - This is the TDVF image without Secure Boot variables enrolled.

For example, to build TDVF (in Linux):  
  `$ cd edk2`

  `$ source edksetup.sh`

  `$ build -p OvmfPkg/OvmfPkgX64.dsc -t GCC5 -a X64`  

## Secure Boot

TDVF uses UEFI Secure Boot as base with extensions for TD launch.
Follow below steps to enroll the Secure Boot variables in build scripts.
  * Generete/download the files: 
    - PK: It should be generated at the discretion of the platform owner (OEM).
      For more information, visit: https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/windows-secure-boot-key-creation-and-management-guidance
    - KEK: http://www.microsoft.com/pkiops/certs/MicCorKEKCA2011_2011-06-24.crt  
    - Windows DB: http://www.microsoft.com/pkiops/certs/MicWinProPCA2011_2011-10-19.crt  
    - UEFI DB: http://www.microsoft.com/pkiops/certs/MicCorUEFCA2011_2011-06-27.crt 
    - Signature GUID for all the above keys: 77fa9abd-0359-4d32-bd60-28f4e78f784b
    - UEFI DBX: Please refer to https://uefi.org/revocationlistfile 
    - SecureBootEnable: binary file with 1 byte (0 for disable and 1 for enable)

## RUNNING TDVF on QEMU

* QEMU commands
`$ /path/to/install_qemu/qemu-system-x86_64 \`  
`  -no-reboot -name debug-threads=on -enable-kvm -smp 4,sockets=2 -object tdx-guest,id=tdx,debug \`  
`  -machine q35,accel=kvm,kvm-type=tdx,kernel_irqchip=split,guest-memory-protection=tdx -no-hpet \`  
`  -cpu host,host-phys-bits,+invtsc \`  
`  -device loader,file=/path/to/tdvf,id=fd0 \`  
`  -m 2G -nographic -vga none`

Running a TD requires not only a TDVF but also an updated QEMU/KVM/LinuxGuest. More information visit:  https://github.com/intel/tdx. For example:
  - The early code of Linux Guest support is at https://github.com/intel/tdx/tree/guest.
  - The early code of KVM support is at https://github.com/intel/tdx/tree/kvm.
  - The early code of QEMU support is at TBD.


## Known limitation
This package is only the sample code to show the concept.
It does not have a full validation such as robustness functional test and fuzzing test. It does not meet the production quality yet.
Any codes including the API definition, the libary and the drivers are subject to change.


