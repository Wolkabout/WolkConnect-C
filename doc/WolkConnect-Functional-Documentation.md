The goal of this document is to provide a high-level overview of WolkConnect libraries.

WolkConnect libraries are used to enable a device’s communication with WolkAbout IoT Platform.
Using WolkConnect libraries in the software or firmware of a device will drastically decrease the time to market for developers or anyone wanting to integrate their own product with WolkAbout IoT Platform.

The available WolkConnect libraries (C, C++, Java, Python) are platform independent, with a special note that the WolkConnect-C library is suitable to be adapted for use on bare-metal devices.

Features of WolkAbout IoT Platform that have been incorporated into WolkConnect libraries will be disambiguated with information on how to perform these features on devices by using WolkConnect’s API.
Conception
The intention of the WolkConnect library is to be used as a dependency in other firmwares or softwares that have their own existing business logic. 
WolkConnect library is not by any means a single service to control the device.

Using WolkConnect library does not require knowledge of WolkAbout IoT Platform. 
The user only utilises APIs provided by the User Application Layer, thereby reducing time-to-market required. 

Providing WolkConnect library with IP connectivity from the Hardware Abstraction Layer is expected from the user.

The gray section of Fig.1.1  represents the developer’s software/firmware in a programing language supported by WolkConnect library

Fig.1.1 WolkConnect library represented in general software/firmware architecture

Ovde kreni sada u opis samog konektora, iz cega se on sastoji. Imas 4 celine, pa mozda onda ova slika 1.1. Kazi da je sva komunikacija kriptovana.

A common dependency for WolkConnect libraries is a JSON library used for parsing data that is exchanged with WolkAbout IoT Platform. This data is formatted using a custom JSON based protocol defined by WolkAbout IoT Platform.

Communication between WolkConnect library and WolkAbout IoT Platform is achieved through the use of the MQTT protocol. WolkConnect libraries have another common dependency, an implementation of an MQTT client that will exchange data with an MQTT server that is part of WolkAbout IoT Platform. 
The communication between WolkConnect library and WolkAbout IoT Platform is made secure with the use of Transport Layer Security (TLS).

WolkConnect libraries use network connectivity provided by the OS, but on devices where these calls are not available it is the user’s responsibility to provide implementations for opening a socket and send/receive methods to the socket.


Picture 1.1. WolkConnect library layers

All the business logic has to be implemented by the user, WolkConnect libraries should simply be considered a third party dependency to enable communication to WolkAbout IoT Platform.
Connect and disconnect

Every connection from a WolkConnect library to WolkAbout IoT Platform is authenticated with a device key and a device password. These credentials are created on WolkAbout IoT Platform when a device is created and are unique to that device. Only one active connection is allowed per device.
Attempting to create an additional connection with the same device credentials will terminate the previous connection. The connection is made secure by default in all WolkConnect libraries through the use of Transport Layer Security (TLS). Connecting without TLS is possible, refer to a specific WolkConnect library’s connect method for more information.

Disconnecting will gracefully terminate the connection, but in the cases of ungraceful disconnections, eg. due to a networking error, a last will message will be broadcast. This last will message will be responsible for immediately declaring the device offline on WolkAbout IoT Platform.
Device

WolkConnect libraries and WolkAbout IoT Platform separate a device’s functionality into two distinct parts:
Device data - represents information that is being communicated to WolkAbout IoT Platform
Device management - dictates the behaviour of how the device operates
Device Data

Real world devices can perform a wide variety of operations that result in meaningful data. These operations could be to conduct a measurement, monitor a certain condition or execute some form of command. The data resulting from these operations have been modeled into three distinct types of data on WolkAbout IoT Platform: sensors, alarms, and actuators.

Information needs to be distinguishable, so every piece of data sent from the device needs to have an identifier. This identifier is called a reference, and all references of a device must be unique.
Sensors

Sensor readings are stored on the device before explicitly being published to WolkAbout IoT Platform. If the exact time when the reading occured is meaningful information, it can be assigned to the reading as a UTC timestamp. If this timestamp is not provided, WolkAbout IoT Platform will assign the reading a timestamp when it has been received, treating the reading like it occured the moment it arrived.

Readings could be of a very high precision, and although this might not be fully displayed on the dashboard, the information is not lost and can be viewed on different parts of WolkAbout IoT Platform.

Sensors readings like GPS and accelerometers hold more than one single information and these types of readings are supported in WolkConnect libraries and on WolkAbout IoT Platform.
See documentation of a specific WolkConnect library on how to create multi-value readings in different programming languages.
Alarms

Alarms are derived from some data on the device and are used to indicate the state of a condition, so the alarm value can either be on or off. Like sensor readings, alarm messages are stored on the device before being published to WolkAbout IoT Platform. Alarms can also have a UTC timestamp to denote when the alarm occurred, but if the timestamp is omitted then WolkAbout IoT Platform will assign a timestamp when it receives the alarm message.

Actuators

Actuators are used to enable WolkAbout IoT Platform to set the state of some part of the device, eg. flip a switch or change the gear of a motor.
In order to achieve this, Wolkabout IoT Platform needs to be aware of what the current state of the actuator is. Here, it is the user’s responsibility to implement an actuator status provider that will report the current value and state of the actuator to WolkAbout IoT Platform.

The possible actuator states are:

READY - waiting to receive a command to change its value
BUSY - in the process of changing its value
ERROR

Now that the device has notified WolkAbout IoT Platform about the current state of its actuators, it needs to be able to listen for commands issued from WolkAbout IoT Platform.
First, a list of actuator references that are available on the device needs to be provided when connecting to WolkAbout IoT Platform in order to subscribe to messages containing actuation commands.
Second, the user has to implement an actuation handler that will execute the commands that have been issued from WolkAbout IoT Platform.

To summarise, when an actuation command is issued from WolkAbout IoT Platform, it will be passed to the actuation handler that will attempt to execute the command, and then the actuator status provider will report back to WolkAbout IoT Platform with the current value and state of the actuator.

Publishing of actuator statuses is performed immediately, but if the device is unable to publish then the information will be stored on the device until the next successful publish.
Device Management


Keep Alive Mechanism

In cases where the device is connected to the platform but is not publishing any data for prolonged periods of time, the device may be declared offline. This is especially true for devices that only have actuators for example. To prevent this issue, a keep alive mechanism will periodically send a message to WolkAbout IoT Platform. This mechanism can also be disabled to reduce bandwidth usage.

Configuration Options

Configuration options are enabled on the device by the user’s implementation of a configuration provider and a configuration handler. All configuration options are sent initially from the device as a single information and after that new configuration values can be issued from WolkAbout IoT Platform. Configuration options are always sent as a whole, even when only one value changes.

Device Firmware Update

WolkAbout IoT Platform gives the possibility of updating the firmware of a device. In order to update the firmware, the user must create a firmware handler.
This firmware handler will specify the following parameters:
current firmware version,
desired size of firmware chunk to be received from WolkAbout IoT Platform,
maximum supported firmware file size,
download location,
implementation of a firmware installer that will be responsible for the installation process.
Optionally, an implementation of firmware download handler that will download a file from an URL issued from WolkAbout IoT Platform.
