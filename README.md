# HALSIM_SocketCAN

This binds SocketCAN listeners to HALSIM CAN functions. As SocketCAN is Linux only, this vendordep only functions on Linux platforms that are in simulation mode.
Supports both Java and C++.

Applications:
 * Hardware attached integration testing with CAN devices
 * using WPILib on something that's not a roboRIO
 * using real CAN with a Romi or something similar (may require additional work)


# Building/Installing

1. copy [HALSIM_SocketCAN.json](HALSIM_SocketCAN.json) to your project's vendordep json folder

2. run
```bash
# Feel free to change 2023 to the correct year
./gradlew publishToMavenLocal && cp -r ~/.m2/repository/halsim ~/wpilib/2023/maven
```
in this repo

Eventually there will be a proper maven sever...hopefully.

# General running tips

FRC non-FD buses (that is, everything except the CANivore) uses 1 Mbps CAN so to configure a SocketCAN interface on ``can0`` one might do:

```bash
sudo ip link set can0 type can bitrate 1000000 && sudo ip link set can0 up
```
to provision ``can0``

# Running in C++

Assuming the SocketCAN interface is ``can0`` -- consult your local ``ip link`` to be sure.

```cpp
#include <HALSIM_SocketCAN.h>

int main() {
  // make sure that CAN-using objects are not instantiated first via static fields
  HAL_Initialize(500, 0);
  HALSIM_SocketCAN_Enable("can0");
  return frc::StartRobot<Robot>();
}
```

# Running in Java
```java
import halsim.socketcan.HALSIM_SocketCAN;

// edit your Main.java as such:

public static void main(String... args) {
if (!HAL.initialize(500, 0)) {
    throw new IllegalStateException("Failed to initialize. Terminating");
}
HALSIM_SocketCAN.enable("can0");
RobotBase.startRobot(Robot::new);
}
```


## Building and editing this vendordep
This uses gradle, and uses the same base setup as a standard GradleRIO robot project. This means you build with `./gradlew build`, and can install the native toolchain with `./gradlew installRoboRIOToolchain`. If you open this project in VS Code with the wpilib extension installed, you will get intellisense set up for both C++ and Java.

By default, this template builds against the latest WPILib development build. To build against the last WPILib tagged release, build with `./gradlew build -PreleaseMode`.
