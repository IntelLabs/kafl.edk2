/** @file
  Library that helps implement monolithic PEI. (SEC goes to DXE)

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PRE_PI_LIB_TDX_H__
#define __PRE_PI_LIB_TDX_H__

/**
  This service enables discovery of additional firmware volumes.

  @param  Instance              This instance of the firmware volume to find.  The value 0 is the
                                Boot Firmware Volume (BFV).
  @param  FwVolHeader           Pointer to the firmware volume header of the volume to return.

  @retval EFI_SUCCESS           The volume was found.
  @retval EFI_NOT_FOUND         The volume was not found.
  @retval EFI_INVALID_PARAMETER FwVolHeader is NULL.

**/
EFI_STATUS
EFIAPI
FfsFindNextVolume (
  IN UINTN                          Instance,
  IN OUT EFI_PEI_FV_HANDLE          *VolumeHandle
  );


/**
  This service enables discovery of additional firmware files.

  @param  SearchType            A filter to find files only of this type.
  @param  FwVolHeader           Pointer to the firmware volume header of the volume to search.
                                This parameter must point to a valid FFS volume.
  @param  FileHeader            Pointer to the current file from which to begin searching.

  @retval EFI_SUCCESS           The file was found.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_NOT_FOUND         The header checksum was not zero.

**/
EFI_STATUS
EFIAPI
FfsFindNextFile (
  IN EFI_FV_FILETYPE            SearchType,
  IN EFI_PEI_FV_HANDLE          VolumeHandle,
  IN OUT EFI_PEI_FILE_HANDLE    *FileHandle
  );


/**
  This service enables discovery sections of a given type within a valid FFS file.

  @param  SearchType            The value of the section type to find.
  @param  FfsFileHeader         A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
EFIAPI
FfsFindSectionData (
  IN EFI_SECTION_TYPE           SectionType,
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  OUT VOID                      **SectionData
  );


/**
  Find a file in the volume by name

  @param FileName       A pointer to the name of the file to
                        find within the firmware volume.

  @param VolumeHandle   The firmware volume to search FileHandle
                        Upon exit, points to the found file's
                        handle or NULL if it could not be found.

  @retval EFI_SUCCESS             File was found.

  @retval EFI_NOT_FOUND           File was not found.

  @retval EFI_INVALID_PARAMETER   VolumeHandle or FileHandle or
                                  FileName was NULL.

**/
EFI_STATUS
EFIAPI
FfsFindFileByName (
  IN CONST  EFI_GUID            *FileName,
  IN CONST  EFI_PEI_FV_HANDLE   VolumeHandle,
  OUT       EFI_PEI_FILE_HANDLE *FileHandle
  );


/**
  Get information about the file by name.

  @param FileHandle   Handle of the file.

  @param FileInfo     Upon exit, points to the file's
                      information.

  @retval EFI_SUCCESS             File information returned.

  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.

  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
FfsGetFileInfo (
  IN CONST  EFI_PEI_FILE_HANDLE   FileHandle,
  OUT EFI_FV_FILE_INFO            *FileInfo
  );


/**
  Get Information about the volume by name

  @param VolumeHandle   Handle of the volume.

  @param VolumeInfo     Upon exit, points to the volume's
                        information.

  @retval EFI_SUCCESS             File information returned.

  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.

  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
FfsGetVolumeInfo (
  IN  EFI_PEI_FV_HANDLE       VolumeHandle,
  OUT EFI_FV_INFO             *VolumeInfo
  );



/**
  Get Fv image from the FV type file, then add FV & FV2 Hob.

  @param FileHandle      File handle of a Fv type file.

  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully to process it.

**/
EFI_STATUS
EFIAPI
FfsProcessFvFile (
  IN  EFI_PEI_FILE_HANDLE   FvFileHandle
  );


/**
  Search through every FV until you find a file of type FileType

  @param FileType        File handle of a Fv type file.
  @param Volumehandle    On success Volume Handle of the match
  @param FileHandle      On success File Handle of the match

  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully found FileType

**/
EFI_STATUS
EFIAPI
FfsAnyFvFindFirstFile (
  IN  EFI_FV_FILETYPE       FileType,
  OUT EFI_PEI_FV_HANDLE     *VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  );

EFI_STATUS
EFIAPI
FfsAnyFvFindFileByName (
  IN  CONST EFI_GUID        *Name,
  OUT EFI_PEI_FV_HANDLE     *VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  );


/**
  Get Fv image from the FV type file, then add FV & FV2 Hob.

  @param FileHandle  File handle of a Fv type file.


  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully to process it.

**/
EFI_STATUS
EFIAPI
FfsProcessFvFile (
  IN  EFI_PEI_FILE_HANDLE   FvFileHandle
  );

EFI_STATUS
EFIAPI
LoadPeCoffImage (
  IN  VOID                                      *PeCoffImage,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );

EFI_STATUS
EFIAPI
LoadDxeCoreFromFfsFile (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  IN UINTN                StackSize
  );

EFI_STATUS
EFIAPI
LoadDxeCoreFromFv (
  IN UINTN  *FvInstance,   OPTIONAL
  IN UINTN  StackSize
  );

EFI_STATUS
EFIAPI
DecompressFirstFv (
  VOID
  );

#endif
