/** @file
  Implementation of the key file download function.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpKeyDxe.h"

/**
  Update the device path node to include the key resource information.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              Device patch successfully updated.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpKeyUpdateDevicePath (
  IN   HTTP_KEY_PRIVATE_DATA   *Private
  )
{
  EFI_DEV_PATH               *Node;
  EFI_DEVICE_PATH_PROTOCOL   *TmpIpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *TmpDnsDevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *NewDevicePath;
  UINTN                      Length;
  EFI_STATUS                 Status;

  TmpIpDevicePath  = NULL;
  TmpDnsDevicePath = NULL;

  //
  // Update the IP node with DHCP assigned information.
  //
  if (!Private->UsingIpv6) {
    Node = AllocateZeroPool (sizeof (IPv4_DEVICE_PATH));
    if (Node == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Node->Ipv4.Header.Type    = MESSAGING_DEVICE_PATH;
    Node->Ipv4.Header.SubType = MSG_IPv4_DP;
    SetDevicePathNodeLength (Node, sizeof (IPv4_DEVICE_PATH));
    CopyMem (&Node->Ipv4.LocalIpAddress, &Private->StationIp, sizeof (EFI_IPv4_ADDRESS));
    Node->Ipv4.RemotePort      = Private->Port;
    Node->Ipv4.Protocol        = EFI_IP_PROTO_TCP;
    Node->Ipv4.StaticIpAddress = FALSE;
    CopyMem (&Node->Ipv4.GatewayIpAddress, &Private->GatewayIp, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Node->Ipv4.SubnetMask, &Private->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  } else {
    Node = AllocateZeroPool (sizeof (IPv6_DEVICE_PATH));
    if (Node == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Node->Ipv6.Header.Type     = MESSAGING_DEVICE_PATH;
    Node->Ipv6.Header.SubType  = MSG_IPv6_DP;
    SetDevicePathNodeLength (Node, sizeof (IPv6_DEVICE_PATH));
    Node->Ipv6.PrefixLength    = IP6_PREFIX_LENGTH;
    Node->Ipv6.RemotePort      = Private->Port;
    Node->Ipv6.Protocol        = EFI_IP_PROTO_TCP;
    Node->Ipv6.IpAddressOrigin = 0;
    CopyMem (&Node->Ipv6.LocalIpAddress, &Private->StationIp.v6, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&Node->Ipv6.RemoteIpAddress, &Private->ServerIp.v6, sizeof (EFI_IPv6_ADDRESS));
    CopyMem (&Node->Ipv6.GatewayIpAddress, &Private->GatewayIp.v6, sizeof (EFI_IPv6_ADDRESS));
  }

  TmpIpDevicePath = AppendDevicePathNode (Private->ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  if (TmpIpDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update the DNS node with DNS server IP list if existed.
  //
  if (Private->DnsServerIp != NULL) {
    Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + sizeof (Node->Dns.IsIPv6) + Private->DnsServerCount * sizeof (EFI_IP_ADDRESS);
    Node = AllocatePool (Length);
    if (Node == NULL) {
      FreePool (TmpIpDevicePath);
      return EFI_OUT_OF_RESOURCES;
    }
    Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
    Node->DevPath.SubType = MSG_DNS_DP;
    SetDevicePathNodeLength (Node, Length);
    Node->Dns.IsIPv6 = Private->UsingIpv6 ? 0x01 : 0x00;
    CopyMem ((UINT8*) Node + sizeof (EFI_DEVICE_PATH_PROTOCOL) + sizeof (Node->Dns.IsIPv6), Private->DnsServerIp, Private->DnsServerCount * sizeof (EFI_IP_ADDRESS));

    TmpDnsDevicePath = AppendDevicePathNode (TmpIpDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (Node);
    FreePool (TmpIpDevicePath);
    TmpIpDevicePath = NULL;
    if (TmpDnsDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Update the URI node with the key file URI.
  //
  Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + AsciiStrSize (Private->KeyUri);
  Node = AllocatePool (Length);
  if (Node == NULL) {
    if (TmpIpDevicePath != NULL) {
      FreePool (TmpIpDevicePath);
    }
    if (TmpDnsDevicePath != NULL) {
      FreePool (TmpDnsDevicePath);
    }
    return EFI_OUT_OF_RESOURCES;
  }
  Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, Length);
  CopyMem ((UINT8*) Node + sizeof (EFI_DEVICE_PATH_PROTOCOL), Private->KeyUri, AsciiStrSize (Private->KeyUri));

  if (TmpDnsDevicePath != NULL) {
    NewDevicePath = AppendDevicePathNode (TmpDnsDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (TmpDnsDevicePath);
  } else {
    ASSERT (TmpIpDevicePath != NULL);
    NewDevicePath = AppendDevicePathNode (TmpIpDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
    FreePool (TmpIpDevicePath);
  }
  FreePool (Node);
  if (NewDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!Private->UsingIpv6) {
    //
    // Reinstall the device path protocol of the child handle.
    //
    Status = gBS->ReinstallProtocolInterface (
                    Private->Ip4Nic->Controller,
                    &gEfiDevicePathProtocolGuid,
                    Private->Ip4Nic->DevicePath,
                    NewDevicePath
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FreePool (Private->Ip4Nic->DevicePath);
    Private->Ip4Nic->DevicePath = NewDevicePath;
  } else {
    //
    // Reinstall the device path protocol of the child handle.
    //
    Status = gBS->ReinstallProtocolInterface (
                    Private->Ip6Nic->Controller,
                    &gEfiDevicePathProtocolGuid,
                    Private->Ip6Nic->DevicePath,
                    NewDevicePath
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FreePool (Private->Ip6Nic->DevicePath);
    Private->Ip6Nic->DevicePath = NewDevicePath;
  }

  return EFI_SUCCESS;
}

/**
  Parse the key URI information from the selected Dhcp4 offer packet.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully parsed out all the key information.
  @retval Others               Failed to parse out the key information.

**/
EFI_STATUS
HttpKeyDhcp4ExtractUriInfo (
  IN     HTTP_KEY_PRIVATE_DATA   *Private
  )
{
  HTTP_KEY_DHCP4_PACKET_CACHE    *SelectOffer;
  UINT32                          SelectIndex;
  UINT32                          DnsServerIndex;
  EFI_DHCP4_PACKET_OPTION         *Option;
  EFI_STATUS                      Status;

  ASSERT (Private != NULL);
  ASSERT (Private->SelectIndex != 0);
  SelectIndex = Private->SelectIndex - 1;
  ASSERT (SelectIndex < HTTP_KEY_OFFER_MAX_NUM);

  DnsServerIndex = 0;

  Status = EFI_SUCCESS;

  //
  // SelectOffer contains the IP address configuration and name server configuration.
  //
  SelectOffer = &Private->OfferBuffer[SelectIndex].Dhcp4;
  Private->KeyUriParser = Private->KeyPathUriParser;
  Private->KeyUri = Private->KeyPathUri;

  //
  // Check the URI scheme.
  //
  Status = HttpKeyCheckUriScheme (Private->KeyUri);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HttpKeyDhcp4ExtractUriInfo: %r.\n", Status));
    if (Status == EFI_INVALID_PARAMETER) {
      AsciiPrint ("\n  Error: Invalid URI address.\n");
    } else if (Status == EFI_ACCESS_DENIED) {
      AsciiPrint ("\n  Error: Access forbidden, only HTTPS connection is allowed.\n");
    }
    return Status;
  }

  if ((SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpIpUriDns)) {
    Option = SelectOffer->OptList[HTTP_KEY_DHCP4_TAG_INDEX_DNS_SERVER];
    ASSERT (Option != NULL);

    //
    // Record the Dns Server address list.
    //
    Private->DnsServerCount = (Option->Length) / sizeof (EFI_IPv4_ADDRESS);

    Private->DnsServerIp = AllocateZeroPool (Private->DnsServerCount * sizeof (EFI_IP_ADDRESS));
    if (Private->DnsServerIp == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (DnsServerIndex = 0; DnsServerIndex < Private->DnsServerCount; DnsServerIndex++) {
      CopyMem (&(Private->DnsServerIp[DnsServerIndex].v4), &(((EFI_IPv4_ADDRESS *) Option->Data)[DnsServerIndex]), sizeof (EFI_IPv4_ADDRESS));
    }

    //
    // Configure the default DNS server if server assigned.
    //
    Status = HttpKeyRegisterIp4Dns (
               Private,
               Option->Length,
               Option->Data
               );
    if (EFI_ERROR (Status)) {
      FreePool (Private->DnsServerIp);
      Private->DnsServerIp = NULL;
      return Status;
    }
  }

  //
  // Extract the port from URL, and use default HTTP port 80 if not provided.
  //
  Status = HttpUrlGetPort (
             Private->KeyUri,
             Private->KeyUriParser,
             &Private->Port
             );
  if (EFI_ERROR (Status) || Private->Port == 0) {
    Private->Port = 80;
  }

  //
  // All key informations are valid here.
  //

  //
  // Update the device path to include the key resource information.
  //
  Status = HttpKeyUpdateDevicePath (Private);
  if (EFI_ERROR (Status) && Private->DnsServerIp != NULL) {
    FreePool (Private->DnsServerIp);
    Private->DnsServerIp = NULL;
  }

  return Status;
}

/**
  Parse the key URI information from the selected Dhcp6 offer packet.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully parsed out all the key information.
  @retval Others               Failed to parse out the key information.

**/
EFI_STATUS
HttpKeyDhcp6ExtractUriInfo (
  IN     HTTP_KEY_PRIVATE_DATA   *Private
  )
{
  HTTP_KEY_DHCP6_PACKET_CACHE    *SelectOffer;
  UINT32                          SelectIndex;
  UINT32                          DnsServerIndex;
  EFI_DHCP6_PACKET_OPTION         *Option;
  EFI_IPv6_ADDRESS                IpAddr;
  CHAR8                           *HostName;
  UINTN                           HostNameSize;
  CHAR16                          *HostNameStr;
  EFI_STATUS                      Status;

  ASSERT (Private != NULL);
  ASSERT (Private->SelectIndex != 0);
  SelectIndex = Private->SelectIndex - 1;
  ASSERT (SelectIndex < HTTP_KEY_OFFER_MAX_NUM);

  DnsServerIndex = 0;

  Status   = EFI_SUCCESS;
  HostName = NULL;
  //
  // SelectOffer contains the IP address configuration and name server configuration.
  //
  SelectOffer = &Private->OfferBuffer[SelectIndex].Dhcp6;
  Private->KeyUriParser = Private->KeyPathUriParser;
  Private->KeyUri = Private->KeyPathUri;

  //
  // Check the URI scheme.
  //
  Status = HttpKeyCheckUriScheme (Private->KeyUri);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HttpKeyDhcp6ExtractUriInfo: %r.\n", Status));
    if (Status == EFI_INVALID_PARAMETER) {
      AsciiPrint ("\n  Error: Invalid URI address.\n");
    } else if (Status == EFI_ACCESS_DENIED) {
      AsciiPrint ("\n  Error: Access forbidden, only HTTPS connection is allowed.\n");
    }
    return Status;
  }

  //
  //  Set the Local station address to IP layer.
  //
  Status = HttpKeySetIp6Address (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register the IPv6 gateway address to the network device.
  //
  Status = HttpKeySetIp6Gateway (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((SelectOffer->OfferType == HttpOfferTypeDhcpNameUriDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpDns) ||
      (SelectOffer->OfferType == HttpOfferTypeDhcpIpUriDns)) {
    Option = SelectOffer->OptList[HTTP_KEY_DHCP6_IDX_DNS_SERVER];
    ASSERT (Option != NULL);

    //
    // Record the Dns Server address list.
    //
    Private->DnsServerCount = HTONS (Option->OpLen) / sizeof (EFI_IPv6_ADDRESS);

    Private->DnsServerIp = AllocateZeroPool (Private->DnsServerCount * sizeof (EFI_IP_ADDRESS));
    if (Private->DnsServerIp == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (DnsServerIndex = 0; DnsServerIndex < Private->DnsServerCount; DnsServerIndex++) {
      CopyMem (&(Private->DnsServerIp[DnsServerIndex].v6), &(((EFI_IPv6_ADDRESS *) Option->Data)[DnsServerIndex]), sizeof (EFI_IPv6_ADDRESS));
    }

    //
    // Configure the default DNS server if server assigned.
    //
    Status = HttpKeySetIp6Dns (
               Private,
               HTONS (Option->OpLen),
               Option->Data
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  //
  // Extract the HTTP server Ip from URL. This is used to Check route table
  // whether can send message to HTTP Server Ip through the GateWay.
  //
  Status = HttpUrlGetIp6 (
             Private->KeyUri,
             Private->KeyUriParser,
             &IpAddr
             );

  if (EFI_ERROR (Status)) {
    //
    // The Http server address is expressed by Name Ip, so perform DNS resolution
    //
    Status = HttpUrlGetHostName (
               Private->KeyUri,
               Private->KeyUriParser,
               &HostName
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    HostNameSize = AsciiStrSize (HostName);
    HostNameStr = AllocateZeroPool (HostNameSize * sizeof (CHAR16));
    if (HostNameStr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    AsciiStrToUnicodeStrS (HostName, HostNameStr, HostNameSize);

    if (HostName != NULL) {
      FreePool (HostName);
    }

    Status = HttpKeyDns (Private, HostNameStr, &IpAddr);
    FreePool (HostNameStr);
    if (EFI_ERROR (Status)) {
      AsciiPrint ("\n  Error: Could not retrieve the host address from DNS server.\n");
      goto Error;
    }
  }

  CopyMem (&Private->ServerIp.v6, &IpAddr, sizeof (EFI_IPv6_ADDRESS));

  //
  // Extract the port from URL, and use default HTTP port 80 if not provided.
  //
  Status = HttpUrlGetPort (
             Private->KeyUri,
             Private->KeyUriParser,
             &Private->Port
             );
  if (EFI_ERROR (Status) || Private->Port == 0) {
    Private->Port = 80;
  }

  //
  // All key informations are valid here.
  //

  //
  // Update the device path to include the key resource information.
  //
  Status = HttpKeyUpdateDevicePath (Private);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  return Status;

Error:
  if (Private->DnsServerIp != NULL) {
    FreePool (Private->DnsServerIp);
    Private->DnsServerIp = NULL;
  }

  return Status;
}


/**
  Discover all the information for key url.

  @param[in, out]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully obtained all the key information .
  @retval Others               Failed to retrieve the key information.

**/
EFI_STATUS
HttpKeyDiscoverUrlInfo (
  IN OUT HTTP_KEY_PRIVATE_DATA   *Private
  )
{
  EFI_STATUS              Status;

  //
  // Start D.O.R.A/S.A.R.R exchange to acquire station ip address and
  // other Http key information.
  //
  Status = HttpKeyDhcp (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!Private->UsingIpv6) {
    Status = HttpKeyDhcp4ExtractUriInfo (Private);
  } else {
    Status = HttpKeyDhcp6ExtractUriInfo (Private);
  }

  return Status;
}

/**
  HttpIo Callback function which will be invoked when specified HTTP_IO_CALLBACK_EVENT happened.

  @param[in]    EventType      Indicate the Event type that occurs in the current callback.
  @param[in]    Message        HTTP message which will be send to, or just received from HTTP server.
  @param[in]    Context        The Callback Context pointer.

  @retval EFI_SUCCESS          Tells the HttpIo to continue the HTTP process.
  @retval Others               Tells the HttpIo to abort the current HTTP process.
**/
EFI_STATUS
EFIAPI
HttpKeyHttpIoCallback (
  IN  HTTP_IO_CALLBACK_EVENT    EventType,
  IN  EFI_HTTP_MESSAGE          *Message,
  IN  VOID                      *Context
  )
{
  HTTP_KEY_PRIVATE_DATA       *Private;
  EFI_STATUS                   Status;
  Private = (HTTP_KEY_PRIVATE_DATA *) Context;

  if (Private->HttpKeyCallback != NULL) {
    Status = Private->HttpKeyCallback->Callback (
               Private->HttpKeyCallback,
               EventType == HttpIoRequest ? HttpKeyHttpRequest : HttpKeyHttpResponse,
               EventType == HttpIoRequest ? FALSE : TRUE,
               sizeof (EFI_HTTP_MESSAGE),
               (VOID *) Message
               );
    return Status;
  }
  return EFI_SUCCESS;
}

/**
  Create a HttpIo instance for the file download.

  @param[in]    Private        The pointer to the driver's private data.

  @retval EFI_SUCCESS          Successfully created.
  @retval Others               Failed to create HttpIo.

**/
EFI_STATUS
HttpKeyCreateHttpIo (
  IN     HTTP_KEY_PRIVATE_DATA       *Private
  )
{
  HTTP_IO_CONFIG_DATA          ConfigData;
  EFI_STATUS                   Status;
  EFI_HANDLE                   ImageHandle;

  ASSERT (Private != NULL);

  ZeroMem (&ConfigData, sizeof (HTTP_IO_CONFIG_DATA));
  if (!Private->UsingIpv6) {
    ConfigData.Config4.HttpVersion    = HttpVersion11;
    ConfigData.Config4.RequestTimeOut = HTTP_KEY_REQUEST_TIMEOUT;
    IP4_COPY_ADDRESS (&ConfigData.Config4.LocalIp, &Private->StationIp.v4);
    IP4_COPY_ADDRESS (&ConfigData.Config4.SubnetMask, &Private->SubnetMask.v4);
    ImageHandle = Private->Ip4Nic->ImageHandle;
  } else {
    ConfigData.Config6.HttpVersion    = HttpVersion11;
    ConfigData.Config6.RequestTimeOut = HTTP_KEY_REQUEST_TIMEOUT;
    IP6_COPY_ADDRESS (&ConfigData.Config6.LocalIp, &Private->StationIp.v6);
    ImageHandle = Private->Ip6Nic->ImageHandle;
  }

  Status = HttpIoCreateIo (
             ImageHandle,
             Private->Controller,
             Private->UsingIpv6 ? IP_VERSION_6 : IP_VERSION_4,
             &ConfigData,
             HttpKeyHttpIoCallback,
             (VOID *) Private,
             &Private->HttpIo
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->HttpCreated = TRUE;
  return EFI_SUCCESS;
}

/**
  A callback function to intercept events during message parser.

  This function will be invoked during HttpParseMessageBody() with various events type. An error
  return status of the callback function will cause the HttpParseMessageBody() aborted.

  @param[in]    EventType          Event type of this callback call.
  @param[in]    Data               A pointer to data buffer.
  @param[in]    Length             Length in bytes of the Data.
  @param[in]    Context            Callback context set by HttpInitMsgParser().

  @retval EFI_SUCCESS              Continue to parser the message body.
  @retval Others                   Abort the parse.

**/
EFI_STATUS
EFIAPI
HttpKeyGetKeyCallback (
  IN HTTP_BODY_PARSE_EVENT      EventType,
  IN CHAR8                      *Data,
  IN UINTN                      Length,
  IN VOID                       *Context
  )
{
  HTTP_KEY_CALLBACK_DATA      *CallbackData;
  EFI_STATUS                   Status;
  HTTP_KEY_CALLBACK_PROTOCOL   *HttpKeyCallback;

  //
  // We only care about the entity data.
  //
  if (EventType != BodyParseEventOnData) {
    return EFI_SUCCESS;
  }

  CallbackData = (HTTP_KEY_CALLBACK_DATA *) Context;
  HttpKeyCallback = CallbackData->Private->HttpKeyCallback;
  if (HttpKeyCallback != NULL) {
    Status = HttpKeyCallback->Callback (
               HttpKeyCallback,
               HttpKeyHttpEntityBody,
               TRUE,
               (UINT32)Length,
               Data
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // Copy data if caller has provided a buffer.
  //
  if (CallbackData->BufferSize > CallbackData->CopyedSize) {
    CopyMem (
      CallbackData->Buffer + CallbackData->CopyedSize,
      Data,
      MIN (Length, CallbackData->BufferSize - CallbackData->CopyedSize)
      );
    CallbackData->CopyedSize += MIN (Length, CallbackData->BufferSize - CallbackData->CopyedSize);
  }

  return EFI_SUCCESS;
}

/**
  This function download the key file by using UEFI HTTP protocol.

  @param[in]       Private         The pointer to the driver's private data.
  @param[in]       HeaderOnly      Only request the response header, it could save a lot of time if
                                   the caller only want to know the size of the requested file.
  @param[in, out]  BufferSize      On input the size of Buffer in bytes. On output with a return
                                   code of EFI_SUCCESS, the amount of data transferred to
                                   Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                   the size of Buffer required to retrieve the requested file.
  @param[out]      Buffer          The memory buffer to transfer the file to. IF Buffer is NULL,
                                   then the size of the requested file is returned in
                                   BufferSize.
  @param[out]      ImageType       The image type of the downloaded file.

  @retval EFI_SUCCESS              The file was loaded.
  @retval EFI_INVALID_PARAMETER    BufferSize is NULL or Buffer Size is not NULL but Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources
  @retval EFI_BUFFER_TOO_SMALL     The BufferSize is too small to read the current directory entry.
                                   BufferSize has been updated with the size needed to complete
                                   the request.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpKeyGetKey (
  IN     HTTP_KEY_PRIVATE_DATA   *Private,
  IN     EFI_HTTP_METHOD          Method,
  IN     BOOLEAN                  HeaderOnly,
  IN OUT UINTN                    *BufferSize,
     OUT UINT8                    *Buffer
  )
{
  EFI_STATUS                 Status;
  EFI_HTTP_STATUS_CODE       StatusCode;
  CHAR8                      *HostName;
  EFI_HTTP_REQUEST_DATA      *RequestData;
  HTTP_IO_RESPONSE_DATA      *ResponseData;
  HTTP_IO                    *HttpIo;
  HTTP_IO_HEADER             *HttpIoHeader;
  VOID                       *Parser;
  HTTP_KEY_CALLBACK_DATA    Context;
  UINTN                      ContentLength;
  UINTN                      UrlSize;
  CHAR16                     *Url;

  ASSERT (Private != NULL);
  ASSERT (Private->HttpCreated);

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*BufferSize != 0 && Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First, check whether we already cached the requested Uri.
  //
  UrlSize = AsciiStrSize (Private->KeyUri);
  Url = AllocatePool (UrlSize * sizeof (CHAR16));
  if (Url == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  AsciiStrToUnicodeStrS (Private->KeyUri, Url, UrlSize);

  //
  // 2. Send HTTP request message.
  //

  //
  // 2.1 Build HTTP header for the request, 3 header is needed to download a key file:
  //       Host
  //       Accept
  //       User-Agent
  //
  HttpIoHeader = HttpKeyCreateHeader (3);
  if (HttpIoHeader == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_2;
  }

  //
  // Add HTTP header field 1: Host
  //
  HostName = NULL;
  Status = HttpUrlGetHostName (
             Private->KeyUri,
             Private->KeyUriParser,
             &HostName
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }
  Status = HttpKeySetHeader (
             HttpIoHeader,
             HTTP_HEADER_HOST,
             HostName
             );
  FreePool (HostName);
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }

  //
  // Add HTTP header field 2: Accept
  //
  Status = HttpKeySetHeader (
             HttpIoHeader,
             HTTP_HEADER_ACCEPT,
             "*/*"
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }

  //
  // Add HTTP header field 3: User-Agent
  //
  Status = HttpKeySetHeader (
             HttpIoHeader,
             HTTP_HEADER_USER_AGENT,
             HTTP_USER_AGENT_EFI_HTTP_KEY
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_3;
  }

  //
  // 2.2 Build the rest of HTTP request info.
  //
  RequestData = AllocatePool (sizeof (EFI_HTTP_REQUEST_DATA));
  if (RequestData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_3;
  }
  RequestData->Method = HttpMethodGet;
  RequestData->Url = Url;

  //
  // 2.4 Send out the request to HTTP server.
  //
  HttpIo = &Private->HttpIo;
  Status = HttpIoSendRequest (
             HttpIo,
             RequestData,
             HttpIoHeader->HeaderCount,
             HttpIoHeader->Headers,
             0,
             NULL
            );
  if (EFI_ERROR (Status)) {
    goto ERROR_4;
  }

  //
  // 3. Receive HTTP response message.
  //

  ResponseData = AllocateZeroPool (sizeof(HTTP_IO_RESPONSE_DATA));
  if (ResponseData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR_4;
  }

  //
  // 3.1 Only set response buffer if we are ready to receive key
  //
  if (!HeaderOnly) {
    ResponseData->Body = (CHAR8 *)Buffer;
    ResponseData->BodyLength = *BufferSize;
  }
  

  Status = HttpIoRecvResponse (
             &Private->HttpIo,
             TRUE,
             ResponseData
             );
  if (EFI_ERROR (Status) || EFI_ERROR (ResponseData->Status)) {
    if (EFI_ERROR (ResponseData->Status)) {
      StatusCode = HttpIo->RspToken.Message->Data.Response->StatusCode;
      HttpKeyPrintErrorMessage (StatusCode);
      Status = ResponseData->Status;
    }
    goto ERROR_5;
  }

  //
  // 3.3 Init a message-body parser from the header information.
  //
  Parser = NULL;
  Context.NewBlock   = FALSE;
  Context.Block      = NULL;
  Context.CopyedSize = 0;
  Context.Buffer     = Buffer;
  Context.BufferSize = *BufferSize;
  Context.Private    = Private;
  Status = HttpInitMsgParser (
             HttpMethodGet,
             ResponseData->Response.StatusCode,
             ResponseData->HeaderCount,
             ResponseData->Headers,
             HttpKeyGetKeyCallback,
             (VOID*) &Context,
             &Parser
             );
  if (EFI_ERROR (Status)) {
    goto ERROR_6;
  }
  //
  // 3.5 Message-body receive & parse is completed, we should be able to get the file size now.
  //
  Status = HttpGetEntityLength (Parser, &ContentLength);
  if (EFI_ERROR (Status)) {
    goto ERROR_6;
  }
  if (*BufferSize < ContentLength) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    Status = EFI_SUCCESS;
  }

  *BufferSize = ContentLength;

  if (EFI_ERROR (Status)) {
    goto ERROR_6;
  }

  if (!HeaderOnly) {
    if (Private->HttpKeyCallback != NULL) {
      Status = Private->HttpKeyCallback->Callback (
                     Private->HttpKeyCallback,
                     HttpKeyHttpEntityBody,
                     TRUE,
                     (UINT32)ResponseData->BodyLength,
                     ResponseData->Body
                     );
     }
  }

ERROR_6:
  if (Parser != NULL) {
    HttpFreeMsgParser (Parser);
  }
  if (Context.Block != NULL) {
    FreePool (Context.Block);
  }

ERROR_5:
  if (ResponseData != NULL) {
    FreePool (ResponseData);
  }
ERROR_4:
  if (RequestData != NULL) {
    FreePool (RequestData);
  }
ERROR_3:
  HttpKeyFreeHeader (HttpIoHeader);
ERROR_2:
  return Status;
}
