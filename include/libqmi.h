/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmidev: External interface to libqmi.
 *
 * Sources used in writing this file (see README for links):
 * [Gobi]/GobiConnectionMgmtAPI.h
 * [Gobi]/GobiConnectionMgmtAPIEnums.h
 * [Gobi]/GobiConnectionMgmtAPIStructs.h
 */

#ifndef LIBQMI_INCLUDE_QMIDEV_H
#define LIBQMI_INCLUDE_QMIDEV_H

#include <stddef.h>
#include <stdint.h>

struct qmidev;

/* GENERIC RESPONSE FUNCTIONS */

/*
 * Callback type for a method that returns no data besides the status of the
 * method call.  dev is the device on which the method was called; context is
 * the context passed to the method, and status is the status of the call.
 *
 * If the call made it to the device and the device returned success, status
 * will be QMIDEV_ERR_NONE (0).  If the call made it to the device but failed,
 * the status will be the sum of QMIDEV_ERR_QMI_OFFSET and the QMI error (a
 * QMIDEV_QMI_ERR_* constant).  If the call failed without the device
 * returning a response, the status will be a QMIDEV_ERR_* constant.
 */
typedef void (*qmidev_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status);

/*
 * Callback type for a method that returns a single int value.
 *
 * result is the result of the method call, if it succeeded.
 *
 * See qmidev_response_fn for more details.
 */
typedef void (*qmidev_int_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  int result);

/*
 * Callback type for a method that returns a single string value.
 *
 * result is the result of the method call, if it succeeded.  OWnership of the
 * string is retained by the library, not transferred to the callback, so the
 * callback must make a copy if it needs to preserve the value after returning.
 *
 * See qmidev_response_fn for more details.
 */
typedef void (*qmidev_string_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  const char *result);

/*
 * Creates a new QMI device backed by a file (to which the QMI messages will
 * be written to and read from).  (This is likely to be a device file, such as
 * /dev/TODO.)
 *
 * Returns the allocated QMI device, or NULL if it could not be created.
 */
struct qmidev *qmidev_new_file(const char *path);

/*
 * Sets the "private data" pointer for a QMI device.  This pointer is solely
 * for use by the caller, perhaps to tie the QMI device to an associated object
 * of its own.
 */
void qmidev_set_priv(struct qmidev *qmidev, void *priv);

/*
 * Returns the "private data" pointer for a QMI device.
 *
 * See qmidev_set_priv for more details.
 */
void *qmidev_priv(struct qmidev *qmidev);

/*
 * Returns the file descriptor upon which the caller should wait to determine
 * when there is activity on the QMI device that needs to be processed.  The
 * caller should add this fd to a select or epoll loop, and call qmidev_process
 * with the QMI device when select or epoll indicates the fd is ready.
 *
 * This function may be called any time after creating the QMI device.
 */
int qmidev_fd(struct qmidev *qmidev);

/*
 * Processes activity on the QMI device.  This function should be called when
 * select or epoll reveals that there is activity on the file descriptor
 * returned by qmidev_fd.
 */
int qmidev_process(struct qmidev *qmidev);

/*
 * Connects to the device.  This may involve several method calls to initialize
 * various aspects of the connection to the device.  The provided callback will
 * be called with the provided context when the library has finished connecting
 * to the device (or failed to connect).
 */
int qmidev_connect(struct qmidev *qmidev,
                   qmidev_response_fn callback, void *context);

/*
 * Disconnects from the device.  This may involve several method calls to tear
 * down various aspects of the connection to the device.  The provided callback
 * will be called with the provided context when the library has finished
 * disconnecting to the device (or failed to disconnect).
 */
int qmidev_disconnect(struct qmidev *qmidev,
                      qmidev_response_fn callback, void *context);

/* ENUMS */

/* TODO: Some of these make no sense, and others will need to be added. */

enum {
  QMIDEV_ERR_NONE = 0,           // 00 Success
  QMIDEV_ERR_GENERAL,            // 01 General error
  QMIDEV_ERR_INTERNAL,           // 02 Internal error
  QMIDEV_ERR_MEMORY,             // 03 Memory error
  QMIDEV_ERR_INVALID_ARG,        // 04 Invalid argument
  QMIDEV_ERR_BUFFER_SZ,          // 05 Buffer too small
  QMIDEV_ERR_NO_DEVICE,          // 06 Unable to detect device
  QMIDEV_ERR_INVALID_DEVID,      // 07 Invalid device ID
  QMIDEV_ERR_NO_CONNECTION,      // 08 No connection to device
  QMIDEV_ERR_IFACE,              // 09 Unable to obtain required interace
  QMIDEV_ERR_CONNECT,            // 10 Unable to connect to interface
  QMIDEV_ERR_REQ_SCHEDULE,       // 11 Unable to schedule request
  QMIDEV_ERR_REQUEST,            // 12 Error sending request
  QMIDEV_ERR_RESPONSE,           // 13 Error receiving response
  QMIDEV_ERR_REQUEST_TO,         // 14 Timeout while sending request
  QMIDEV_ERR_RESPONSE_TO,        // 15 Timeout while receiving response
  QMIDEV_ERR_MALFORMED_RSP,      // 16 Malformed response received
  QMIDEV_ERR_INVALID_RSP,        // 17 Invalid/error response received
  QMIDEV_ERR_INVALID_FILE,       // 18 Invalid file path
  QMIDEV_ERR_FILE_OPEN,          // 19 Unable to open file
  QMIDEV_ERR_FILE_COPY,          // 20 Unable to copy file
  QMIDEV_ERR_QDL_SCM,            // 21 Unable to open service control mgr
  QMIDEV_ERR_NO_QDL_SVC,         // 22 Unable to detect QDL service
  QMIDEV_ERR_NO_QDL_SVC_INFO,    // 23 Unable to obtain QDL service info
  QMIDEV_ERR_NO_QDL_SVC_PATH,    // 24 Unable to locate QSL service
  QMIDEV_ERR_QDL_SVC_CFG,        // 25 Unable to reconfigure QDL service
  QMIDEV_ERR_QDL_SVC_IFACE,      // 26 Unable to interface to QDL service
  QMIDEV_ERR_OFFLINE,            // 27 Unable to set device offline
  QMIDEV_ERR_RESET,              // 28 Unable to reset device
  QMIDEV_ERR_NO_SIGNAL,          // 29 No available signal
  QMIDEV_ERR_MULTIPLE_DEVICES,   // 30 Multiple devices detected
  QMIDEV_ERR_DRIVER,             // 31 Error interfacing to driver
  QMIDEV_ERR_NO_CANCELABLE_OP,   // 32 No cancelable operation is pending
  QMIDEV_ERR_CANCEL_OP,          // 33 Error canceling outstanding operation
  QMIDEV_ERR_QDL_CRC,            // 34 QDL image data CRC error
  QMIDEV_ERR_QDL_PARSING,        // 35 QDL image data parsing error
  QMIDEV_ERR_QDL_AUTH,           // 36 QDL image authentication error
  QMIDEV_ERR_QDL_WRITE,          // 37 QDL image write error
  QMIDEV_ERR_QDL_OPEN_SIZE,      // 38 QDL image size error
  QMIDEV_ERR_QDL_OPEN_TYPE,      // 39 QDL image type error
  QMIDEV_ERR_QDL_OPEN_PROT,      // 40 QDL memory protection error
  QMIDEV_ERR_QDL_OPEN_SKIP,      // 41 QDL image not required
  QMIDEV_ERR_QDL_ERR_GENERAL,    // 42 QDL general error
  QMIDEV_ERR_QDL_BAR_MODE,       // 43 QDL BAR mode error
};

/* TODO: This should really be in libqmi, not libqmidev. */

enum {
  QMIDEV_QMI_ERR_None                          = 0,
  QMIDEV_QMI_ERR_MalformedMessage              = 1,
  QMIDEV_QMI_ERR_NoMemory                      = 2,
  QMIDEV_QMI_ERR_Internal                      = 3,
  QMIDEV_QMI_ERR_Aborted                       = 4,
  QMIDEV_QMI_ERR_ClientIDsExhausted            = 5,
  QMIDEV_QMI_ERR_UnabortableTransaction        = 6,
  QMIDEV_QMI_ERR_InvalidClientID               = 7,
  QMIDEV_QMI_ERR_NoThresholdsProvided          = 8,
  QMIDEV_QMI_ERR_InvalidHandle                 = 9,
  QMIDEV_QMI_ERR_InvalidProfile                = 10,
  QMIDEV_QMI_ERR_InvalidPINID                  = 11,
  QMIDEV_QMI_ERR_IncorrectPIN                  = 12,
  QMIDEV_QMI_ERR_NoNetworkFound                = 13,
  QMIDEV_QMI_ERR_CallFailed                    = 14,
  QMIDEV_QMI_ERR_OutOfCall                     = 15,
  QMIDEV_QMI_ERR_NotProvisioned                = 16,
  QMIDEV_QMI_ERR_MissingArgument               = 17,
  QMIDEV_QMI_ERR_ArgumentTooLong               = 19,
  QMIDEV_QMI_ERR_InvalidTransactionID          = 22,
  QMIDEV_QMI_ERR_DeviceInUse                   = 23,
  QMIDEV_QMI_ERR_NetworkUnsupported            = 24,
  QMIDEV_QMI_ERR_DeviceUnsupported             = 25,
  QMIDEV_QMI_ERR_NoEffect                      = 26,
  QMIDEV_QMI_ERR_NoFreeProfile                 = 27,
  QMIDEV_QMI_ERR_InvalidPDPType                = 28,
  QMIDEV_QMI_ERR_InvalidTechnologyPreference   = 29,
  QMIDEV_QMI_ERR_InvalidProfileType            = 30,
  QMIDEV_QMI_ERR_InvalidServiceType            = 31,
  QMIDEV_QMI_ERR_InvalidRegisterAction         = 32,
  QMIDEV_QMI_ERR_InvalidPSAttachAction         = 33,
  QMIDEV_QMI_ERR_AuthenticationFailed          = 34,
  QMIDEV_QMI_ERR_PINBlocked                    = 35,
  QMIDEV_QMI_ERR_PINAlwaysBlocked              = 36,
  QMIDEV_QMI_ERR_UIMUninitialized              = 37,
  QMIDEV_QMI_ERR_MaximumQoSRequestsInUse       = 38,
  QMIDEV_QMI_ERR_IncorrectFlowFilter           = 39,
  QMIDEV_QMI_ERR_NetworkQoSUnaware             = 40,
  QMIDEV_QMI_ERR_InvalidQoSID                  = 41,
  QMIDEV_QMI_ERR_QoSUnavailable                = 42,
  QMIDEV_QMI_ERR_FlowSuspended                 = 43,
  QMIDEV_QMI_ERR_GeneralError                  = 46,
  QMIDEV_QMI_ERR_UnknownError                  = 47,
  QMIDEV_QMI_ERR_InvalidArgument               = 48,
  QMIDEV_QMI_ERR_InvalidIndex                  = 49,
  QMIDEV_QMI_ERR_NoEntry                       = 50,
  QMIDEV_QMI_ERR_DeviceStorageFull             = 51,
  QMIDEV_QMI_ERR_DeviceNotReady                = 52,
  QMIDEV_QMI_ERR_NetworkNotReady               = 53,
  QMIDEV_QMI_ERR_WMSCauseCode                  = 54,
  QMIDEV_QMI_ERR_WMSMessageNotSent             = 55,
  QMIDEV_QMI_ERR_WMSMessageDeliveryFailure     = 56,
  QMIDEV_QMI_ERR_WMSInvalidMessageID           = 57,
  QMIDEV_QMI_ERR_WMSEncoding                   = 58,
  QMIDEV_QMI_ERR_AuthenticationLock            = 59,
  QMIDEV_QMI_ERR_InvalidTransition             = 60,
  QMIDEV_QMI_ERR_SessionInactive               = 65,
  QMIDEV_QMI_ERR_SessionInvalid                = 66,
  QMIDEV_QMI_ERR_SessionOwnership              = 67,
  QMIDEV_QMI_ERR_InsufficientResources         = 68,
  QMIDEV_QMI_ERR_Disabled                      = 69,
  QMIDEV_QMI_ERR_InvalidOperation              = 70,
  QMIDEV_QMI_ERR_InvalidQMICommand             = 71,
  QMIDEV_QMI_ERR_WMSTPDUType                   = 72,
  QMIDEV_QMI_ERR_WMSSMSCAddress                = 73,
  QMIDEV_QMI_ERR_InformationUnavailable        = 74,
  QMIDEV_QMI_ERR_SegmentTooLong                = 75,
  QMIDEV_QMI_ERR_SegmentOrder                  = 76,
  QMIDEV_QMI_ERR_BundlingNotSupported          = 77,
  QMIDEV_QMI_ERR_SIMFileNotFound               = 80,
  QMIDEV_QMI_ERR_AccessDenied                  = 82,
  QMIDEV_QMI_ERR_HardwareRestricted            = 83,
  QMIDEV_QMI_ERR_CATEventRegistrationFailed    = 61441,
  QMIDEV_QMI_ERR_CATInvalidTerminalResponse    = 61442,
  QMIDEV_QMI_ERR_CATInvalidEnvelopeCommand     = 61443,
  QMIDEV_QMI_ERR_CATEnvelopeCommandBusy        = 61444,
  QMIDEV_QMI_ERR_CATEnvelopeCommandFailed      = 61445
};

/* TODO: These should be generated with dcbw's script. */

typedef enum {
  QMIDEV_ACT_STATE_NOT_ACTIVATED = 0,
  QMIDEV_ACT_STATE_ACTIVATED,
  QMIDEV_ACT_STATE_CONNECTING,
  QMIDEV_ACT_STATE_IN_PROGRESS,
  QMIDEV_ACT_STATE_OTASP_SECURITY_AUTHENTICATED,
  QMIDEV_ACT_STATE_OTASP_NAM_DOWNLOADED,
  QMIDEV_ACT_STATE_OTASP_MDN_DOWNLOADED,
  QMIDEV_ACT_STATE_OTASP_IMSI_DOWNLOADED,
  QMIDEV_ACT_STATE_OTASP_PRL_DOWNLOADED,
  QMIDEV_ACT_STATE_OTASP_SPC_DOWNLOADED,
  QMIDEV_ACT_STATE_OTASP_SETTINGS_COMMITTED
} qmidev_activation_state;

typedef enum {
  QMIDEV_POWER_ONLINE = 0,
  QMIDEV_POWER_LOW_POWER,
  QMIDEV_POWER_FACTORY_TEST,
  QMIDEV_POWER_OFFLINE,
  QMIDEV_POWER_RESET,
  QMIDEV_POWER_SHUTDOWN,
  QMIDEV_POWER_PERSISTENT_LOW_POWER,
  QMIDEV_POWER_MODE_ONLY_LOW_POWER
} qmidev_power_state;

typedef enum {
  QMIDEV_SESSION_DISCONNECTED = 1,
  QMIDEV_SESSION_CONNECTED,
  QMIDEV_SESSION_SUSPENDED,
  QMIDEV_SESSION_AUTHENTICATING
} qmidev_packet_status;

typedef enum {
  QMIDEV_BEARER_CDMA2000 = 1,
  QMIDEV_BEARER_EVDO_REV0,
  QMIDEV_BEARER_GPRS,
  QMIDEV_BEARER_WCDMA,
  QMIDEV_BEARER_EVDO_REVA,
  QMIDEV_BEARER_EGPRS,
  QMIDEV_BEARER_HSDPA,
  QMIDEV_BEARER_HSUPA,
  QMIDEV_BEARER_HSPA,
  QMIDEV_BEARER_LTE,
  QMIDEV_BEARER_EHRPD,
  QMIDEV_BEARER_HSDPA_PLUS,
  QMIDEV_BEARER_HSPA_PLUS,
  QMIDEV_BEARER_HSDPA_PLUS_DUAL,
  QMIDEV_BEARER_HSPA_PLUS_DUAL,
  QMIDEV_BEARER_UNKNOWN = 255
} qmidev_data_bearer;

typedef enum {
  QMIDEV_STORAGE_UIM = 0,
  QMIDEV_STORAGE_NV,
  QMIDEV_STORAGE_UNKNOWN = 2
} qmidev_sms_storage;

typedef enum {
  QMIDEV_TAG_MT_READ   = 0,
  QMIDEV_TAG_MT_UNREAD,
  QMIDEV_TAG_MO_SENT,
  QMIDEV_TAG_MO_UNSENT
} qmidev_sms_tag;

typedef struct qmidev_signal_strength qmidev_signal_strength;

typedef enum {
  QMIDEV_INTERFACE_NONE = 0,
  QMIDEV_INTERFACE_CDMA_1X,
  QMIDEV_INTERFACE_CDMA_HRPD,
  QMIDEV_INTERFACE_AMPS,
  QMIDEV_INTERFACE_GSM,
  QMIDEV_INTERFACE_UMTS,
  QMIDEV_INTERFACE_LTE = 8
} qmidev_radio_interface;

typedef enum {
  QMIDEV_BAND_CLASS_CDMABandClass0                                      = 0,
  QMIDEV_BAND_CLASS_CDMABandClass1                                      = 1,
  QMIDEV_BAND_CLASS_CDMABandClass3                                      = 3,
  QMIDEV_BAND_CLASS_CDMABandClass4                                      = 4,
  QMIDEV_BAND_CLASS_CDMABandClass5                                      = 5,
  QMIDEV_BAND_CLASS_CDMABandClass6                                      = 6,
  QMIDEV_BAND_CLASS_CDMABandClass7                                      = 7,
  QMIDEV_BAND_CLASS_CDMABandClass8                                      = 8,
  QMIDEV_BAND_CLASS_CDMABandClass9                                      = 9,
  QMIDEV_BAND_CLASS_CDMABandClass10                                     = 10,
  QMIDEV_BAND_CLASS_CDMABandClass11                                     = 11,
  QMIDEV_BAND_CLASS_CDMABandClass12                                     = 12,
  QMIDEV_BAND_CLASS_CDMABandClass13                                     = 13,
  QMIDEV_BAND_CLASS_CDMABandClass14                                     = 14,
  QMIDEV_BAND_CLASS_CDMABandClass15                                     = 15,
  QMIDEV_BAND_CLASS_CDMABandClass16                                     = 16,
  QMIDEV_BAND_CLASS_CDMABandClass17                                     = 17,
  QMIDEV_BAND_CLASS_CDMABandClass18                                     = 18,
  QMIDEV_BAND_CLASS_CDMABandClass19                                     = 19,
  QMIDEV_BAND_CLASS_GSM450                                              = 40,
  QMIDEV_BAND_CLASS_GSM480                                              = 41,
  QMIDEV_BAND_CLASS_GSM750                                              = 42,
  QMIDEV_BAND_CLASS_GSM850                                              = 43,
  QMIDEV_BAND_CLASS_GSM900Extended                                      = 44,
  QMIDEV_BAND_CLASS_GSM900Primary                                       = 45,
  QMIDEV_BAND_CLASS_GSM900Railways                                      = 46,
  QMIDEV_BAND_CLASS_GSM1800                                             = 47,
  QMIDEV_BAND_CLASS_GSM1900                                             = 48,
  QMIDEV_BAND_CLASS_WCDMA2100                                           = 80,
  QMIDEV_BAND_CLASS_WCDMAPCS1900                                        = 81,
  QMIDEV_BAND_CLASS_WCDMADCS1800                                        = 82,
  QMIDEV_BAND_CLASS_WCDMA1700US                                         = 83,
  QMIDEV_BAND_CLASS_WCDMA850                                            = 84,
  QMIDEV_BAND_CLASS_WCDMA800                                            = 85,
  QMIDEV_BAND_CLASS_WCDMA2600                                           = 86,
  QMIDEV_BAND_CLASS_WCDMA900                                            = 87,
  QMIDEV_BAND_CLASS_WCDMA1700Japan                                      = 88,
  QMIDEV_BAND_CLASS_WCDMA1500Japan                                      = 90,
  QMIDEV_BAND_CLASS_WCDMA850Japan                                       = 91,
  QMIDEV_BAND_CLASS_EUTRABand1                                          = 120,
  QMIDEV_BAND_CLASS_EUTRABand2                                          = 121,
  QMIDEV_BAND_CLASS_EUTRABand3                                          = 122,
  QMIDEV_BAND_CLASS_EUTRABand4                                          = 123,
  QMIDEV_BAND_CLASS_EUTRABand5                                          = 124,
  QMIDEV_BAND_CLASS_EUTRABand6                                          = 125,
  QMIDEV_BAND_CLASS_EUTRABand7                                          = 126,
  QMIDEV_BAND_CLASS_EUTRABand8                                          = 127,
  QMIDEV_BAND_CLASS_EUTRABand9                                          = 128,
  QMIDEV_BAND_CLASS_EUTRABand10                                         = 129,
  QMIDEV_BAND_CLASS_EUTRABand11                                         = 130,
  QMIDEV_BAND_CLASS_EUTRABand12                                         = 131,
  QMIDEV_BAND_CLASS_EUTRABand13                                         = 132,
  QMIDEV_BAND_CLASS_EUTRABand14                                         = 133,
  QMIDEV_BAND_CLASS_EUTRABand17                                         = 134,
  QMIDEV_BAND_CLASS_EUTRABand33                                         = 135,
  QMIDEV_BAND_CLASS_EUTRABand34                                         = 136,
  QMIDEV_BAND_CLASS_EUTRABand35                                         = 137,
  QMIDEV_BAND_CLASS_EUTRABand36                                         = 138,
  QMIDEV_BAND_CLASS_EUTRABand37                                         = 139,
  QMIDEV_BAND_CLASS_EUTRABand38                                         = 140,
  QMIDEV_BAND_CLASS_EUTRABand39                                         = 141,
  QMIDEV_BAND_CLASS_EUTRABand40                                         = 142,
  QMIDEV_BAND_CLASS_EUTRABand18                                         = 143,
  QMIDEV_BAND_CLASS_EUTRABand19                                         = 144,
  QMIDEV_BAND_CLASS_EUTRABand20                                         = 145,
  QMIDEV_BAND_CLASS_EUTRABand21                                         = 146,
} qmidev_band_class;

struct qmidev_signal_strength {
  qmidev_radio_interface radio_interface;
  int8_t signal_strength;
};

typedef struct {
  uint8_t idx;
  qmidev_sms_tag tag;
} qmidev_sms_list_entry;

typedef struct {
  qmidev_radio_interface radio_interface;
  qmidev_band_class active_band_class;
  int active_channel;
} qmidev_rf_info;

typedef enum {
  QMIDEV_DORMANCY_DORMANT = 1,
  QMIDEV_DORMANCY_ACTIVE
} qmidev_dormancy_status;

/* TODO: enums */
typedef int qmidev_lu_domain;
typedef int qmidev_lu_cause;
typedef int qmidev_mobile_ip_status;
typedef int qmidev_sms_format;

typedef enum {
  QMIDEV_OMADM_STATE_COMPLETE_UPDATED = 0,
  QMIDEV_OMADM_STATE_COMPLETE_UNAVAILABLE,
  QMIDEV_OMADM_STATE_FAILED,
  QMIDEV_OMADM_STATE_RETRYING,
  QMIDEV_OMADM_STATE_CONNECTING,
  QMIDEV_OMADM_STATE_CONNECTED,
  QMIDEV_OMADM_STATE_AUTHENTICATED,
  QMIDEV_OMADM_STATE_MDN_DOWNLOADED,
  QMIDEV_OMADM_STATE_MSID_DOWNLOADED,
  QMIDEV_OMADM_STATE_PRL_DOWNLOADED,
  QMIDEV_OMADM_STATE_MIP_PROFILE_DOWNLOADED
} qmidev_omadm_state;

typedef enum {
  QMIDEV_ROAMIND_ROAMING = 0,
  QMIDEV_ROAMIND_HOME,
  QMIDEV_ROAMIND_ROAMING_PARTNER
} qmidev_roaming_indicator;

typedef enum {
  QMIDEV_SESSION_TECH_UMTS = 1,
  QMIDEV_SESSION_TECH_CDMA = 2
} qmidev_session_tech;

typedef enum {
  QMIDEV_SESSION_AUTH_PAP = 1,
  QMIDEV_SESSION_AUTH_CHAP = 2
} qmidev_session_auth;

typedef enum {
  QMIDEV_DATA_CAP_NONE = 0,
  QMIDEV_DATA_CAP_CS = 1,
  QMIDEV_DATA_CAP_PS = 2,
  QMIDEV_DATA_CAP_SIMUL_CS_PS = 3,
  QMIDEV_DATA_CAP_NONSIMUL_CS_PS = 4
} qmidev_data_cap;

typedef struct {
  uint32_t max_tx_rate;
  uint32_t max_rx_rate;
  uint8_t data_caps; /* qmidev_data_service_caps */
  int8_t sim_supported;
  uint8_t num_interfaces;
  uint8_t interfaces[]; /* qmidev_radio_interface */
} qmidev_device_caps;

typedef enum {
  QMIDEV_REG_TYPE_AUTOMATIC = 1,
  QMIDEV_REG_TYPE_MANUAL = 2
} qmidev_reg_type;

typedef enum {
  QMIDEV_RADIO_ACCESS_TECH_GSM = 4,
  QMIDEV_RADIO_ACCESS_TECH_UMTS = 5,
  QMIDEV_RADIO_ACCESS_TECH_LTE = 8
} qmidev_radio_access_tech;

typedef enum {
  QMIDEV_OMA_FAILURE_REASON_UNKNOWN = 0,
  QMIDEV_OMA_FAILURE_REASON_NETWORK_UNAVAILABLE,
  QMIDEV_OMA_FAILURE_REASON_SERVER_UNAVAILABLE,
  QMIDEV_OMA_FAILURE_REASON_AUTHENTICATION_FAILED,
  QMIDEV_OMA_FAILURE_REASON_MAX_RETRY_EXCEEDED,
  QMIDEV_OMA_FAILURE_REASON_SESSION_CANCELLED
} qmidev_oma_failure_reason;

typedef enum {
  QMIDEV_PIN_ID_PIN1 = 1,
  QMIDEV_PIN_ID_PIN2,
  QMIDEV_PIN_ID_UNIVERSAL_PIN,
  QMIDEV_PIN_HIDDEN_KEY
} qmidev_pin_id;

typedef enum {
  QMIDEV_PIN_STATUS_UNKNOWN = 0,
  QMIDEV_PIN_STATUS_ENABLED_UNVERIFIED,
  QMIDEV_PIN_STATUS_ENABLED_VERIFIED,
  QMIDEV_PIN_STATUS_DISABLED,
  QMIDEV_PIN_STATUS_BLOCKED,
  QMIDEV_PIN_STATUS_PERM_BLOCKED
} qmidev_pin_status;

/* METHODS */

/* Callback type for qmidev_get_power.  state is the power state. */
typedef void (*qmidev_power_state_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_power_state state);

/* Gets the power state of the device. */
int qmidev_get_power(
  struct qmidev *qmidev,
  qmidev_power_state_response_fn callback,
  void *context);

/**
 * Callback type for qmidev_get_ids.
 * @esn: Electronic Serial Number,
 * @imei: International Mobile Equipment Identifier
 * @meid: Mobile Equipment Identifier
 */
typedef void (*qmidev_get_ids_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  const char *esn,
  const char *imei,
  const char *meid);

/* Gets the serial numbers of the device. */
int qmidev_get_ids(
  struct qmidev *qmidev,
  qmidev_get_ids_response_fn callback,
  void *context);

/* Sets the operating mode of the device. */
int qmidev_set_power(
  struct qmidev *qmidev,
  qmidev_power_state state,
  qmidev_response_fn callback,
  void *context);

typedef uint32_t qmidev_session_id;

typedef void (*qmidev_packet_status_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_packet_status packet_status);

int qmidev_get_packet_status(
  struct qmidev *qmidev,
  qmidev_packet_status_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_start_network.  If successful, session_id
 * is the id of the newly-started session.  If unsuccessful, failure_reason
 * is the reason the session did not start.
 */
typedef void (*qmidev_start_network_response_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_session_id session_id, /* TODO: Really int? */
  uint16_t call_end_reason);

/* Starts a packet data session.  TODO: Document absurd arguments. */
int qmidev_start_network(
  struct qmidev *qmidev,
  qmidev_start_network_response_fn callback,
  void *context);

/*
 * Stops a packet data session.  session_id is the session id, as passed to
 * the callback for qmidev_start_data_session when the session started.
 */
int qmidev_stop_network(
  struct qmidev *qmidev,
  qmidev_session_id session_id,
  qmidev_response_fn callback,
  void *context);

/* CALLBACKS */

/*
 * A general note on qmidev_set_*_callback functions:
 *
 * QMI devices offer a set of asynchronous messages (callbacks) that the host
 * can register for.  These functions allow the caller to register a callback
 * function (with a context) to be called when one of these messages is
 * received.
 *
 * Since registering or unregistering for one of these messages requires
 * talking to the device, these functions are asynchronous, and can fail.
 * Each takes two sets of callbacks and contexts: callback and context are the
 * callback and context to call when the associated message is received, and
 * set_callback and set_context are the callback and context to call when the
 * callback-setting process itself has succeeded or failed.
 *
 * If callback is non-NULL, the library will replace an existing callback, if
 * any, or register for the message and set the callback if not.
 *
 * If callback is NULL, the library will remove any existing callback and
 * unregister for the message if registered.
 *
 * The callback types contain a status parameter; if a message from the device
 * is intended for the callback, but is malformed, the callback will be called
 * with an appropriate failure code.
 */

/* Activation status callback type.  state is the activation state. */
typedef void (*qmidev_activation_status_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_activation_state state);

/* Sets the activation status callback. */
int qmidev_set_activation_status_changed_callback(
  struct qmidev *qmidev,
  qmidev_activation_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Byte totals callback type. */
typedef void (*qmidev_byte_totals_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  uint64_t tx_bytes,
  uint64_t rx_bytes);

/* Sets the byte totals callback. */
int qmidev_set_byte_totals_callback(
  struct qmidev *qmidev,
  qmidev_byte_totals_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Data bearer callback type.  tech is the new data bearer technology. */
typedef void (*qmidev_data_bearer_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_data_bearer tech);

/* Sets the data bearer callback. */
int qmidev_set_data_bearer_callback(
  struct qmidev *qmidev,
  qmidev_data_bearer_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Data capabilities callback type.  TODO. */
typedef void (*qmidev_data_capabilities_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  size_t num_caps,
  qmidev_data_cap *caps);

/* Sets the data capabilities callback. */
int qmidev_set_data_capabilities_callback(
  struct qmidev *qmidev,
  qmidev_data_capabilities_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Dormancy status callback type.  dormancy is the dormancy status. */
typedef void (*qmidev_dormancy_status_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_dormancy_status dormancy);

/* Sets the dormancy status callback. */
int qmidev_set_dormancy_status_callback(
  struct qmidev *qmidev,
  qmidev_dormancy_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * LU reject callback type.  domain and cause are the LU domain and the cause
 * for rejection.
 */
typedef void (*qmidev_lu_reject_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_lu_domain domain,
  qmidev_lu_cause cause);

/* Sets the LU reject callback. */
int qmidev_set_lu_reject_callback(
  struct qmidev *qmidev,
  qmidev_lu_reject_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Mobile IP status callback type.  ip_status is the new status. */
typedef void (*qmidev_mobile_ip_status_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_mobile_ip_status ip_status);

/* Sets the mobile IP status callback. */
int qmidev_set_mobile_ip_status_callback(
  struct qmidev *qmidev,
  qmidev_mobile_ip_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * New SMS callback type.  storage is the storage location of the new message;
 * idx is the message index within that storage location.
 */
typedef void (*qmidev_new_sms_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_sms_storage storage,
  uint8_t idx);

/* Sets the new SMS callback. */
int qmidev_set_new_sms_callback(
  struct qmidev *qmidev,
  qmidev_new_sms_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* We just log this callback? */

/* TODO: Better types for type and id? */

/*
 * OMA-DM alert callback type.  type and id are the type and ID of the
 * alert.
 */
typedef void (*qmidev_omadm_alert_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  int type,
  int id);

/* Sets the OMA-DM alert callback. */
int qmidev_set_omadm_alert_callback(
  struct qmidev *qmidev,
  qmidev_omadm_alert_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* OMA-DM state callback type.  state is the new OMA-DM state. */
typedef void (*qmidev_omadm_state_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_omadm_state state);

/* Sets the OMA-DM state callback. */
int qmidev_set_omadm_state_callback(
  struct qmidev *qmidev,
  qmidev_omadm_state_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* TODO: enums? */

/*
 * PDS (position determination service) state callback type.  state is the new
 * state.
 */
typedef void (*qmidev_pds_state_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  int enabled,
  int tracking);

/* Sets the PDS state callback. */
int qmidev_set_pds_state_callback(
  struct qmidev *qmidev,
  qmidev_pds_state_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Power state callback type.  state is the new power state of the device. */
typedef void (*qmidev_power_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_power_state state);

/* Sets the power state callback. */
int qmidev_set_power_callback(
  struct qmidev *qmidev,
  qmidev_power_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* RF info callback type.  info is the RF info. */
typedef void (*qmidev_rf_info_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  const qmidev_rf_info info);

/* Sets the RF info callback. */
int qmidev_set_rf_info_callback(
  struct qmidev *qmidev,
  qmidev_rf_info_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * Roaming indicator callback type.  indicator is the new state for the
 * indicator.
 */
typedef void (*qmidev_roaming_indicator_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_roaming_indicator indicator);

/* Sets the roaming indicator callback. */
int qmidev_set_roaming_indicator_callback(
  struct qmidev *qmidev,
  qmidev_roaming_indicator_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* packet_status callback type.  state is the new state. */
typedef void (*qmidev_packet_status_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_packet_status packet_status);

/* Sets the packet status callback. */
int qmidev_set_packet_status_callback(
  struct qmidev *qmidev,
  qmidev_packet_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * Signal strength callback type.  interface is the radio interface whose
 * strength has changed, and strength is the new strength.
 */
typedef void (*qmidev_signal_strength_callback_fn)(
  struct qmidev *qmidev,
  void *context,
  int status,
  qmidev_radio_interface interface,
  int8_t strength);

/*
 * Sets the signal strength callback and the thresholds at which to call it.
 * num_thresholds is the number of thresholds; thresholds is an array of
 * num_thresholds signal strength thresholds.
 */
int qmidev_set_signal_strength_callback(
  struct qmidev *qmidev,
  qmidev_signal_strength_callback_fn callback,
  void *context,
  size_t num_thresholds,
  int8_t *thresholds,
  qmidev_response_fn set_callback,
  void *set_context);

#endif /* LIBQMI_INCLUDE_QMIDEV_H */
