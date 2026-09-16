#include "PLG_transport_ble.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/stream_buffer.h> // Arduino.h chi keo san task/queue/semaphore, khong co stream buffer
#include "PLG_protocol.h"

// Nordic UART Service (NUS) - xem giai thich lua chon o PLG_transport_ble.h.
// Ten "RX"/"TX" dat theo goc nhin CUA BOARD: RX = board nhan, TX = board gui.
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

static NimBLECharacteristic *tx_char = nullptr;
static volatile bool ble_connected = false;
static bool ble_started = false;

// Hang doi byte tho giua callback BLE va loop().
//
// Callback BLE chay trong task RIENG cua NimBLE, khong phai trong loop(). Neu goi thang
// protocol_feed() tu callback thi no se ghi vao chart_cpu[]/current_time_str... DUNG LUC
// loop() dang doc cac bien do de ve man hinh -> tranh chap du lieu, co the ve ra so sai
// hoac hong chuoi. Nen callback chi bo byte tho vao StreamBuffer (co san trong FreeRTOS,
// an toan cho 1 task ghi + 1 task doc), con viec parse de loop() lam qua transport_ble_poll().
static StreamBufferHandle_t rx_stream = nullptr;
static const size_t RX_STREAM_SIZE = 512; // ~4 dong du lieu, du dem khi loop() ban ve man hinh

// Gui tra loi bat tay nguoc lai PC qua characteristic NOTIFY. Goi tu loop() (qua
// protocol_feed), khong phai tu callback BLE.
static void ble_reply(const char *text)
{
    if (tx_char == nullptr || !ble_connected)
        return;
    tx_char->setValue((const uint8_t *)text, strlen(text));
    tx_char->notify();
}

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *server, ble_gap_conn_desc *desc) override
    {
        ble_connected = true;
    }

    void onDisconnect(NimBLEServer *server) override
    {
        ble_connected = false;
        // Quang ba lai ngay de PC co the ket noi lai ma khong phai khoi dong lai board.
        NimBLEDevice::startAdvertising();
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *characteristic) override
    {
        std::string value = characteristic->getValue();
        if (value.empty() || rx_stream == nullptr)
            return;
        // Khong cho (timeout 0): neu hang doi day thi bo phan thua con hon lam nghen task BLE.
        // Mat vai byte chi lam hong 1 dong du lieu, dong ke tiep (~0.8s sau) se dung lai ngay.
        xStreamBufferSend(rx_stream, value.data(), value.size(), 0);
    }
};

void transport_ble_begin()
{
    if (ble_started)
    {
        // Da khoi tao roi (vd nguoi dung roi sang WiFi rui quay lai BLE): chi can bat lai
        // quang ba, khong init lai stack - NimBLEDevice::init() 2 lan se hong.
        NimBLEDevice::startAdvertising();
        return;
    }

    rx_stream = xStreamBufferCreate(RX_STREAM_SIZE, 1); // trigger level 1 = bao co byte la doc duoc ngay
    if (rx_stream == nullptr)
    {
        Serial.println("PLG_>>>> BLE: khong cap phat duoc hang doi RX, bo qua khoi tao");
        return;
    }

    NimBLEDevice::init(BLE_DEVICE_NAME);

    NimBLEServer *server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService *service = server->createService(NUS_SERVICE_UUID);

    NimBLECharacteristic *rx_char = service->createCharacteristic(
        NUS_RX_CHAR_UUID,
        // WRITE_NR (write without response) cho phep PC ban du lieu lien tuc ma khong phai cho
        // xac nhan tung goi - nhanh hon han cho luong du lieu dinh ky nhu Task Manager.
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    rx_char->setCallbacks(new RxCallbacks());

    tx_char = service->createCharacteristic(NUS_TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

    service->start();

    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(NUS_SERVICE_UUID);
    advertising->setScanResponse(true); // de ten thiet bi hien day du khi PC do tim
    advertising->start();

    ble_started = true;
    Serial.printf("PLG_>>>> BLE: dang quang ba ten \"%s\", cho PC ghep noi\n", BLE_DEVICE_NAME);
}

void transport_ble_poll()
{
    if (rx_stream == nullptr)
        return;

    // Doc theo tung khoi cho den khi het hang doi. Timeout 0 = khong bao gio chan loop().
    uint8_t buf[128];
    size_t n;
    while ((n = xStreamBufferReceive(rx_stream, buf, sizeof(buf), 0)) > 0)
    {
        protocol_feed(buf, n, ble_reply);
    }
}

bool transport_ble_is_connected()
{
    return ble_connected;
}
