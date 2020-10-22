/** @file
  The implementation of EFI_LOAD_FILE_PROTOCOL for UEFI HTTP key.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpKeyDxe.h"

/**
  Install HTTP  Callback Protocol if not installed before.

  @param[in] Private           Pointer to HTTP private data.

  @retval EFI_SUCCESS          HTTP Callback Protocol installed successfully.
  @retval Others               Failed to install HTTP Callback Protocol.

**/
EFI_STATUS
HttpKeyInstallCallback (
  IN HTTP_KEY_PRIVATE_DATA           *Private
  )
{
  EFI_STATUS                  Status;
  EFI_HANDLE                  ControllerHandle;

  if (!Private->UsingIpv6) {
    ControllerHandle = Private->Ip4Nic->Controller;
  } else {
    ControllerHandle = Private->Ip6Nic->Controller;
  }

  //
  // Check whether gHttpKeyCallbackProtocolGuid already installed.
  //
  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gHttpKeyCallbackProtocolGuid,
                  (VOID **) &Private->HttpKeyCallback
                  );
  if (Status == EFI_UNSUPPORTED) {

    CopyMem (
      &Private->LoadKeyCallback,
      &gHttpKeyDxeHttpKeyCallback,
      sizeof (HTTP_KEY_CALLBACK_PROTOCOL)
      );

    //
    // Install a default callback if user didn't offer one.
    //
    Status = gBS->InstallProtocolInterface (
                    &ControllerHandle,
                    &gHttpKeyCallbackProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Private->LoadKeyCallback
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Private->HttpKeyCallback = &Private->LoadKeyCallback;
  }

  return EFI_SUCCESS;
}

/**
  Uninstall HTTP Callback Protocol if it's installed by this driver.

  @param[in] Private           Pointer to HTTP private data.

**/
VOID
HttpKeyUninstallCallback (
  IN HTTP_KEY_PRIVATE_DATA           *Private
  )
{
  if (Private->HttpKeyCallback == &Private->LoadKeyCallback) {
    gBS->UninstallProtocolInterface (
          Private->Controller,
          &gHttpKeyCallbackProtocolGuid,
          &Private->HttpKeyCallback
          );
    Private->HttpKeyCallback = NULL;
  }
}

/**
  Enable the use of UEFI HTTP key function.

  If the driver has already been started but not satisfy the requirement (IP stack and
  specified key file path), this function will stop the driver and start it again.

  @param[in]    Private            The pointer to the driver's private data.
  @param[in]    UsingIpv6          Specifies the type of IP addresses that are to be
                                   used during the session that is being started.
                                   Set to TRUE for IPv6, and FALSE for IPv4.
  @param[in]    KeyPath           The device specific path of the file to load.

  @retval EFI_SUCCESS              HTTP key was successfully enabled.
  @retval EFI_INVALID_PARAMETER    Private is NULL or KeyPath is NULL.
  @retval EFI_INVALID_PARAMETER    The KeyPath doesn't contain a valid URI device path node.
  @retval EFI_ALREADY_STARTED      The driver is already in started state.
  @retval EFI_OUT_OF_RESOURCES     There are not enough resources.

**/
EFI_STATUS
HttpKeyStart (
  IN HTTP_KEY_PRIVATE_DATA           *Private,
  IN BOOLEAN                          UsingIpv6,
  IN CHAR16         *KeyPath
  )
{
  UINTN                Index;
  EFI_STATUS           Status;
  CHAR8                *Uri;

  Uri = NULL;

  if (Private == NULL || KeyPath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the URI in the input KeyPath
  //
  Status = HttpKeyParseKeyPath (KeyPath, &Uri);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether we need to stop and restart the HTTP key driver.
  //
  if (Private->Started) {
    //
    // Restart is needed in 2 cases:
    // 1. Http key driver has already been started but not on the required IP stack.
    // 2. The specified key file URI in KeyPath is different with the one we have
    // recorded before.
    //
    if ((UsingIpv6 != Private->UsingIpv6) ||
        ((Uri != NULL) && (AsciiStrCmp (Private->KeyUri, Uri) != 0))) {
      //
      // Restart is required, first stop then continue this start function.
      //
      Status = HttpKeyStop (Private);
      if (EFI_ERROR (Status)) {
        if (Uri != NULL) {
          FreePool (Uri);
        }
        return Status;
      }
    } else {
      //
      // Restart is not required.
      //
      if (Uri != NULL) {
        FreePool (Uri);
      }
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Detect whether using ipv6 or not, and set it to the private data.
  //
  if (UsingIpv6 && Private->Ip6Nic != NULL) {
    Private->UsingIpv6 = TRUE;
  } else if (!UsingIpv6 && Private->Ip4Nic != NULL) {
    Private->UsingIpv6 = FALSE;
  } else {
    if (Uri != NULL) {
      FreePool (Uri);
    }
    return EFI_UNSUPPORTED;
  }

  //
  // Record the specified URI and prepare the URI parser if needed.
  //
  Private->KeyPathUri = Uri;
  if (Private->KeyPathUri != NULL) {
    Status = HttpParseUrl (
               Private->KeyPathUri,
               (UINT32) AsciiStrLen (Private->KeyPathUri),
               FALSE,
               &Private->KeyPathUriParser
               );
    if (EFI_ERROR (Status)) {
      FreePool (Private->KeyPathUri);
      return Status;
    }
  }

  //
  // Init the content of cached DHCP offer list.
  //
  ZeroMem (Private->OfferBuffer, sizeof (Private->OfferBuffer));
  if (!Private->UsingIpv6) {
    for (Index = 0; Index < HTTP_KEY_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp4.Packet.Offer.Size = HTTP_CACHED_DHCP4_PACKET_MAX_SIZE;
    }
  } else {
    for (Index = 0; Index < HTTP_KEY_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp6.Packet.Offer.Size = HTTP_CACHED_DHCP6_PACKET_MAX_SIZE;
    }
  }

  if (Private->UsingIpv6) {
    //
    // Set Ip6 policy to Automatic to start the Ip6 router discovery.
    //
    Status = HttpKeySetIp6Policy (Private);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  Private->Started   = TRUE;
  DEBUG((DEBUG_VERBOSE, "Start connection over IPv%d", Private->UsingIpv6 ? 6 : 4));

  return EFI_SUCCESS;
}

/**
  Attempt to complete a DHCPv4 D.O.R.A or DHCPv6 S.R.A.A sequence

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              DHCP successfully retrieved.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval EFI_NOT_STARTED          The driver is in stopped state.
  @retval EFI_DEVICE_ERROR         An unexpected network error occurred.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
HttpKeyDhcp (
  IN HTTP_KEY_PRIVATE_DATA           *Private
  )
{
  EFI_STATUS                Status;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  Status = EFI_DEVICE_ERROR;

  if (!Private->UsingIpv6) {
    //
    // Start D.O.R.A process to get a IPv4 address
    //
    Status = HttpKeyDhcp4Dora (Private);
  } else {
     //
    // Start S.A.R.R process to get a IPv6 address
    //
    Status = HttpKeyDhcp6Sarr (Private);
  }

  return Status;
}

/**
  Attempt to download the key file through HTTP message exchange.

  @param[in]          Private         The pointer to the driver's private data.
  @param[in, out]     BufferSize      On input the size of Buffer in bytes. On output with a return
                                      code of EFI_SUCCESS, the amount of data transferred to
                                      Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of Buffer required to retrieve the requested file.
  @param[in]          Buffer          The memory buffer to transfer the file to. If Buffer is NULL,
                                      then the size of the requested file is returned in
                                      BufferSize.
  @retval EFI_SUCCESS                 Boot file was loaded successfully.
  @retval EFI_INVALID_PARAMETER       Private is NULL, or BufferSize is NULL.
  @retval EFI_INVALID_PARAMETER       *BufferSize is not zero, and Buffer is NULL.
  @retval EFI_NOT_STARTED             The driver is in stopped state.
  @retval EFI_BUFFER_TOO_SMALL        The BufferSize is too small to read the key file. BufferSize has
                                      been updated with the size needed to complete the request.
  @retval EFI_DEVICE_ERROR            An unexpected network error occurred.
  @retval Others                      Other errors as indicated.

**/
EFI_STATUS
HttpKeyLoadKey (
  IN     HTTP_KEY_PRIVATE_DATA       *Private,
  IN OUT UINTN                        *BufferSize,
  IN     VOID                         *Buffer       OPTIONAL
  )
{
  EFI_STATUS             Status;

  if (Private == NULL || BufferSize == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  if (*BufferSize != 0 && Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  Status = HttpKeyInstallCallback (Private);
  if (EFI_ERROR(Status)) {
    goto ON_EXIT;
  }

  if (Private->KeyUri == NULL) {
    //
    // Use DNS to parse the key url
    //
    Status = HttpKeyDiscoverUrlInfo (Private);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_VERBOSE, "Error: Could not parse key url\n"));
      goto ON_EXIT;
    }
  }

  if (!Private->HttpCreated) {
    //
    // Create HTTP child.
    //
    Status = HttpKeyCreateHttpIo (Private);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  if (Private->KeySize == 0) {
    //
    // Discover the information about the key if we haven't.
    //

    //
    // Try to use HTTP HEAD method.
    //
    Status = HttpKeyGetKey(
               Private,
               HttpMethodHead,
               TRUE,
               &Private->KeySize,
               NULL
               );
    if (EFI_ERROR (Status) && ((Status != EFI_BUFFER_TOO_SMALL) &&
      (Status != EFI_ABORTED))) {
      //
      // Failed to get file size by HEAD method, may be trunked encoding, try HTTP GET method.
      //
      ASSERT (Private->KeySize == 0);
      Status = HttpKeyGetKey(
                 Private,
                 HttpMethodGet, 
                 TRUE,
                 &Private->KeySize,
                 NULL
                 );
      if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
        DEBUG((DEBUG_VERBOSE, "\n  Error: Could not retrieve file size from HTTP server.\n"));
        goto ON_EXIT;
      }
    }
  }

  if (*BufferSize < Private->KeySize) {
    *BufferSize = Private->KeySize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto ON_EXIT;
  }

  if (!EFI_ERROR (Status) || (Status == EFI_BUFFER_TOO_SMALL)) {
    //
    // Load the key file into Buffer
    //
    Status = HttpKeyGetKey (
             Private,
             HttpMethodGet, 
             FALSE,
             BufferSize,
             Buffer
             );
  }

ON_EXIT:
  HttpKeyUninstallCallback (Private);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_ACCESS_DENIED) {
      DEBUG((DEBUG_VERBOSE, "Error: Could not establish connection with HTTP server.\n"));
    } else if (Status == EFI_BUFFER_TOO_SMALL && Buffer != NULL) {
      DEBUG((DEBUG_VERBOSE, "Error: Buffer size is smaller than the requested file.\n"));
    } else if (Status == EFI_OUT_OF_RESOURCES) {
      DEBUG((DEBUG_VERBOSE, "Error: Could not allocate I/O buffers.\n"));
    } else if (Status == EFI_DEVICE_ERROR) {
      DEBUG((DEBUG_VERBOSE, "Error: Network device error.\n"));
    } else if (Status == EFI_TIMEOUT) {
      DEBUG((DEBUG_VERBOSE, "Error: Server response timeout.\n"));
    } else if (Status == EFI_ABORTED) {
      DEBUG((DEBUG_VERBOSE, "Error: Remote key cancelled.\n"));
    } else if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG((DEBUG_VERBOSE, "Error: Unexpected network error.\n"));
    }
  }

  return Status;
}

/**
  Disable the use of UEFI HTTP key function.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              HTTP key was successfully disabled.
  @retval EFI_NOT_STARTED          The driver is already in stopped state.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval Others                   Unexpected error when stop the function.

**/
EFI_STATUS
HttpKeyStop (
  IN HTTP_KEY_PRIVATE_DATA           *Private
  )
{
  UINTN            Index;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  if (Private->HttpCreated) {
    HttpIoDestroyIo (&Private->HttpIo);
    Private->HttpCreated = FALSE;
  }

  Private->Started = FALSE;
  ZeroMem (&Private->StationIp, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->SubnetMask, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->GatewayIp, sizeof (EFI_IP_ADDRESS));
  Private->Port = 0;
  Private->KeyUri = NULL;
  Private->KeyUriParser = NULL;
  Private->KeySize = 0;
  Private->SelectIndex = 0;
  Private->SelectProxyType = HttpOfferTypeMax;

  if (!Private->UsingIpv6) {
    //
    // Stop and release the DHCP4 child.
    //
    Private->Dhcp4->Stop (Private->Dhcp4);
    Private->Dhcp4->Configure (Private->Dhcp4, NULL);

    for (Index = 0; Index < HTTP_KEY_OFFER_MAX_NUM; Index++) {
      if (Private->OfferBuffer[Index].Dhcp4.UriParser) {
        HttpUrlFreeParser (Private->OfferBuffer[Index].Dhcp4.UriParser);
      }
    }
  } else {
    //
    // Stop and release the DHCP6 child.
    //
    Private->Dhcp6->Stop (Private->Dhcp6);
    Private->Dhcp6->Configure (Private->Dhcp6, NULL);

    for (Index = 0; Index < HTTP_KEY_OFFER_MAX_NUM; Index++) {
      if (Private->OfferBuffer[Index].Dhcp6.UriParser) {
        HttpUrlFreeParser (Private->OfferBuffer[Index].Dhcp6.UriParser);
      }
    }
  }

  if (Private->DnsServerIp != NULL) {
    FreePool (Private->DnsServerIp);
    Private->DnsServerIp = NULL;
  }

  if (Private->KeyPathUri!= NULL) {
    FreePool (Private->KeyPathUri);
    HttpUrlFreeParser (Private->KeyPathUriParser);
    Private->KeyPathUri = NULL;
    Private->KeyPathUriParser = NULL;
  }

  ZeroMem (Private->OfferBuffer, sizeof (Private->OfferBuffer));
  Private->OfferNum = 0;
  ZeroMem (Private->OfferCount, sizeof (Private->OfferCount));
  ZeroMem (Private->OfferIndex, sizeof (Private->OfferIndex));

  return EFI_SUCCESS;
}

/**
  Causes the driver to load a specified file.

  @param  This       Protocol instance pointer.
  @param  KeyPath   The device specific path of the file to load.
  @param  BufferSize On input the size of Buffer in bytes. On output with a return
                     code of EFI_SUCCESS, the amount of data transferred to
                     Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                     the size of Buffer required to retrieve the requested file.
  @param  Buffer     The memory buffer to transfer the file to. IF Buffer is NULL,
                     then the size of the requested file is returned in
                     BufferSize.

  @retval EFI_SUCCESS           The file was loaded.
  @retval EFI_UNSUPPORTED       The device does not support the provided BootPolicy
  @retval EFI_INVALID_PARAMETER KeyPath is not a valid device path, or
                                BufferSize is NULL.
  @retval EFI_NO_MEDIA          No medium was present to load the file.
  @retval EFI_DEVICE_ERROR      The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE       The remote system did not respond.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_ABORTED           The file load process was manually cancelled.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to read the current directory entry.
                                BufferSize has been updated with the size needed to complete
                                the request.

**/
EFI_STATUS
EFIAPI
HttpKeyDxeLoadKey (
  IN LOAD_KEY_PROTOCOL           *This,
  IN CHAR16         *KeyPath,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer OPTIONAL
  )
{
  HTTP_KEY_PRIVATE_DATA        *Private;
  HTTP_KEY_VIRTUAL_NIC         *VirtualNic;
  EFI_STATUS                    MediaStatus;
  BOOLEAN                       UsingIpv6;
  EFI_STATUS                    Status;

  if (This == NULL || BufferSize == NULL || KeyPath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VirtualNic = HTTP_KEY_VIRTUAL_NIC_FROM_LOADKEY (This);
  Private = VirtualNic->Private;

  //
  // Check media status before HTTP start
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Private->Controller, HTTP_KEY_CHECK_MEDIA_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    DEBUG((DEBUG_VERBOSE,"Error: Could not detect network connection.\n"));
    return EFI_NO_MEDIA;
  }

  //
  // Check whether the virtual nic is using IPv6 or not.
  //
  UsingIpv6 = FALSE;
  if (VirtualNic == Private->Ip6Nic) {
    UsingIpv6 = TRUE;
  }

  //
  // Initialize HTTP.
  //
  Status = HttpKeyStart (Private, UsingIpv6, KeyPath);
  if (Status != EFI_SUCCESS && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  Status = HttpKeyLoadKey (Private, BufferSize, Buffer);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_BUFFER_TOO_SMALL) {
      HttpKeyStop (Private);
    }
    return Status;
  }

  //
  // Stop the HTTP service after the image is downloaded.
  //
  HttpKeyStop (Private);
  return Status;
}

///
/// Load File Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
LOAD_KEY_PROTOCOL  gHttpKeyDxeLoadKey = {
  HttpKeyDxeLoadKey
};

/**
  Callback function that is invoked when the HTTP Boot driver is about to transmit or has received a
  packet.

  This function is invoked when the HTTP Boot driver is about to transmit or has received packet.
  Parameters DataType and Received specify the type of event and the format of the buffer pointed
  to by Data. Due to the polling nature of UEFI device drivers, this callback function should not
  execute for more than 5 ms.
  The returned status code determines the behavior of the HTTP Boot driver.

  @param[in]  This                Pointer to the HTTP_KEY_CALLBACK_PROTOCOL instance.
  @param[in]  DataType            The event that occurs in the current state.
  @param[in]  Received            TRUE if the callback is being invoked due to a receive event.
                                  FALSE if the callback is being invoked due to a transmit event.
  @param[in]  DataLength          The length in bytes of the buffer pointed to by Data.
  @param[in]  Data                A pointer to the buffer of data, the data type is specified by
                                  DataType.

  @retval EFI_SUCCESS             Tells the HTTP Boot driver to continue the HTTP Boot process.
  @retval EFI_ABORTED             Tells the HTTP Boot driver to abort the current HTTP Boot process.
**/
EFI_STATUS
EFIAPI
HttpKeyCallback (
  IN HTTP_KEY_CALLBACK_PROTOCOL     *This,
  IN HTTP_KEY_CALLBACK_DATA_TYPE    DataType,
  IN BOOLEAN                             Received,
  IN UINT32                              DataLength,
  IN VOID                                *Data     OPTIONAL
  )
{
  EFI_HTTP_MESSAGE        *HttpMessage;
  EFI_HTTP_HEADER         *HttpHeader;
  HTTP_KEY_PRIVATE_DATA  *Private;
  UINT32                  Percentage;

  Private = HTTP_KEY_PRIVATE_DATA_FROM_CALLBACK_PROTOCOL(This);

  switch (DataType) {
  case HttpKeyDhcp4:
  case HttpKeyDhcp6:
    break;

  case HttpKeyHttpRequest:
    if (Data != NULL) {
      HttpMessage = (EFI_HTTP_MESSAGE *) Data;
      if (HttpMessage->Data.Request->Method == HttpMethodGet &&
          HttpMessage->Data.Request->Url != NULL) {
        DEBUG ((DEBUG_VERBOSE,"URI: %s\n", HttpMessage->Data.Request->Url));
      }
    }
    break;

  case HttpKeyHttpResponse:
    if (Data != NULL) {
      HttpMessage = (EFI_HTTP_MESSAGE *) Data;

      if (HttpMessage->Data.Response != NULL) {
        if (HttpKeyIsHttpRedirectStatusCode (HttpMessage->Data.Response->StatusCode)) {
          //
          // Server indicates the resource has been redirected to a different URL
          // according to the section 6.4 of RFC7231 and the RFC 7538.
          // Display the redirect information on the screen.
          //
          HttpHeader = HttpFindHeader (
                 HttpMessage->HeaderCount,
                 HttpMessage->Headers,
                 HTTP_HEADER_LOCATION
                 );
          if (HttpHeader != NULL) {
            DEBUG((DEBUG_VERBOSE, "HTTP ERROR: Resource Redirected.\n  New Location: %a\n", HttpHeader->FieldValue));
          }
          break;
        }
      }

      HttpHeader = HttpFindHeader (
                     HttpMessage->HeaderCount,
                     HttpMessage->Headers,
                     HTTP_HEADER_CONTENT_LENGTH
                     );
      if (HttpHeader != NULL) {
        Private->TransferSize = AsciiStrDecimalToUintn (HttpHeader->FieldValue);
        Private->ReceivedSize = 0;
        Private->Percentage   = 0;
        DEBUG((DEBUG_VERBOSE, "%a:%d: filesize 0x%x\n", __func__, __LINE__, Private->TransferSize));
      }
    }
    break;

  case HttpKeyHttpEntityBody:
    if (DataLength != 0) {
      if (Private->TransferSize != 0) {
        Private->ReceivedSize += DataLength;
        Percentage = (UINT32) DivU64x64Remainder (MultU64x32 (Private->ReceivedSize, 100), Private->TransferSize, NULL);
        if (Private->Percentage != Percentage) {
          Private->Percentage = Percentage;
        }
      } else {
        //
        // In some case we couldn't get the file size from the HTTP header, so we
        // just print the downloaded file size.
        //
        Private->ReceivedSize += DataLength;
        DEBUG((DEBUG_VERBOSE, "Downloading...%lu Bytes", Private->ReceivedSize));
      }
    }
    break;

  default:
    break;
  };

  return EFI_SUCCESS;
}

///
/// HTTP key Callback Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
HTTP_KEY_CALLBACK_PROTOCOL  gHttpKeyDxeHttpKeyCallback = {
  HttpKeyCallback
};
