#include "HALSIM_SocketCAN.h"

#ifdef __linux__
#include "CANController.h"
#include "hal/CAN.h"
#include "hal/simulation/CanData.h"
#include "hal/Errors.h"
#include "hal/HALBase.h"
#include <cstring>
#include <unordered_map>
#include <mutex>

namespace
{
struct StreamSession
{
    uint32_t messageID;
    uint32_t messageIDMask;
    uint32_t maxMessages;
};

struct HALSIM_SocketCAN
{
    CANController controller;
    int32_t sendMessageHandle;
    int32_t receiveMessageHandle;
    int32_t canStatusCallback;
    int32_t openStreamCallback;
    int32_t readStreamCallback;
    int32_t closeStreamCallback;

    std::unordered_map<uint32_t, StreamSession*> sessions;
    uint32_t session_ctr = 0;
    std::mutex session_lock;
};


} // namespace


static void SendMessageCallback(const char *name, void *param,
                                uint32_t messageID,
                                const uint8_t *data,
                                uint8_t dataSize, int32_t periodMs,
                                int32_t *status)
{
    HALSIM_SocketCAN& controller = *static_cast<HALSIM_SocketCAN*>(param);
    CANData canData;
    std::memcpy(canData.data, data, dataSize);
    canData.length = dataSize;
    canData.id = messageID;
    // TODO: Handle Repeating
    controller.controller.putData(canData);
    *status = HAL_SUCCESS;
}

static void ReceiveMessageCallback(const char *name, void *param, uint32_t *messageID, uint32_t messageIDMask,
                                   uint8_t *data, uint8_t *dataSize, uint32_t *timeStamp, int32_t *status)
{
    HALSIM_SocketCAN& controller = *static_cast<HALSIM_SocketCAN*>(param);
    auto readData = controller.controller.getData(*messageID,messageIDMask);
    if (!readData.has_value()) {
        *status = HAL_ERR_CANSessionMux_MessageNotFound;
        return;
    }

    std::memcpy(data, readData->data, readData->length);
    *dataSize = readData->length;
    *timeStamp = readData->timestamp;
    *messageID = readData->id;
    *status = HAL_SUCCESS;
}

static void StatusCallback(const char *name, void *param, float *percentBusUtilization,
                           uint32_t *busOffCount, uint32_t *txFullCount, uint32_t *receiveErrorCount,
                           uint32_t *transmitErrorCount, int32_t *status)
{
    *status = HAL_SUCCESS;
}

static void OpenStreamCallback(const char* name, void* param, uint32_t* sessionHandle, uint32_t messageID,
                               uint32_t messageIDMask, uint32_t maxMessages, int32_t* status)
{
    HALSIM_SocketCAN& controller = *static_cast<HALSIM_SocketCAN*>(param);
    StreamSession* ses = new StreamSession{};
    ses->messageID = messageID;
    ses->messageIDMask = messageIDMask;
    ses->maxMessages = maxMessages;
    {
        std::lock_guard<std::mutex> guard(controller.session_lock);
        *sessionHandle = controller.session_ctr++;
        // register the handle
        controller.sessions[*sessionHandle] = ses;
    }
    *status = HAL_SUCCESS;
}

static void ReadStreamCallback(const char* name, void* param, uint32_t sessionHandle,
                                struct HAL_CANStreamMessage* messages, uint32_t messagesToRead,
                                uint32_t* messagesRead, int32_t* status) 
{
    HALSIM_SocketCAN& controller = *static_cast<HALSIM_SocketCAN*>(param);
    if (!controller.sessions.contains(sessionHandle)) 
    {
        *status = HAL_HANDLE_ERROR;
        return;
    }
    std::lock_guard<std::mutex> guard(controller.session_lock);

    StreamSession& ses = *static_cast<StreamSession*>(controller.sessions[sessionHandle]);
    uint32_t i = 0;
    for (i = 0; i < messagesToRead; i++) 
    {
        auto readData = controller.controller.getData(ses.messageID, ses.messageIDMask);
        if (!readData.has_value()) {
            break;
        }
        std::memcpy(messages[i].data, readData->data, readData->length);
        messages[i].dataSize = readData->length;
        messages[i].messageID = readData->id;
        // sobbing
        messages[i].timeStamp = readData->timestamp;

    }
    *messagesRead = i;
    *status = HAL_SUCCESS;
}

static void CloseStreamCallback(const char* name, void* param, uint32_t sessionHandle)
{
    HALSIM_SocketCAN& controller = *static_cast<HALSIM_SocketCAN*>(param);
    std::lock_guard<std::mutex> guard(controller.session_lock);
    if (!controller.sessions.contains(sessionHandle)) return;
    StreamSession* ses = controller.sessions[sessionHandle];
    controller.sessions.erase(sessionHandle);
    delete ses;
}

extern "C"
{
    HALSIM_SocketCAN_Handle HALSIM_SocketCAN_Enable(const char *name)
    {
        // if (HAL_GetRuntimeType() != HAL_Runtime_Simulation) return nullptr;
        auto controller = new HALSIM_SocketCAN{};
        if (controller->controller.start(name) < 0)
        {
            delete controller;
            return nullptr;
        }

        controller->sendMessageHandle = HALSIM_RegisterCanSendMessageCallback(SendMessageCallback, controller);
        controller->receiveMessageHandle = HALSIM_RegisterCanReceiveMessageCallback(ReceiveMessageCallback, controller);
        controller->canStatusCallback = HALSIM_RegisterCanGetCANStatusCallback(StatusCallback, controller);

        controller->openStreamCallback = HALSIM_RegisterCanOpenStreamCallback(OpenStreamCallback, controller);
        controller->readStreamCallback = HALSIM_RegisterCanReadStreamCallback(ReadStreamCallback, controller);
        controller->closeStreamCallback = HALSIM_RegisterCanCloseStreamCallback(CloseStreamCallback, controller);

        return controller;
    }
    void HALSIM_SocketCAN_Clean(HALSIM_SocketCAN_Handle handle)
    {
        //if (HAL_GetRuntimeType() != HAL_Runtime_Simulation) return;
        HALSIM_SocketCAN* controller = static_cast<HALSIM_SocketCAN *>(handle);
        HALSIM_CancelCanGetCANStatusCallback(controller->canStatusCallback);
        HALSIM_CancelCanReceiveMessageCallback(controller->receiveMessageHandle);
        HALSIM_CancelCanSendMessageCallback(controller->sendMessageHandle);

        HALSIM_CancelCanOpenStreamCallback(controller->openStreamCallback);
        HALSIM_CancelCanReadStreamCallback(controller->readStreamCallback);
        HALSIM_CancelCanCloseStreamCallback(controller->closeStreamCallback);
        delete controller;
    }
}
#else

// this isn't linux, so noop the entire library
HALSIM_SocketCAN_Handle HALSIM_SocketCAN_Enable(const char* name) { return nullptr; };
void HALSIM_SocketCAN_Clean(HALSIM_SocketCAN_Handle handle) {};
#endif
