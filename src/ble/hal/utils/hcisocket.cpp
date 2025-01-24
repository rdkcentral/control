/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

//
//  hcisocket.cpp
//

#include "hcisocket_p.h"
// #include "linux/containerhelpers.h"

#include "ctrlm_log_ble.h"



using namespace std;

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>


#define FD_SIGNAL(x) (x[1])
#define FD_RECV(x) (x[0])

#ifndef SOCK_CLOEXEC
#  define SOCK_CLOEXEC      0
#endif

#ifndef AF_BLUETOOTH
#  define AF_BLUETOOTH      31
#endif


#define BTPROTO_L2CAP       0
#define BTPROTO_HCI         1
#define BTPROTO_RFCOMM      3

#define SOL_HCI             0
#define SOL_L2CAP           6
#define SOL_RFCOMM          18
#ifndef SOL_BLUETOOTH
#  define SOL_BLUETOOTH     274
#endif

// HCI sockopts
#define HCI_DATA_DIR        1
#define HCI_FILTER          2
#define HCI_TIME_STAMP      3

// HCI channels
#define HCI_CHANNEL_RAW     0
#define HCI_CHANNEL_MONITOR 2
#define HCI_CHANNEL_CONTROL 3

// HCI data types
#define HCI_COMMAND_PKT     0x01
#define HCI_ACLDATA_PKT     0x02
#define HCI_SCODATA_PKT     0x03
#define HCI_EVENT_PKT       0x04
#define HCI_VENDOR_PKT      0xff

#define HCI_FLT_TYPE_BITS   31
#define HCI_FLT_EVENT_BITS  63

#define HCI_MAX_EVENT_SIZE  260


// HCI ioctls
#define HCIGETDEVLIST   _IOR('H', 210, int)
#define HCIGETDEVINFO   _IOR('H', 211, int)
#define HCIGETCONNLIST  _IOR('H', 212, int)
#define HCIGETCONNINFO  _IOR('H', 213, int)


struct hci_filter {
    uint32_t type_mask;
    uint32_t event_mask[2];
    uint16_t opcode;
};

struct sockaddr_hci {
    sa_family_t hci_family;
    unsigned short hci_dev;
    unsigned short hci_channel;
};

//  HCI Packet structure
#define HCI_TYPE_LEN    1

typedef struct {
    uint16_t     opcode;    // OCF & OGF
    uint8_t      plen;
} __attribute__ ((packed))  hci_command_hdr;
#define HCI_COMMAND_HDR_SIZE    3

struct __attribute__ ((packed)) hci_event_hdr {
    uint8_t      evt;
    uint8_t      plen;
};
#define HCI_EVENT_HDR_SIZE  2


// Bluetooth address
typedef struct {
    uint8_t b[6];
} __attribute__((packed)) bdaddr_t;


// HCI connection info
struct hci_conn_info {
    uint16_t    handle;
    bdaddr_t    bdaddr;
    uint8_t     type;
    uint8_t     out;
    uint16_t    state;
    uint32_t    link_mode;
};

#define SCO_LINK    0x00
#define ACL_LINK    0x01
#define ESCO_LINK   0x02
#define LE_LINK     0x80
#define AMP_LINK    0x81


struct hci_conn_info_req {
    bdaddr_t bdaddr;
    uint8_t   type;
    struct hci_conn_info conn_info[0];
};

struct hci_conn_list_req {
    uint16_t dev_id;
    uint16_t conn_num;
    struct hci_conn_info conn_info[0];
};




#define EVT_DISCONN_COMPLETE    0x05
struct __attribute__ ((packed)) evt_disconn_complete {
    uint8_t      status;
    uint16_t     handle;
    uint8_t      reason;
};
#define EVT_DISCONN_COMPLETE_SIZE 4

// BLE Meta Event
#define EVT_LE_META_EVENT       0x3E
struct __attribute__ ((packed)) evt_le_meta_event {
    uint8_t      subevent;
    uint8_t      data[0];
};
#define EVT_LE_META_EVENT_SIZE 1

// BLE Meta Event - connection complete
#define EVT_LE_CONN_COMPLETE    0x01
struct __attribute__ ((packed)) evt_le_connection_complete {
    uint8_t      status;
    uint16_t     handle;
    uint8_t      role;
    uint8_t      peer_bdaddr_type;
    bdaddr_t    peer_bdaddr;
    uint16_t     interval;
    uint16_t     latency;
    uint16_t     supervision_timeout;
    uint8_t      master_clock_accuracy;
};
#define EVT_LE_CONN_COMPLETE_SIZE 18

// BLE Meta Event - update complete
#define EVT_LE_CONN_UPDATE_COMPLETE 0x03
struct __attribute__ ((packed)) evt_le_connection_update_complete {
    uint8_t      status;
    uint16_t     handle;
    uint16_t     interval;
    uint16_t     latency;
    uint16_t     supervision_timeout;
};
#define EVT_LE_CONN_UPDATE_COMPLETE_SIZE 9


// LE commands
#define OGF_LE_CTL  0x08

#define OCF_LE_CONN_UPDATE  0x0013
struct __attribute__ ((packed)) le_connection_update_cp {
    uint16_t     handle;
    uint16_t     min_interval;
    uint16_t     max_interval;
    uint16_t     latency;
    uint16_t     supervision_timeout;
    uint16_t     min_ce_length;
    uint16_t     max_ce_length;
};
#define LE_CONN_UPDATE_CP_SIZE 14


//HCI_VS_LE_SET_MORE_DATA_CAP_CMD_CODE
#define OGF_LE_VSC              0x3F
#define OCF_LE_MORE_DATA        0x01B3
#define LE_MORE_DATA_VSC_SIZE   3
//More Data Capability, 0x05~0x0A: 0x05 means 50% capability, 0x0A means 100% 
#define OCF_LE_MORE_DATA_CAPABILITY_LEVEL   0x0A
struct __attribute__ ((packed)) le_vsc_more_data_capability {
    uint16_t     handle;
    uint8_t      level;
};

// sanity checks for struct packing
static_assert(sizeof(hci_command_hdr) == HCI_COMMAND_HDR_SIZE, "struct hci_command_hdr size incorrect");
static_assert(sizeof(hci_event_hdr) == HCI_EVENT_HDR_SIZE, "struct hci_event_hdr size incorrect");

static_assert(sizeof(evt_disconn_complete) == EVT_DISCONN_COMPLETE_SIZE, "struct evt_disconn_complete size incorrect");
static_assert(sizeof(evt_le_meta_event) == EVT_LE_META_EVENT_SIZE, "struct evt_le_meta_event size incorrect");
static_assert(sizeof(evt_le_connection_complete) == EVT_LE_CONN_COMPLETE_SIZE, "struct evt_le_connection_complete size incorrect");
static_assert(sizeof(evt_le_connection_update_complete) == EVT_LE_CONN_UPDATE_COMPLETE_SIZE, "struct evt_le_connection_update_complete size incorrect");

static_assert(sizeof(le_connection_update_cp) == LE_CONN_UPDATE_CP_SIZE, "struct le_connection_update_cp size incorrect");


static void ClearEventFd(int &fd) {
    if(fd > -1) {
        uint64_t d;
        if((read(fd, &d, sizeof(uint64_t))) != sizeof(uint64_t)) {
            XLOGD_ERROR("failed to clear eventfd");
        }
    } else {
        XLOGD_ERROR("invalid fd");
    }
}

static void SignalEventFd(int &fd) {
    if(fd > -1) {
        uint64_t d = 1;
        if((write(fd, &d, sizeof(uint64_t))) != sizeof(uint64_t)) {
            XLOGD_ERROR("failed to signal eventfd");
        }
    } else {
        XLOGD_ERROR("invalid fd");
    }
}


// -----------------------------------------------------------------------------
/*!
    \class HciSocket
    \brief Wraps a bluetooth HCI socket to provide limited event notifications
    and command executation.

    This object is not intended to be a fully featured interface to the kernel's
    bluetooth HCI driver, rather it is specifically targeted for bluetooth low
    energy devices and then only the basic events and only one command.


    \warning To get all target events from the socket the process needs the
    \c CAP_NET_RAW capability or root privilage.  In addition the hci driver
    in the kernel hasn't been converted over to using user namespaces and hence
    setting \c CAP_NET_RAW will not work in a userns. The code in question is
    the following in the kernel

    \code
        if (!capable(CAP_NET_RAW)) {
            uf.type_mask &= hci_sec_filter.type_mask;
            uf.event_mask[0] &= *((u32 *) hci_sec_filter.event_mask + 0);
            uf.event_mask[1] &= *((u32 *) hci_sec_filter.event_mask + 1);
        }
    \endcode

    The issue is that the \c capable(...) call uses the initial user namespace
    of the container rather than the current. Instead it should be using
    \c capable_ns(...).  See https://github.com/moby/moby/issues/25622 for
    details on the issue.


    \warning The kernel's HCI driver hasn't been updated to work inside a
    container with a network namespace, the following kernel code means that
    attempting to create the socket will always return with \c EAFNOSUPPORT

    \code
        static int bt_sock_create(struct net *net, struct socket *sock, int proto,
                      int kern)
        {
            int err;

            if (net != &init_net)
                return -EAFNOSUPPORT;

            ...
    \endcode

 */

// -----------------------------------------------------------------------------
/*!
    \fn HciSocket::connectionCompleted(uint16_t handle, const BleAddress &device, const BleConnectionParameters &params);

    This signal is emitted when an BLE Connection Complete Event is received
    from the driver.  The \a handle is the unique handle used to identify the
    new connection and \a device is the BDADDR of the remote device that has
    just been connected to.

    \a params are the connection parameters that are currently being used for
    the connection.  Note that the BleConnectionParameters::minInterval() and
    BleConnectionParameters::maxInterval() of \a params will be the same and
    refer to the current interval used for the connection.

    \see Section 7.7.65.1 LE Connection Complete Event of Volumne 2 part E of
    the Bluetooth Core Spec 4.0
 */


// -----------------------------------------------------------------------------
/*!
    Constructs a new HciSocket object that is open and connected to the HCI
    device.  The socket will be bound to the hci device with the given
    \a deviceId, this should typically be 0 for the \c hci0 device.

    \a netNsFd refers to the network namespace in which to create the socket, if
    it is less than 0 then the current network namespace will be used.

    If a failure occurs an empty shared pointer is returned.
 */
shared_ptr<HciSocket> HciSocket::create(uint deviceId, int netNsFd)
{
    shared_ptr<HciSocketImpl> hciSocket = make_shared<HciSocketImpl>(deviceId, netNsFd);

    if (!hciSocket->isValid()) {
        hciSocket.reset();
    }

    return hciSocket;
}

// -----------------------------------------------------------------------------
/*!
    Constructs a new HciSocket object wrapping an existing \a socketFd.

    This method takes ownership of \a socketFd, it will be stored within the
    object and closed when the object is destroyed.

    It is expected the socket is opened with the following arguments:
    \code
        socket(AF_BLUETOOTH, (SOCK_RAW | SOCK_CLOEXEC), BTPROTO_HCI);
    \endcode

    This function is provided so that an HCI socket can be passed in from
    the host if running inside a container.

    If a failure occurs an empty shared pointer is returned.
 */
shared_ptr<HciSocket> HciSocket::createFromSocket(int socketFd,
                                                  uint deviceId)
{
    XLOGD_INFO("wrapping socket %d with HciSocket object", socketFd);

    shared_ptr<HciSocketImpl> hciSocket = make_shared<HciSocketImpl>(socketFd, deviceId);

    if (!hciSocket->isValid()) {
        hciSocket.reset();
    }

    return hciSocket;
}


HciSocketImpl::HciSocketImpl()
    : m_isAlive(make_shared<bool>(true))
    , m_hciSocket(-1)
    , m_hciDeviceId(0)
{
    m_socketThread.name = "";
    m_socketThread.id = 0;
    m_socketThread.running = false;
}

void *SocketThread(void *data);

HciSocketImpl::HciSocketImpl(uint hciDeviceId, int netNsFd)
    : m_isAlive(make_shared<bool>(true))
    , m_hciSocket(-1)
    , m_hciDeviceId(hciDeviceId)
{
    m_socketThread.name = "";
    m_socketThread.id = 0;
    m_socketThread.running = false;

    XLOGD_INFO("creating new socket for HciSocket object");

    // create the HCI socket, optionally in the supplied network namespace
    int sockFd = -1;
    if (netNsFd < 0) {
        sockFd = socket(AF_BLUETOOTH, (SOCK_RAW | SOCK_CLOEXEC), BTPROTO_HCI);
    }

    if (sockFd < 0) {
        int errsv = errno;
        XLOGD_WARN("failed to create hci socket %d (%s)", errsv, strerror(errsv));
        return;
    }

    // open eventfd to signal to the thread to exit
    if (pipe(m_exitEventFds) == -1) {
        int errsv = errno;
        XLOGD_ERROR("failed to open exit eventfd, error = <%d>, <%s>", errsv, strerror(errsv));
        close(sockFd);
        return;
    }

    m_socketThread.name = "ble_hci_socket";
    sem_init(&m_socketThreadSem, 0, 0);
    if (false == ctrlm_utils_thread_create(&m_socketThread, SocketThread, this)) {
        sem_destroy(&m_socketThreadSem);
        XLOGD_ERROR("failed to start hci socket thread");
        close(sockFd);
        return;
    } else  {
        // Block until initialization is complete or a timeout occurs
        XLOGD_INFO("Waiting for %s thread initialization...", m_socketThread.name);
        sem_wait(&m_socketThreadSem);
        sem_destroy(&m_socketThreadSem);
    }

    // setup the hci socket
    if (!setSocketFilter(sockFd) || !bindSocket(sockFd, hciDeviceId)) {
        close(sockFd);
        return;
    }

    m_hciSocket = sockFd;
}

HciSocketImpl::~HciSocketImpl()
{
    if (m_socketThread.running) {
        if(FD_SIGNAL(m_exitEventFds) > -1) {
            SignalEventFd(FD_SIGNAL(m_exitEventFds));
        }
        ctrlm_utils_thread_join(&m_socketThread, 2);
    }
    if (FD_SIGNAL(m_exitEventFds) > -1) {
        close(FD_SIGNAL(m_exitEventFds));
    }
    if (FD_RECV(m_exitEventFds) > -1) {
        close(FD_RECV(m_exitEventFds));
    }

    *m_isAlive = false;

    if ((m_hciSocket >= 0) && (::close(m_hciSocket) != 0)) {
        int errsv = errno;
        XLOGD_WARN("failed to close hci socket %d (%s)", errsv, strerror(errsv));
    }
}


// -----------------------------------------------------------------------------
/*!
    \internal

    Sets the HCI filter so we get only the events we care about.

 */
bool HciSocketImpl::setSocketFilter(int socketFd) const
{
    const uint32_t filterTypeMask = (1UL << HCI_EVENT_PKT);
    const uint32_t filterEvenMask[2] = { (1UL << EVT_DISCONN_COMPLETE),
                                        (1UL << (EVT_LE_META_EVENT - 32)) };


    // get the filter first in case we don't need to change it
    struct hci_filter filter;
    bzero(&filter, sizeof(filter));
    socklen_t filterLen = sizeof(filter);

    if (getsockopt(socketFd, SOL_HCI, HCI_FILTER, &filter, &filterLen) < 0) {
        int errsv = errno;
        XLOGD_WARN("failed to set hci socket filter %d (%s)", errsv, strerror(errsv));

    } else if (filterLen != sizeof(filter)) {
        XLOGD_WARN("returned filter is not the expected size");

    } else if ( ((filter.type_mask & filterTypeMask) == filterTypeMask) &&
                ((filter.event_mask[0] & filterEvenMask[0]) == filterEvenMask[0]) &&
                ((filter.event_mask[1] & filterEvenMask[1]) == filterEvenMask[1]) ) {
        XLOGD_INFO("hci filter already matches, no need to reset");
        return true;
    }

    XLOGD_DEBUG("hci filter was [ type=0x%04x events={0x%08x, 0x%08x} ]",
           filter.type_mask, filter.event_mask[0], filter.event_mask[1]);

    XLOGD_INFO("setting hci filter to [ type=0x%04x events={0x%08x, 0x%08x} ]",
          filterTypeMask, filterEvenMask[0], filterEvenMask[1]);

    // setup filter for only receiving BLE meta events
    bzero(&filter, sizeof(filter));
    filter.type_mask = filterTypeMask;
    filter.event_mask[0] = filterEvenMask[0];
    filter.event_mask[1] = filterEvenMask[1];

    if (setsockopt(socketFd, SOL_HCI, HCI_FILTER, &filter, sizeof(filter)) < 0) {
        int errsv = errno;
        XLOGD_WARN("failed to set hci socket filter %d (%s)", errsv, strerror(errsv));
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Binds the HCI socket to the given hci device.

 */
bool HciSocketImpl::bindSocket(int socketFd, uint hciDeviceId) const
{
    // bind socket to the HCI device
    struct sockaddr_hci addr;
    bzero(&addr, sizeof(addr));
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = hciDeviceId;
    addr.hci_channel = HCI_CHANNEL_RAW;

    if (bind(socketFd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {

        // EALREADY is sent if we've already bound the socket, ignore this error
        if (errno != EALREADY) {
            int errsv = errno;
            XLOGD_WARN("failed to bind to hci socket %d (%s)", errsv, strerror(errsv));
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool HciSocket::isValid() const

    Returns \c true if the socket was successifully opened.
 */
bool HciSocketImpl::isValid() const
{
    return (m_hciSocket >= 0);
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Sends a command to the HCI device.
 */
bool HciSocketImpl::sendCommand(uint16_t ogf, uint16_t ocf, void *data, uint8_t dataLen)
{
    struct iovec vec[3];
    int vecLen = 0;

    uint8_t type = HCI_COMMAND_PKT;
    vec[0].iov_base = &type;
    vec[0].iov_len = 1;
    vecLen++;

    hci_command_hdr hdr;
    hdr.opcode = (uint16_t)((ocf & 0x03ff) | (ogf << 10));
    hdr.plen = dataLen;
    vec[1].iov_base = &hdr;
    vec[1].iov_len = HCI_COMMAND_HDR_SIZE;
    vecLen++;

    if (data && dataLen) {
        vec[2].iov_base = data;
        vec[2].iov_len  = dataLen;
        vecLen++;
    }

    ssize_t wr = TEMP_FAILURE_RETRY(::writev(m_hciSocket, vec, vecLen));
    if (wr < 0) {
        int errsv = errno;
        XLOGD_WARN("failed to write command %d (%s)", errsv, strerror(errsv));
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Checks that the supplied connection parameters are valid, this is borrowed
    from the kernel checker code.
 */
bool HciSocketImpl::checkConnectionParams(uint16_t minInterval,
                                          uint16_t maxInterval,
                                          uint16_t latency,
                                          uint16_t supervisionTimeout) const
{
    if ((minInterval > maxInterval) || (minInterval < 6) || (maxInterval > 3200))
        return false;

    if ((supervisionTimeout < 10) || (supervisionTimeout > 3200))
        return false;

    if (maxInterval >= (supervisionTimeout * 8))
        return false;

    uint16_t maxLatency = (supervisionTimeout * 8 / maxInterval) - 1;
    if ((latency > 499) || (latency > maxLatency))
        return false;

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \fn bool HciSocket::requestConnectionUpdate(const BleConnectionParameters &params)

    Sends a request to the HCI device to update the connection parameters.

    \sa Volume 2, Part E, Section 7.7.65.1 LE Connection Complete Event
    of the Bluetooth Core Spec version 4.0

 */
bool HciSocketImpl::requestConnectionUpdate(uint16_t connHandle,
                                            const BleConnectionParameters &params)
{
    // convert the parameters
    le_connection_update_cp connUpdate;
    bzero(&connUpdate, sizeof(connUpdate));

    connUpdate.min_interval        = uint16_t(params.minimumInterval() / 1.25f);
    connUpdate.max_interval        = uint16_t(params.maximumInterval() / 1.25f);
    connUpdate.latency             = uint16_t(params.latency());
    connUpdate.supervision_timeout = uint16_t(params.supervisionTimeout() / 10);

    // check the parameters
    if (!checkConnectionParams(connUpdate.min_interval, connUpdate.max_interval,
                               connUpdate.latency, connUpdate.supervision_timeout)) {
        XLOGD_WARN("invalid connection parameters, aborting request");
        return false;
    }

    connUpdate.handle = connHandle;

    // send the request to the kernel
    return sendCommand(OGF_LE_CTL, OCF_LE_CONN_UPDATE, &connUpdate,
                       LE_CONN_UPDATE_CP_SIZE);
}

// -----------------------------------------------------------------------------
/*!
    \fn bool HciSocket::sendIncreaseDataCapability()

    Sends a request to the HCI device to send the VSC to increase data 
    capability for bluetooth
 */
bool HciSocketImpl::sendIncreaseDataCapability(uint16_t connHandle)
{
    le_vsc_more_data_capability moreData;
    bzero(&moreData, sizeof(moreData));
    moreData.handle = connHandle;
    moreData.level = OCF_LE_MORE_DATA_CAPABILITY_LEVEL;
    bool ret = sendCommand(OGF_LE_VSC, OCF_LE_MORE_DATA, &moreData, LE_MORE_DATA_VSC_SIZE);
    XLOGD_WARN("Sent VSC MORE_DATA_CAPABILITY (0x%X) to handle: %d, return = %s", moreData.level, connHandle, ret ? "TRUE":"FALSE");
    return ret;
}

// -----------------------------------------------------------------------------
/*!
    \fn QList<HciSocket::ConnectedDeviceInfo> HciSocket::getConnectedDevices() const

    Returns a list of all the connected bluetooth LE devices.  On failure an
    empty list is returned, which is the same for if there are no actual
    attached devices.

 */
vector<HciSocket::ConnectedDeviceInfo> HciSocketImpl::getConnectedDevices() const
{
    vector<ConnectedDeviceInfo> devices;

    const uint16_t maxConns = 10;

    // create a buffer to store all the results from the ioctl
    uint8_t data[sizeof(struct hci_conn_list_req) + (maxConns * sizeof(struct hci_conn_info))];
    bzero(data, sizeof(data));

    struct hci_conn_list_req *req =
        reinterpret_cast<struct hci_conn_list_req*>(data);
    const struct hci_conn_info *info =
        reinterpret_cast<const struct hci_conn_info*>(data + sizeof(struct hci_conn_list_req));

    req->dev_id = m_hciDeviceId;
    req->conn_num = maxConns;

    // request a list of the all the connections
    int ret = TEMP_FAILURE_RETRY(::ioctl(m_hciSocket, HCIGETCONNLIST, data));
    if (ret != 0) {
        int errsv = errno;
        XLOGD_WARN("HCIGETCONNLIST ioctl failed %d (%s)", errsv, strerror(errsv));
        return devices;
    }

    // append any devices to the list
    for (uint16_t i = 0; i < req->conn_num; i++) {

        // we only care about bluetooth LE connections
        if (info[i].type != LE_LINK)
            continue;

        // get the bdaddr of the device
        BleAddress bdAddr(info[i].bdaddr.b, BleAddress::LSBOrder);
        ConnectedDeviceInfo bdInfo(std::move(bdAddr), info[i].handle,
                                   info[i].state, info[i].link_mode);

        // append the device
        devices.push_back(bdInfo);
    }

    return devices;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Returns the string for the given error / status code returned by the HCI
    interface.

    \sa Volume 2, Part D, Section 1.3 'List of Error Codes' of the Bluetooth
    Core Spec version 4.0

 */
const char* HciSocketImpl::hciErrorString(uint8_t code) const
{
    switch (code) {
        case 0x00:  return "Success";
        case 0x01:  return "Unknown HCI Command";
        case 0x02:  return "Unknown Connection Identifier";
        case 0x03:  return "Hardware Failure";
        case 0x04:  return "Page Timeout";
        case 0x05:  return "Authentication Failure";
        case 0x06:  return "PIN or Key Missing";
        case 0x07:  return "Memory Capacity Exceeded";
        case 0x08:  return "Connection Timeout";
        case 0x09:  return "Connection Limit Exceeded";
        case 0x0A:  return "Synchronous Connection Limit To A Device Exceeded";
        case 0x0B:  return "ACL Connection Already Exists";
        case 0x0C:  return "Command Disallowed";
        case 0x0D:  return "Connection Rejected due to Limited Resources";
        case 0x0E:  return "Connection Rejected Due To Security Reasons";
        case 0x0F:  return "Connection Rejected due to Unacceptable BD_ADDR";
        case 0x10:  return "Connection Accept Timeout Exceeded";
        case 0x11:  return "Unsupported Feature or Parameter Value";
        case 0x12:  return "Invalid HCI Command Parameters";
        case 0x13:  return "Remote User Terminated Connection";
        case 0x14:  return "Remote Device Terminated Connection due to Low Resources";
        case 0x15:  return "Remote Device Terminated Connection due to Power Off";
        case 0x16:  return "Connection Terminated By Local Host";
        case 0x17:  return "Repeated Attempts";
        case 0x18:  return "Pairing Not Allowed";
        case 0x19:  return "Unknown LMP PDU";
        case 0x1A:  return "Unsupported Remote Feature / Unsupported LMP Feature";
        case 0x1B:  return "SCO Offset Rejected";
        case 0x1C:  return "SCO Interval Rejected";
        case 0x1D:  return "SCO Air Mode Rejected";
        case 0x1E:  return "Invalid LMP Parameters / Invalid LL Parameters";
        case 0x1F:  return "Unspecified Error";
        case 0x20:  return "Unsupported LMP Parameter Value / Unsupported LL Parameter Value";
        case 0x21:  return "Role Change Not Allowed";
        case 0x22:  return "LMP Response Timeout / LL Response Timeout";
        case 0x23:  return "LMP Error Transaction Collision";
        case 0x24:  return "LMP PDU Not Allowed";
        case 0x25:  return "Encryption Mode Not Acceptable";
        case 0x26:  return "Link Key cannot be Changed";
        case 0x27:  return "Requested QoS Not Supported";
        case 0x28:  return "Instant Passed";
        case 0x29:  return "Pairing With Unit Key Not Supported";
        case 0x2A:  return "Different Transaction Collision";
        case 0x2C:  return "QoS Unacceptable Parameter";
        case 0x2D:  return "QoS Rejected";
        case 0x2E:  return "Channel Classification Not Supported";
        case 0x2F:  return "Insufficient Security";
        case 0x30:  return "Parameter Out Of Mandatory Range";
        case 0x32:  return "Role Switch Pending";
        case 0x34:  return "Reserved Slot Violation";
        case 0x35:  return "Role Switch Failed";
        case 0x36:  return "Extended Inquiry Response Too Large";
        case 0x37:  return "Secure Simple Pairing Not Supported By Host";
        case 0x38:  return "Host Busy - Pairing";
        case 0x39:  return "Connection Rejected due to No Suitable Channel Found";
        case 0x3A:  return "Controller Busy";
        case 0x3B:  return "Unacceptable Connection Parameters";
        case 0x3C:  return "Directed Advertising Timeout";
        case 0x3D:  return "Connection Terminated due to MIC Failure";
        case 0x3E:  return "Connection Failed to be Established";
        case 0x3F:  return "MAC Connection Failed";
        case 0x40:  return "Coarse Clock Adjustment Rejected but Will Try to Adjust Using Clock Dragging";
        default:    return "Unknown";
    }
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an \c EVT_LE_META_EVENT event has been received and the sub
    event type is \c EVT_LE_CONN_COMPLETE.

    This event is sent after the HCI device has successifully connected to a
    remote device.

    \sa Volume 2, Part E, Section 7.7.65.1 LE Connection Complete Event
    of the Bluetooth Core Spec version 4.0

 */
void HciSocketImpl::onConnectionCompleteEvent(const evt_le_connection_complete *event)
{
    XLOGD_DEBUG("EVT_LE_CONN_COMPLETE - { 0x%02hhx, %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx, %hu, %hu, %hu, %hu }",
           event->status,
           event->peer_bdaddr.b[5], event->peer_bdaddr.b[4],
           event->peer_bdaddr.b[3], event->peer_bdaddr.b[2],
           event->peer_bdaddr.b[1], event->peer_bdaddr.b[0],
           event->handle, event->interval, event->latency,
           event->supervision_timeout);

    // check the status of the connection
    if (event->status != 0x00) {
        XLOGD_WARN("connection failed (0x%02hhx - %s)", event->status,
                 hciErrorString(event->status));
        return;
    }

    // extract the remote device address
    BleAddress bdaddr(event->peer_bdaddr.b, BleAddress::LSBOrder);

    // extract the connection values
    const double intervalMs = double(event->interval) * 1.25;
    const int supervisionTimeoutMs = int(event->supervision_timeout) * 10;
    const int latency = event->latency;

    BleConnectionParameters params;
    params.setIntervalRange(intervalMs, intervalMs);
    params.setSupervisionTimeout(supervisionTimeoutMs);
    params.setLatency(latency);

}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an \c EVT_LE_META_EVENT event has been received and the sub
    event type is \c EVT_LE_CONN_UPDATE_COMPLETE.

    This event is sent after the HCI device has update the connection parameters
    for a given connection.  We extract the fields and create an
    \l{HciConnectionParameters} object from them before emitting the
    \l{connectionUpdated,HciSocketImpl::connectionUpdated} signal.

    \sa Volume 2, Part E, Section 7.7.65.3 LE Connection Update Complete Event
    of the Bluetooth Core Spec version 4.0

 */
void HciSocketImpl::onUpdateCompleteEvent(const evt_le_connection_update_complete *event)
{
    XLOGD_DEBUG("EVT_LE_CONN_UPDATE_COMPLETE - { 0x%02hhx, %hu, %hu, %hu, %hu }",
           event->status, event->handle, event->interval, event->latency,
           event->supervision_timeout);

    if (event->status != 0x00) {
        XLOGD_WARN("update connection failed (0x%02hhx - %s)", event->status,
                 hciErrorString(event->status));
        return;
    }

    // extract the connection values
    const double intervalMs = double(event->interval) * 1.25;
    const int supervisionTimeoutMs = int(event->supervision_timeout) * 10;
    const int latency = event->latency;

    BleConnectionParameters params;
    params.setIntervalRange(intervalMs, intervalMs);
    params.setSupervisionTimeout(supervisionTimeoutMs);
    params.setLatency(latency);

}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when an \c EVT_DISCONN_COMPLETE event has been received.

    \sa Volume 2, Part E, 7.7.5 Disconnection Complete Event of the Bluetooth
    Core Spec version 4.0

 */
void HciSocketImpl::onDisconnectionCompleteEvent(const evt_disconn_complete *event)
{
    XLOGD_DEBUG("EVT_DISCONN_COMPLETE - { 0x%02hhx, %hu, 0x%02hhx }",
           event->status, event->handle, event->reason);

    if (event->status != 0x00) {
        XLOGD_WARN("disconnection failed (0x%02hhx - %s)", event->status,
                 hciErrorString(event->status));
        return;
    }

}

// -----------------------------------------------------------------------------
/*!
    \internal

    Called when the HCI socket is readable (i.e. an event is in the queue). The
    event is read and then passed onto one of the event handlers.

 */
void HciSocketImpl::onSocketActivated()
{
    // read an event from the buffer
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    ssize_t len = TEMP_FAILURE_RETRY(recv(m_hciSocket, buf, HCI_MAX_EVENT_SIZE, MSG_DONTWAIT));
    if (len < 0) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            int errsv = errno;
            XLOGD_WARN("failed to read from hci socket %d (%s)", errsv, strerror(errsv));
        }
        return;
    }

    if (len == 0) {
        XLOGD_WARN("read from hci socket returned 0 bytes");
        return;
    }

    // check the message is an HCI event
    if (buf[0] != HCI_EVENT_PKT) {
        XLOGD_INFO("odd, received non-event message even though it should be filtered out");
        return;
    }
    len -= HCI_TYPE_LEN;

    // check we have enough for the header
    if (len < HCI_EVENT_HDR_SIZE) {
        XLOGD_WARN("read too short message from hci socket (read %zd bytes)", len);
        return;
    }
    len -= HCI_EVENT_HDR_SIZE;

    // get the message header and sanity check the length
    const hci_event_hdr *hdr = reinterpret_cast<const hci_event_hdr*>(buf + HCI_TYPE_LEN);
    if (hdr->plen != len) {
        XLOGD_WARN("size of received event doesn't match header value");
        return;
    }


    // check if a disconnect event
    if (hdr->evt == EVT_DISCONN_COMPLETE) {

        if (len < EVT_DISCONN_COMPLETE_SIZE) {
            XLOGD_WARN("disconnect event EVT_DISCONN_COMPLETE has invalid size "
                     "(expected:%u actual:%zd)", EVT_DISCONN_COMPLETE_SIZE, len);
            return;
        }

        const evt_disconn_complete *disconnEvent =
            reinterpret_cast<const evt_disconn_complete*>(buf + HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE);

        onDisconnectionCompleteEvent(disconnEvent);


    // check if a meta event
    } else if (hdr->evt == EVT_LE_META_EVENT) {

        if (len < EVT_LE_META_EVENT_SIZE) {
            XLOGD_WARN("le meta event EVT_LE_META_EVENT has invalid size "
                     "(expected:%u actual:%zd)", EVT_LE_META_EVENT_SIZE, len);
            return;
        }
        len -= EVT_LE_META_EVENT_SIZE;

        const evt_le_meta_event *metaEvt =
            reinterpret_cast<const evt_le_meta_event*>(buf + HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE);

        if (metaEvt->subevent == EVT_LE_CONN_COMPLETE) {

            // sanity check the length of the sub event
            if (len < EVT_LE_CONN_COMPLETE_SIZE) {
                XLOGD_WARN("le meta event EVT_LE_CONN_COMPLETE has invalid size "
                         "(expected:%u actual:%zd)", EVT_LE_CONN_COMPLETE_SIZE, len);
                return;
            }

            // pass the event onto the handler
            const evt_le_connection_complete *leConnComplt =
                reinterpret_cast<const evt_le_connection_complete*>(metaEvt->data);

            onConnectionCompleteEvent(leConnComplt);

        } else if (metaEvt->subevent == EVT_LE_CONN_UPDATE_COMPLETE) {

            // sanity check the length of the sub event
            if (len < EVT_LE_CONN_UPDATE_COMPLETE_SIZE) {
                XLOGD_WARN("le meta event EVT_LE_CONN_UPDATE_COMPLETE_SIZE has invalid size"
                         "(expected:%u actual:%zd)", EVT_LE_CONN_UPDATE_COMPLETE_SIZE, len);
                return;
            }

            // pass the event onto the handler
            const evt_le_connection_update_complete *leUpdateComplt =
                reinterpret_cast<const evt_le_connection_update_complete*>(metaEvt->data);

            onUpdateCompleteEvent(leUpdateComplt);
        }
    }
}


void *SocketThread(void *data)
{
    HciSocketImpl *socketImpl = (HciSocketImpl *)data;
    shared_ptr<bool> isAlive = socketImpl->m_isAlive;

    fd_set rfds;
    int nfds = -1;
    bool running = true;

    // Unblock the caller that launched this thread
    sem_post(&socketImpl->m_socketThreadSem);

    XLOGD_INFO("Enter main loop for HCI socket thread");
    do {
        // Needs to be reinitialized before each call to select() because select() will modify these variables
        FD_ZERO(&rfds);
        FD_SET(FD_RECV(socketImpl->m_exitEventFds), &rfds);
        nfds = FD_RECV(socketImpl->m_exitEventFds);

        FD_SET(socketImpl->m_hciSocket,  &rfds);
        nfds = std::max(nfds, socketImpl->m_hciSocket);
        nfds++;

        int ret = select(nfds, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            int errsv = errno;
            XLOGD_DEBUG("select() failed: error = <%d>, <%s>", errsv, strerror(errsv));
            continue;
        }

        if (*isAlive && FD_ISSET(FD_RECV(socketImpl->m_exitEventFds), &rfds)) {
            XLOGD_DEBUG("received exit signal");
            ClearEventFd(FD_RECV(socketImpl->m_exitEventFds));
            running = false;
            break;
        }

        if (*isAlive && FD_ISSET(socketImpl->m_hciSocket, &rfds)) {
            socketImpl->onSocketActivated();
        }

    } while (running && *isAlive);

    if (*isAlive == false) {
        XLOGD_ERROR("HciSocketImpl object has been destroyed before thread exited.  Suspect something went wrong, exiting...");
    } else if (!running) {
        XLOGD_INFO("HCI socket thread exited gracefully.");
    } else {
        XLOGD_ERROR("HCI socket thread exited unexpectedly, suspect an error occurred...");
    }

    return NULL;
}
