
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */
#include "esp_log.h"
#include <N2kMsg.h>
#include <NMEA2000.h> // https://github.com/ttlappalainen/NMEA2000
#include "sdkconfig.h"
#define ESP32_CAN_TX_PIN (gpio_num_t) CONFIG_ESP32_CAN_TX_PIN // from sdkconfig (idf menuconfig)
#define ESP32_CAN_RX_PIN (gpio_num_t) CONFIG_ESP32_CAN_RX_PIN
#include <NMEA2000_esp32.h> // https://github.com/ttlappalainen/NMEA2000_esp32
#include "N2kMessages.h"
#include "ESP32N2kStream.h"
#include "N2kGroupFunctionBinaryStatus.h" // this file was not included in the library

static const char *TAG = "my_N2K_lib";
tNMEA2000_esp32 NMEA2000;

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] = {127501UL, 0};
const unsigned long ReceiveMessages[] = {127502UL, 0};

static TaskHandle_t N2K_task_handle = NULL;
static tN2kBinaryStatus switchBanks[2]; // each Bank contains up to 28 Items
static bool statusNeedsUpdate;
#define BinStatusUpdatePeriod 2500
#define BinStatusUpdateOffset 100
tN2kSyncScheduler binStatusScheduler(false, BinStatusUpdatePeriod, BinStatusUpdateOffset);

static uint8_t device_instance = 1;
// GroupFunctionHandlerForPGN127501 can ask if we have a particular instance
bool HasInstance(uint8_t _Instance)
{
    return (_Instance == device_instance);
}
// GroupFunctionHandlerForPGN127501 can change our particular instance
// Function should change _NewInstance for _OldInstance.
bool ChangeInstance(uint8_t _OldInstance, uint8_t _NewInstance)
{
    if (_OldInstance == device_instance)
    {
        // only change instance if match _OldInstance
        device_instance = _NewInstance;
        return true;
    }
    return false;
}

// GroupFunctionHandlerForPGN127501 can change the transmission interval(period) and offset
bool binStatusChangeTransmissionInterval(uint8_t _Instance, uint32_t TransmissionInterval, uint16_t TransmissionIntervalOffset)
{
    binStatusScheduler.SetPeriodAndOffset(TransmissionInterval, TransmissionIntervalOffset);
    return true;
};

// Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
void OnN2kOpen()
{
    // Start schedulers now.
    binStatusScheduler.UpdateNextTime();
}

#define MAX_SWITCHES (32)
extern "C" void setSwitchState(int index, bool OnOff)
{
    if (index >= MAX_SWITCHES)
    {
        return;
    }
    tN2kOnOff itemStatus = (OnOff) ? (N2kOnOff_On) : (N2kOnOff_Off);
    int bank = index / 28;
    int indexInBank = index % 28;
    N2kSetStatusBinaryOnStatus(switchBanks[bank], itemStatus, indexInBank + 1);
    statusNeedsUpdate = true; // set a flag to send the updated status back to the bus
}
extern "C" bool getSwitchState(int index)
{
    if (index >= MAX_SWITCHES)
    {
        return 0;
    }
    int bank = index / 28;
    int indexInBank = index % 28;
    tN2kOnOff itemStatus = N2kGetStatusOnBinaryStatus(switchBanks[bank], indexInBank + 1);
    return (itemStatus == N2kOnOff_On);
}

void SendN2kBinaryStatus()
{
    tN2kMsg N2kMsg;
    SetN2kBinaryStatus(N2kMsg, 0, switchBanks[0]);
    NMEA2000.SendMsg(N2kMsg, 0);
    SetN2kBinaryStatus(N2kMsg, 1, switchBanks[1]);
    NMEA2000.SendMsg(N2kMsg, 0);
}

void TaskN2kBinStatus()
{
    if ((statusNeedsUpdate) || (binStatusScheduler.IsTime()))
    {
        binStatusScheduler.UpdateNextTime();
        SendN2kBinaryStatus();
        statusNeedsUpdate = false;
    }
}

// NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg)
{
    if (N2kMsg.PGN == 127502L)
    {
        uint8_t bank;
        tN2kBinaryStatus inStatus;
        ParseN2kSwitchbankControl(N2kMsg, bank, inStatus);
        ESP_LOGD(TAG, "PGN: %ld, bank: %d, status: %0llx,", N2kMsg.PGN, bank, inStatus);
        for (int i = 0; i < 28; i++)
        {
            tN2kOnOff status = (tN2kOnOff)(inStatus & 0x03);
            inStatus >>= 2;
            if (status != N2kOnOff_Unavailable) // if get 0b11 status means don't change
            {
                N2kSetStatusBinaryOnStatus(switchBanks[bank], status, i + 1);
                statusNeedsUpdate = true; // set a flag to send the updated status back to the bus
            }
        }
    }
}

// If SetNext is true, function must set next binary instace message from given instance and set _Instance to set instance.
// If there is not next, function return false and set _Instance to 0xff
// If _Instance is 0xff, function must return binary status for firts instance.
bool SetBinaryStatusMessage(uint8_t &_Instance, tN2kMsg &N2kMsg, bool SetNext)
{
    // TODO ???
    return false;
}

// If there is request for 127501, you must respond to it.
bool ISORequestHandler(unsigned long RequestedPGN, unsigned char Requester, int DeviceIndex)
{
    if (RequestedPGN == 127501UL)
    {
        SendN2kBinaryStatus();
        return true;
    }

    return false;
}

// This is a FreeRTOS task
void N2K_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting task");
    statusNeedsUpdate = false;
    N2kResetBinaryStatus(switchBanks[0]);
    N2kResetBinaryStatus(switchBanks[1]);
    for (int i = 0; i < MAX_SWITCHES; i++)
    {
        setSwitchState(i, false); // All switches off
    }

    NMEA2000.SetN2kCANMsgBufSize(8);
    NMEA2000.SetN2kCANReceiveFrameBufSize(100);

    // Set Product information
    NMEA2000.SetProductInformation("112233",               // Manufacturer's Model serial code.
                                   100,                    // Manufacturer's product code
                                   "Simple switch bank",   // Manufacturer's Model ID
                                   "1.0.0.2 (2017-06-13)", // Manufacturer's Software version code
                                   "1.0.0.0 (2017-06-13)", // Manufacturer's Model version
                                   0xff,                   // load equivalency - use default
                                   0xffff,                 // NMEA 2000 version - use default
                                   0xff,                   // Certification level - use default
                                   0);

    // Set device information. For binary control device use class 30 and function 140 for SetDeviceInformation. It is informative for other devices what you do.
    NMEA2000.SetDeviceInformation(112233, // Unique number. Use e.g. Serial number.
                                  140,    // Device function=Load Controller. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                  75,     // Device class=Electrical Distribution. See codes on  https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                  2040,   // Just choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                                  4,      // Marine
                                  0);

    NMEA2000.SetForwardStream(new ESP32N2kStream()); // PC output on native port
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);   // Show in clear text
    // NMEA2000.EnableForward(false);                 // Disable all msg forwarding to USB (=Serial)

    NMEA2000.SetMsgHandler(HandleNMEA2000Msg);

    // You must also create ISO Request handler and configure it with NMEA2000.SetISORqstHandler(..); in initialization.
    NMEA2000.SetISORqstHandler(ISORequestHandler);

    // You should also handle group function messages 126208.
    NMEA2000.AddGroupFunctionHandler(new tN2kGroupFunctionHandlerForPGN127501(&NMEA2000,
                                                                              &HasInstance,
                                                                              &SetBinaryStatusMessage,
                                                                              &ChangeInstance,
                                                                              &binStatusChangeTransmissionInterval));

    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 22);

    // Here we tell, which PGNs we transmit and receive
    NMEA2000.ExtendTransmitMessages(TransmitMessages, 0);
    NMEA2000.ExtendReceiveMessages(ReceiveMessages, 0);

    // Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
    NMEA2000.SetOnOpen(OnN2kOpen);
    NMEA2000.Open();
    for (;;)
    {
        // put your main code here, to run repeatedly:
        TaskN2kBinStatus();
        NMEA2000.ParseMessages();
        vTaskDelay(1); // yield for 1ms (make sure to set FreeRTOS tick rate to 1000Hz in menuconfig)
    }
    vTaskDelete(NULL); // should never get here...
}

// was setup() in Arduino example:
extern "C" int my_N2K_lib_init(void)
{
    esp_err_t result = ESP_OK;
    ESP_LOGV(TAG, "create task");
    xTaskCreate(
        &N2K_task,            // Pointer to the task entry function.
        "N2K_task",           // A descriptive name for the task for debugging.
        3072,                 // size of the task stack in bytes.
        NULL,                 // Optional pointer to pvParameters
        tskIDLE_PRIORITY + 3, // priority at which the task should run
        &N2K_task_handle      // Optional pass back task handle
    );
    if (N2K_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Unable to create task.");
        result = ESP_ERR_NO_MEM;
        goto err_out;
    }

err_out:
    if (result != ESP_OK)
    {
        if (N2K_task_handle != NULL)
        {
            vTaskDelete(N2K_task_handle);
            N2K_task_handle = NULL;
        }
    }

    return result;
}
