#include <hal/CAN.h>
#include <hal/DriverStation.h>
#include <hal/Extensions.h>
#include "HALSIM_SocketCAN.h"
#include <wpinet/EventLoopRunner.h>
#include <wpinet/uv/Timer.h>
#include <cstdio>
#include <cstdlib>

static std::unique_ptr<wpi::EventLoopRunner> eventLoopRunner;
void SetupEventLoop(wpi::uv::Loop& loop) {
  auto canHeartbeat = wpi::uv::Timer::Create(loop);
  canHeartbeat->timeout.connect([] {
    auto allianceStation = HAL_GetAllianceStation(nullptr);
    uint8_t redAlliance = 0;
    if (allianceStation < 4) {
      redAlliance = 1;
    }
    HAL_ControlWord controlWord;
    HAL_GetControlWord(&controlWord);
    uint8_t data[] = {redAlliance, static_cast<uint8_t>(controlWord.enabled), static_cast<uint8_t>(controlWord.autonomous),
                      static_cast<uint8_t>(controlWord.test), static_cast<uint8_t>(HAL_GetOutputsEnabled()), 0, 0, 0};
    int32_t status;
    HAL_CAN_SendMessage(0x01011840, data, 8, HAL_CAN_SEND_PERIOD_NO_REPEAT, &status);
  });
  canHeartbeat->Start(wpi::uv::Timer::Time(100), wpi::uv::Timer::Time(100));
}
extern "C" {
#if defined(WIN32) || defined(_WIN32)
__declspec(dllexport)
#endif
    int HALSIM_InitExtension(void) {
  std::puts("SocketCAN Initializing.");

  HALSIM_SocketCAN_Handle handle = HALSIM_SocketCAN_Enable(std::getenv("SOCKETCAN_INTERFACE"));
  if (handle == nullptr) {
    std::puts("Failed to initialize SocketCAN");
  } else {
    eventLoopRunner = std::make_unique<wpi::EventLoopRunner>();
    eventLoopRunner->ExecAsync(SetupEventLoop);
    std::puts("SocketCAN Initialized!");
  }
  return 0;
}
}  // extern "C"
