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

typedef unsigned char byte;

/*
 * Type of a QMI device.  May be created with qmidev_new_file and destroyed
 * with qmidev_destroy.
 */
typedef struct qmidev qmidev;

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
  qmidev *dev,
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
  qmidev *dev,
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
  qmidev *dev,
  void *context,
  int status,
  const char *result);

/* TODO */
int qmidev_init(void);

/*
 * Creates a new QMI device backed by a file (to which the QMI messages will
 * be written to and read from).  (This is likely to be a device file, such as
 * /dev/TODO.)
 *
 * Returns the allocated QMI device, or NULL if it could not be created.
 */
qmidev *qmidev_new_file(const char *path);

/*
 * Sets the "private data" pointer for a QMI device.  This pointer is solely
 * for use by the caller, perhaps to tie the QMI device to an associated object
 * of its own.
 */
void qmidev_set_priv(qmidev *dev, void *priv);

/*
 * Returns the "private data" pointer for a QMI device.
 *
 * See qmidev_set_priv for more details.
 */
void *qmidev_priv(qmidev *dev);

/*
 * Returns the file descriptor upon which the caller should wait to determine
 * when there is activity on the QMI device that needs to be processed.  The
 * caller should add this fd to a select or epoll loop, and call qmidev_process
 * with the QMI device when select or epoll indicates the fd is ready.
 *
 * This function may be called any time after creating the QMI device.
 */
int qmidev_fd(qmidev *dev);

/*
 * Processes activity on the QMI device.  This function should be called when
 * select or epoll reveals that there is activity on the file descriptor
 * returned by qmidev_fd.
 */
int qmidev_process(qmidev *dev);

/*
 * Connects to the device.  This may involve several method calls to initialize
 * various aspects of the connection to the device.  The provided callback will
 * be called with the provided context when the library has finished connecting
 * to the device (or failed to connect).
 */
int qmidev_connect(qmidev *dev, qmidev_response_fn callback, void *context);

/*
 * Disconnects from the device.  This may involve several method calls to tear
 * down various aspects of the connection to the device.  The provided callback
 * will be called with the provided context when the library has finished
 * disconnecting to the device (or failed to disconnect).
 */
int qmidev_disconnect(qmidev *dev, qmidev_response_fn callback, void *context);

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
} qmidev_session_state;

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

/* Performs automatic service activation. */
int qmidev_activate_automatic(
  qmidev *dev,
  const char *activation_code,
  qmidev_response_fn callback,
  void *context);

/**
 * Performs manual service activation and then resets the device.
 * @spc: Service Programming Code (six digit string)
 * @sid: System identification number
 * @mdn: Mobile Directory Number (string or NULL)
 * @min: Mobile Identification Number (string or NULL)
 * @prl: PRL file (byte array or NULL),
 * @prl_len: length of PRL file, if provided
 * @mnha: MN-HA (string or NULL)
 * @mnaaa: MN-AAA (string or NULL)
 */
int qmidev_activate_manual(
  qmidev *dev,
  const char *spc,
  int sid,
  const char *mdn,
  const char *min,
  const char *prl,
  size_t prl_len,
  const char *mnha,
  const char *mnaaa,
  qmidev_response_fn callback,
  void *context);

/*
 * Deletes one or more SMS messages from the device.
 * storage_type is the storage type of the messages to delete.
 *
 * If idx and tag are both NULL, all the messages in the given storage
 * location are deleted.
 *
 * If idx is NULL but tag is not, all the messages in the given storage
 * location with the given tag are deleted.
 *
 * If idx is not NULL buttag is, the message with the given index in
 * the given storage location is deleted.
 *
 * If idx and tag are both non-NULL, an error is returned.
 */
int qmidev_sms_delete(
  qmidev *dev,
  qmidev_sms_storage storage_type,
  uint8_t *idx,
  qmidev_sms_tag *tag,
  qmidev_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_get_activation_state.  state is the activation
 * state of the device.
 */
typedef void (*qmidev_activation_state_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_activation_state state);

/* Gets the activation state of the device. */
int qmidev_get_activation_state(
  qmidev *dev,
  qmidev_activation_state_response_fn callback,
  void *context);

/* Gets the active mobile IP profile of the device. */
int qmidev_get_active_mobile_ip_profile(
  qmidev *dev,
  qmidev_int_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_get_data_bearer_technology.  tech is the data
 * bearer technology.
 */
typedef void (*qmidev_data_bearer_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_data_bearer tech);

/*
 * Gets the current data bearer technology of the device.  Only valid when
 * connected to a network.
 */
int qmidev_get_data_bearer_technology(
  qmidev *dev,
  qmidev_data_bearer_response_fn callback,
  void *context);

/* Callback type for qmidev_get_device_capabilities.  */
typedef void (*qmidev_device_capabilities_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  const qmidev_device_caps *caps);

/* Gets the device capabilities.  TODO. */
int qmidev_get_device_capabilities(
  qmidev *dev,
  qmidev_device_capabilities_response_fn callback,
  void *context);

/* TODO: enums */

/* Callback type for qmidev_get_firmware_info. */
typedef void (*qmidev_firmware_info_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int firmware_id,
  int technology,
  int carrier,
  int region,
  int gps_capability);

/* Gets information about the current device firmware. */
int qmidev_get_firmware_info(
  qmidev *dev,
  qmidev_firmware_info_response_fn callback,
  void *context);

/* Gets the device firmware revision. */
int qmidev_get_firmware_revision(
  qmidev *dev,
  qmidev_string_response_fn callback,
  void *context);

/* Callback type for qmidev_get_firmware_revisions. */
typedef void (*qmidev_firmware_revisions_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  const char *amss_version,
  const char *boot_version,
  const char *pri_version);

/* Gets the revisions of the current device AMSS, boot, and PRI firmware. */
int qmidev_get_firmware_revisions(
  qmidev *dev,
  qmidev_firmware_revisions_response_fn callback,
  void *context);

/* Gets the hardware revision of the device. */
int qmidev_get_hardware_revision(
  qmidev *dev,
  qmidev_string_response_fn callback,
  void *context);

/**
 * Callback type for qmidev_get_home_network.
 * @mcc: Mobile Country Code
 * @mnc: Mobile Network Code
 * @name: network name
 * @sid: System ID (of home network)
 * @nid: Network ID (of home network)
 */
typedef void (*qmidev_home_network_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int mcc,
  int mnc,
  const char *name,
  int sid,
  int nid);

/* Gets information about the home network of the device. */
int qmidev_get_home_network(
  qmidev *dev,
  qmidev_home_network_response_fn callback,
  void *context);

/* Gets the IMSI of the device. */
int qmidev_get_imsi(
  qmidev *dev,
  qmidev_string_response_fn callback,
  void *context);

/* Gets the manufacturer of the device. */
int qmidev_get_manufacturer(
  qmidev *dev,
  qmidev_string_response_fn callback,
  void *context);

/* Gets the model ID of the device. */
int qmidev_get_model_id(
  qmidev *dev,
  qmidev_string_response_fn callback,
  void *context);

/**
 * Callback type for qmidev_get_plmn_name.
 * @name_is_ucs2: 1 if the name is in UCS-2, 0 if TODO
 * @name_len: the length of name in bytes.
 * @name: the name itself
 */
typedef void (*qmidev_plmn_name_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  char name_is_ucs2,
  size_t name_len,
  const char *name);

/**
 * Gets PLMN information for a given network.
 * @mcc: Mobile Country Code
 * @mnc: Mobile Network Code
 */
int qmidev_get_plmn_name(
  qmidev *dev,
  int mcc,
  int mnc,
  qmidev_plmn_name_response_fn callback,
  void *context);

/* Callback type for qmidev_get_power.  state is the power state. */
typedef void (*qmidev_power_state_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_power_state state);

/* Gets the power state of the device. */
int qmidev_get_power(
  qmidev *dev,
  qmidev_power_state_response_fn callback,
  void *context);

/* Gets the PRL version on the device. */
int qmidev_get_prl_version(
  qmidev *dev,
  qmidev_int_response_fn callback,
  void *context);

/* Callback type for qmidev_get_rf_info.  info is the info structure. */
typedef void (*qmidev_rf_info_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  const qmidev_rf_info info);

/* Gets the current RF information from the device. */
int qmidev_get_rf_info(
  qmidev *dev,
  qmidev_rf_info_response_fn callback,
  void *context);

/**
 * Callback type for qmidev_get_ids.
 * @esn: Electronic Serial Number,
 * @imei: International Mobile Equipment Identifier
 * @meid: Mobile Equipment Identifier
 */
typedef void (*qmidev_get_ids_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  const char *esn,
  const char *imei,
  const char *meid);

/* Gets the serial numbers of the device. */
int qmidev_get_ids(
  qmidev *dev,
  qmidev_get_ids_response_fn callback,
  void *context);

/* TODO: enums */

/* Callback type for qmidev_get_serving_network. */
typedef void (*qmidev_serving_network_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int reg_state,
  int cs_status,
  int ps_status,
  int radio_access_network,
  size_t num_interfaces,
  const qmidev_radio_interface *interfaces,
  int roaming,
  int mcc,
  int mnc,
  size_t name_len,
  const char *name);

/* Gets information about the serving network. */
int qmidev_get_serving_network(
  qmidev *dev,
  qmidev_serving_network_response_fn callback,
  void *context);

/* Callback type for qmidev_get_serving_network_caps. */
typedef void (*qmidev_serving_network_caps_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  size_t caps_len,
  const qmidev_data_cap *caps);

/* Gets the capabilities of the serving network. */
int qmidev_get_serving_network_caps(
  qmidev *dev,
  qmidev_serving_network_caps_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_get_signal_strengths.  num_strengths is the number
 * of signal strengths; strengths is an array of num_strengths items, each
 * containing the strength for one radio interface.
 */
typedef void (*qmidev_signal_strengths_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int num_strengths,
  qmidev_signal_strength strengths[]);

/* Gets the signal strength(s) of the device. */
int qmidev_get_signal_strengths(
  qmidev *dev,
  qmidev_signal_strengths_response_fn callback,
  void *context);

/* Callback type for qmidev_get_sms. */
typedef void (*qmidev_sms_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_sms_tag tag,
  qmidev_sms_format format,
  size_t message_len,
  const byte *message);

/* Gets an SMS from the device, given the storage location and idx. */
int qmidev_get_sms(
  qmidev *dev,
  qmidev_sms_storage storage,
  uint8_t idx,
  qmidev_sms_response_fn callback,
  void *context);

/* Callback type for qmidev_get_smsc_address. */
typedef void (*qmidev_smsc_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  const char *smsc_address,
  const char *smsc_type);

/* Gets the SMSC address (and type). */
int qmidev_get_smsc_address(
  qmidev *dev,
  qmidev_smsc_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_get_sms_list.  num_entries is the number of entries
 * in the SMS list, and entries is an array of num_entries entries.
 */
typedef void (*qmidev_sms_list_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  size_t num_entries,
  const qmidev_sms_list_entry *entries);

/* Gets a list of SMS messages, given a storage location and (optional) tag. */
int qmidev_get_sms_list(
  qmidev *dev,
  qmidev_sms_storage storage,
  qmidev_sms_tag *tag,
  qmidev_sms_list_response_fn callback,
  void *context);

/**
 * Callback type for qmidev_get_voice_number.
 *
 * @voice_number: the device's voice number,
 * @min: the device's Mobile Identification Number
 */
typedef void (*qmidev_voice_number_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  const char *voice_number,
  const char *min);

/* Gets the voice number (and MIN) of the device. */
int qmidev_get_voice_number(
  qmidev *dev,
  qmidev_voice_number_response_fn callback,
  void *context);

/* Initiates network registration. */
int qmidev_initiate_network_registration(
  qmidev *dev,
  qmidev_reg_type registration_type,
  int mcc,
  int mnc,
  qmidev_radio_access_tech radio_access_type,
  qmidev_response_fn callback,
  void *context);

/* Callback type for qmidev_omadm_get_session_info.  TODO. */
typedef void (*qmidev_omadm_session_info_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  /* TODO: enums */
  int state,
  int type,
  int failure_reason,
  int retry_count,
  int session_pause,
  int time_remaining);

/*
 * Gets information related to the current (or previous, if no session is
 * active) OMA-DM session.
 */
int qmidev_omadm_get_session_info(
  qmidev *dev,
  qmidev_omadm_session_info_response_fn callback,
  void *context);

/* Starts an OMA-DM session.  type is the type of session to initiate. */
int qmidev_omadm_start_session(
  qmidev *dev,
  /* TODO: enum */
  int type,
  qmidev_response_fn callback,
  void *context);

typedef struct qmidev_network_info qmidev_network_info;

struct qmidev_network_info {
  int mcc;
  int mnc;
  /* flags */
  char in_use;
  char roaming;
  char forbidden;
  char preferred;
};

/*
 * Callback type for qmidev_perform_network_scan.  num_networks is the number
 * of networks found; networks is an array of num_networks network info
 * structures.
 */
typedef void (*qmidev_network_scan_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  size_t num_networks;
  qmidev_network_info *networks;
);

/* Performs a scan for available networks. */
int qmidev_perform_network_scan(
  qmidev *dev,
  qmidev_network_scan_response_fn *callback,
  void *context);

/**
 * Resets the device's configuration to factory defaults and then resets the
 * device.
 * @spc: Service Programming Code.
 */
int qmidev_reset_to_factory_defaults(
  qmidev *dev,
  const char *spc,
  qmidev_response_fn callback,
  void *context);

/* Enables or disables port automatic tracking for the NMEA port. */
int qmidev_set_port_automatic_tracking(
  qmidev *dev,
  int enabled,
  qmidev_response_fn callback,
  void *context);

/* Sets the operating mode of the device. */
int qmidev_set_power(
  qmidev *dev,
  qmidev_power_state state,
  qmidev_response_fn callback,
  void *context);

/* Enables or disables service automatic tracking. */
int qmidev_set_service_automatic_tracking(
  qmidev *dev,
  int enabled,
  qmidev_response_fn callback,
  void *context);

/* Sets the SMSC address (and type). */
int qmidev_set_smsc_address(
  qmidev *dev,
  const char *smsc_address,
  const char *smsc_type,
  qmidev_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_start_data_session.  If successful, session_id
 * is the id of the newly-started session.  If unsuccessful, failure_reason
 * is the reason the session did not start.
 */
typedef void (*qmidev_start_data_session_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int session_id, /* TODO: Really int? */
  qmidev_oma_failure_reason failure_reason);

/* Starts a packet data session.  TODO: Document absurd arguments. */
int qmidev_start_data_session(
  qmidev *dev,
  qmidev_session_tech *tech,
  uint32_t *pri_dns,
  uint32_t *sec_dns,
  uint32_t *pri_nbns,
  uint32_t *sec_nbns,
  const char *apn_name,
  uint32_t *ip_addr,
  qmidev_session_auth *auth,
  const char *username,
  const char *password,
  qmidev_start_data_session_response_fn callback,
  void *context);

/*
 * Stops a packet data session.  session_id is the session id, as passed to
 * the callback for qmidev_start_data_session when the session started.
 */
int qmidev_stop_data_session(
  qmidev *dev,
  int session_id,
  qmidev_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_uim_change_pin.  If unsuccessful,
 * verify_retries_left and unblock_retries_left will contain the number of
 * retries available to verify the PIN and unblock the PIN, respectively.
 */
typedef void (*qmidev_uim_change_pin_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int verify_retries_left,
  int unblock_retries_left);

/*
 * Changes a PIN on the UIM.  pin_id is which pin (1 for PIN, 2 for PIN2);
 * old_value and new_value are the old and new values for the PIN.
 */
int qmidev_uim_change_pin(
  qmidev *dev,
  qmidev_pin_id pin_id,
  const char *old_value,
  const char *new_value,
  qmidev_uim_change_pin_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_uim_get_pin_status.  pin_status is the PIN status;
 * verify_retries_left and unblock_retries_left are the number of retries
 * available to verify the PIN and unblock the PIN, respectively.
 */
typedef void (*qmidev_uim_get_pin_status_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_pin_status pin_status,
  int verify_retries_left,
  int unblock_retries_left);

/*
 * Gets the status of one of the PINs on the UIM.  pin_id is which pin (1 for
 * PIN, 2 for PIN2).
 */
int qmidev_uim_get_pin_status(
  qmidev *dev,
  qmidev_pin_id pin_id,
  qmidev_uim_get_pin_status_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_uim_set_pin_protection.  If unsuccessful,
 * verify_retries_left is the number of retries available to verify the PIN.
 */
typedef void (*qmidev_uim_set_pin_protection_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int verify_retries_left);

/**
 * Enables or disables PIN protection for one of the PINs on the UIM.
 * @pin_id: which pin (1 for PIN, 2 for PIN2)
 * @enabled: whether to enable (1) or disable (0) the PIN protection
 * @value: the value of the PIN.
 */
int qmidev_uim_set_pin_protection(
  qmidev *dev,
  qmidev_pin_id pin_id,
  int enabled,
  const char *value,
  qmidev_uim_set_pin_protection_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_uim_unblock_pin.  If unsuccessful,
 * verify_retries_left and unblock_retries_left are the number of retries
 * available to verify the PIN and unblock the PIN, respectively.
 */
typedef void (*qmidev_uim_unblock_pin_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int verify_retries_left,
  int unblock_retries_left);

/*
 * Unblocks one of the PINs on the UIM.  pin_id is which pin (1 = PIN, 2 =
 * PIN2), puk_value is the value of the corresponding PUK (PUK or PUK2), and
 * new_value is the value to which to reset the PIN to.
 */
int qmidev_uim_unblock_pin(
  qmidev *dev,
  qmidev_pin_id pin_id,
  const char *puk_value,
  const char *new_value,
  qmidev_uim_unblock_pin_response_fn callback,
  void *context);

/*
 * Callback type for qmidev_uim_verify_pin.  If unsuccessful,
 * verify_retries_left is the number of retries available to verify the PIN.
 */
typedef void (*qmidev_uim_verify_pin_response_fn)(
  qmidev *dev,
  void *context,
  int status,
  int verify_retries_left);

/*
 * Verifies one of the PINs on the UIM.  pin_id is which pin (1 = PIN, 2 =
 * PIN2).  value is the value of that PIN.
 */
int qmidev_uim_verify_pin(
  qmidev *dev,
  qmidev_pin_id pin_id,
  const char *value,
  qmidev_uim_verify_pin_response_fn callback,
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
  qmidev *dev,
  void *context,
  int status,
  qmidev_activation_state state);

/* Sets the activation status callback. */
int qmidev_set_activation_status_changed_callback(
  qmidev *dev,
  qmidev_activation_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Byte totals callback type. */
typedef void (*qmidev_byte_totals_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  uint64_t tx_bytes,
  uint64_t rx_bytes);

/* Sets the byte totals callback. */
int qmidev_set_byte_totals_callback(
  qmidev *dev,
  qmidev_byte_totals_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Data bearer callback type.  tech is the new data bearer technology. */
typedef void (*qmidev_data_bearer_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_data_bearer tech);

/* Sets the data bearer callback. */
int qmidev_set_data_bearer_callback(
  qmidev *dev,
  qmidev_data_bearer_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Data capabilities callback type.  TODO. */
typedef void (*qmidev_data_capabilities_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  size_t num_caps,
  qmidev_data_cap *caps);

/* Sets the data capabilities callback. */
int qmidev_set_data_capabilities_callback(
  qmidev *dev,
  qmidev_data_capabilities_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Dormancy status callback type.  dormancy is the dormancy status. */
typedef void (*qmidev_dormancy_status_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_dormancy_status dormancy);

/* Sets the dormancy status callback. */
int qmidev_set_dormancy_status_callback(
  qmidev *dev,
  qmidev_dormancy_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * LU reject callback type.  domain and cause are the LU domain and the cause
 * for rejection.
 */
typedef void (*qmidev_lu_reject_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_lu_domain domain,
  qmidev_lu_cause cause);

/* Sets the LU reject callback. */
int qmidev_set_lu_reject_callback(
  qmidev *dev,
  qmidev_lu_reject_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Mobile IP status callback type.  ip_status is the new status. */
typedef void (*qmidev_mobile_ip_status_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_mobile_ip_status ip_status);

/* Sets the mobile IP status callback. */
int qmidev_set_mobile_ip_status_callback(
  qmidev *dev,
  qmidev_mobile_ip_status_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * New SMS callback type.  storage is the storage location of the new message;
 * idx is the message index within that storage location.
 */
typedef void (*qmidev_new_sms_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_sms_storage storage,
  uint8_t idx);

/* Sets the new SMS callback. */
int qmidev_set_new_sms_callback(
  qmidev *dev,
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
  qmidev *dev,
  void *context,
  int status,
  int type,
  int id);

/* Sets the OMA-DM alert callback. */
int qmidev_set_omadm_alert_callback(
  qmidev *dev,
  qmidev_omadm_alert_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* OMA-DM state callback type.  state is the new OMA-DM state. */
typedef void (*qmidev_omadm_state_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_omadm_state state);

/* Sets the OMA-DM state callback. */
int qmidev_set_omadm_state_callback(
  qmidev *dev,
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
  qmidev *dev,
  void *context,
  int status,
  int enabled,
  int tracking);

/* Sets the PDS state callback. */
int qmidev_set_pds_state_callback(
  qmidev *dev,
  qmidev_pds_state_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* Power state callback type.  state is the new power state of the device. */
typedef void (*qmidev_power_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_power_state state);

/* Sets the power state callback. */
int qmidev_set_power_callback(
  qmidev *dev,
  qmidev_power_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* RF info callback type.  info is the RF info. */
typedef void (*qmidev_rf_info_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  const qmidev_rf_info info);

/* Sets the RF info callback. */
int qmidev_set_rf_info_callback(
  qmidev *dev,
  qmidev_rf_info_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * Roaming indicator callback type.  indicator is the new state for the
 * indicator.
 */
typedef void (*qmidev_roaming_indicator_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_roaming_indicator indicator);

/* Sets the roaming indicator callback. */
int qmidev_set_roaming_indicator_callback(
  qmidev *dev,
  qmidev_roaming_indicator_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/* (Packet data) session state callback type.  state is the new state. */
typedef void (*qmidev_session_state_callback_fn)(
  qmidev *dev,
  void *context,
  int status,
  qmidev_session_state state);

/* Sets the session state callback. */
int qmidev_set_session_state_callback(
  qmidev *dev,
  qmidev_session_state_callback_fn callback,
  void *context,
  qmidev_response_fn set_callback,
  void *set_context);

/*
 * Signal strength callback type.  interface is the radio interface whose
 * strength has changed, and strength is the new strength.
 */
typedef void (*qmidev_signal_strength_callback_fn)(
  qmidev *dev,
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
  qmidev *dev,
  qmidev_signal_strength_callback_fn callback,
  void *context,
  size_t num_thresholds,
  int8_t *thresholds,
  qmidev_response_fn set_callback,
  void *set_context);

#endif /* LIBQMI_INCLUDE_QMIDEV_H */
