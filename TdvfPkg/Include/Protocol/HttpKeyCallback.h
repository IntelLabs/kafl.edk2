#ifndef __HTTP_KEY_CALLBACK_H__
#define __HTTP_KEY_CALLBACK_H__

#define HTTP_KEY_CALLBACK_PROTOCOL_GUID \
  { \
    0x6352f520, 0xf2cd, 0x11ea, (0x8b, 0x6e, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66} \
  }

typedef struct _HTTP_KEY_CALLBACK_PROTOCOL  HTTP_KEY_CALLBACK_PROTOCOL;

///
/// HTTP_KEY_CALLBACK_DATA_TYPE
///
typedef enum {
  ///
  /// Data points to a DHCP4 packet which is about to transmit or has received.
  ///
  HttpKeyDhcp4,
  ///
  /// Data points to a DHCP6 packet which is about to be transmit or has received.
  ///
  HttpKeyDhcp6,
  ///
  /// Data points to an EFI_HTTP_MESSAGE structure, whichcontians a HTTP request message
  /// to be transmitted.
  ///
  HttpKeyHttpRequest,
  ///
  /// Data points to an EFI_HTTP_MESSAGE structure, which contians a received HTTP
  /// response message.
  ///
  HttpKeyHttpResponse,
  ///
  /// Part of the entity body has been received from the HTTP server. Data points to the
  /// buffer of the entity body data.
  ///
  HttpKeyHttpEntityBody,
  HttpKeyTypeMax
} HTTP_KEY_CALLBACK_DATA_TYPE;

/**
  Callback function that is invoked when the HTTP load key driver is about to transmit or has received a
  packet.

  This function is invoked when the HTTP load key driver is about to transmit or has received packet.
  Parameters DataType and Received specify the type of event and the format of the buffer pointed
  to by Data. Due to the polling nature of UEFI device drivers, this callback function should not
  execute for more than 5 ms.
  The returned status code determines the behavior of the HTTP load key driver.

  @param[in]  This                Pointer to the TDVF_LOAD_KEY_CALLBACK_PROTOCOL instance.
  @param[in]  DataType            The event that occurs in the current state.
  @param[in]  Received            TRUE if the callback is being invoked due to a receive event.
                                  FALSE if the callback is being invoked due to a transmit event.
  @param[in]  DataLength          The length in bytes of the buffer pointed to by Data.
  @param[in]  Data                A pointer to the buffer of data, the data type is specified by
                                  DataType.

  @retval EFI_SUCCESS             Tells the HTTP load key driver to continue the HTTP load key process.
  @retval EFI_ABORTED             Tells the HTTP load key driver to abort the current HTTP load key process.
**/
typedef
EFI_STATUS
(EFIAPI * HTTP_KEY_CALLBACK) (
  IN HTTP_KEY_CALLBACK_PROTOCOL    *This,
  IN HTTP_KEY_CALLBACK_DATA_TYPE   DataType,
  IN BOOLEAN                            Received,
  IN UINT32                             DataLength,
  IN VOID                               *Data   OPTIONAL
 );

///
/// EFI HTTP load key Callback Protocol is invoked when the HTTP load key driver is about to transmit or
/// has received a packet. The load key Callback Protocol must be installed on the same handle
/// as the Load Key Protocol for the HTTP load key.
///
struct _HTTP_KEY_CALLBACK_PROTOCOL {
  HTTP_KEY_CALLBACK Callback;
};

extern EFI_GUID gEfiHttpKeyCallbackProtocolGuid;

#endif
