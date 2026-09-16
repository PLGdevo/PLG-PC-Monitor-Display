# PLG TFT LCD Task Manager — ESP32-S3 Edition

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-e7352c.svg)]()
[![Framework](https://img.shields.io/badge/framework-Arduino--ESP32-00979d.svg)]()
[![Status](https://img.shields.io/badge/status-Sprint%201%20in%20progress-yellow.svg)]()

> Tài liệu này mô tả bản thiết kế cho việc **di dời (migrate)** firmware `PLG_TFT_LCD_TASKMANAGER` từ
> Raspberry Pi Pico (RP2040) sang **ESP32-S3**, đồng thời bổ sung khả năng kết nối không dây
> (WiFi / Bluetooth) bên cạnh USB Serial hiện có. Xem [README.md](README.md) để biết trạng thái
> bản Pico hiện hành.

> **Trạng thái code**: Sprint 0 và Sprint 1 đã viết xong, **build sạch** (`esp32/`, 0 lỗi 0
> warning, RAM 6.6% / Flash 11.0%) nhưng **chưa nghiệm thu trên phần cứng thật** — theo
> Definition of Done bên dưới thì chưa được tính là Done. Hướng dẫn build + bảng nghiệm thu:
> [esp32/README.md](esp32/README.md).

## Mục lục

- [Mô tả dự án](#mô-tả-dự-án)
- [Vì sao chuyển sang ESP32-S3](#vì-sao-chuyển-sang-esp32-s3)
- [Yêu cầu phần cứng](#yêu-cầu-phần-cứng)
- [Sơ đồ chân (pinout)](#sơ-đồ-chân-pinout)
- [Kiến trúc phần mềm](#kiến-trúc-phần-mềm)
- [3 chế độ kết nối](#3-chế-độ-kết-nối)
- [Cấu trúc thư mục dự kiến](#cấu-trúc-thư-mục-dự-kiến)
- [Kế hoạch triển khai theo Sprint](#kế-hoạch-triển-khai-theo-sprint)
- [Định nghĩa Hoàn thành (Definition of Done)](#định-nghĩa-hoàn-thành-definition-of-done)
- [Rủi ro & giả định](#rủi-ro--giả-định)
- [License](#license)

## Mô tả dự án

`PLG_TFT_LCD_TASKMANAGER` là firmware điều khiển màn hình TFT ST7789 (240×320) đóng vai trò giao
diện hiển thị/điều khiển cho máy cầu lông tự động PLG, đồng thời hoạt động như màn hình phụ hiển
thị thông số hệ thống máy tính (CPU/RAM/GPU/nhiệt độ/pin/giờ) — tương tự một "Task Manager vật lý"
gắn ngoài case PC.

Bản gốc chạy trên Raspberry Pi Pico, chỉ nhận dữ liệu qua **USB Serial**. Bản ESP32-S3 giữ nguyên
toàn bộ trải nghiệm giao diện (HOME, SETTING, CLOCK, Task Manager charts, đổi màu/font/ngôn ngữ)
nhưng bổ sung:

- Chọn **cách kết nối với máy tính** ngay trên thiết bị: USB (COM), Bluetooth (BLE), hoặc WiFi.
- Với WiFi: thiết bị tự quét mạng, cho người dùng chọn SSID và nhập mật khẩu bằng encoder xoay,
  sau đó có thể **lưu lại thành 1 bộ IP tĩnh cố định** để những lần sau kết nối ngay không cần
  cấu hình lại.
- Với Bluetooth: thiết bị tự phát (advertise) để máy tính ghép nối và truyền dữ liệu.

## Vì sao chuyển sang ESP32-S3

| Tiêu chí | RP2040 (Pico) | ESP32-S3 |
|---|---|---|
| WiFi/BLE tích hợp | Không (cần module rời) | Có sẵn (802.11 b/g/n + BLE 5.0) |
| RAM | 264 KB | 512 KB SRAM (+ PSRAM tùy board) — dư dả cho stack TCP/BLE + buffer chart |
| Flash lưu cấu hình | Raw flash sector (`hardware/flash.h`) | NVS (`Preferences.h`) — bền hơn, có wear-levelling |
| Xử lý song song | 1 lõi (single-core) | Dual-core (1 lõi lo UI/encoder, 1 lõi lo network) — tránh giật lag khi WiFi hoạt động |
| Hệ sinh thái driver TFT/BLE/WiFi | Hạn chế, tự viết nhiều | Phong phú (Arduino-ESP32 core, `Adafruit_ST7789`, `NimBLE-Arduino`, `WiFi.h`) |

**Quyết định**: dùng **ESP32-S3** (không dùng ESP32 cổ điển) vì có USB OTG native (giữ được trải
nghiệm cắm USB debug/flash giống Pico hiện tại), nhiều GPIO hơn, và là dòng khuyến nghị hiện hành
của Espressif cho thiết kế mới.

## Yêu cầu phần cứng

- Board **ESP32-S3-DevKitC-1** (hoặc tương đương, chip `ESP32-S3-WROOM-1`, tối thiểu 8MB flash)
- Màn hình TFT ST7789 240×320, giao tiếp SPI (giữ nguyên phần cứng màn hình cũ)
- Encoder xoay có nút nhấn (KY-040 hoặc tương đương, giữ nguyên)
- Cáp USB-C (USB OTG native của ESP32-S3, dùng để nạp firmware + debug Serial, không cần mạch nạp rời)

## Sơ đồ chân (pinout)

Vì đổi board nên toàn bộ GPIO được chọn lại, tránh các chân "strapping" của ESP32-S3
(`GPIO0`, `GPIO3`, `GPIO45`, `GPIO46`) và chân USB native (`GPIO19`/`GPIO20`) để không xung đột lúc boot/flash:

| Chức năng            | GPIO   | Ghi chú |
|----------------------|--------|---------|
| TFT MOSI (SDIN)      | GPIO11 | SPI2 (FSPI) mặc định của board DevKitC-1 |
| TFT SCLK             | GPIO12 | |
| TFT DC               | GPIO9  | |
| TFT CS               | GPIO10 | |
| TFT RST              | GPIO8  | |
| TFT BLK (backlight)  | GPIO7  | PWM để chỉnh độ sáng (tính năng mở rộng, có thể để luôn HIGH ban đầu) |
| Encoder CLK          | GPIO4  | |
| Encoder DT           | GPIO5  | |
| Encoder SW (nút nhấn)| GPIO6  | |
| Đèn báo board (status LED) | GPIO2 | |
| Nút BOOT (vào chế độ nạp firmware) | GPIO0 (có sẵn trên board) | Không cần mạch ngoài — giữ nút BOOT + RESET có sẵn trên DevKitC-1, khác với Pico phải gắn nút BOOTSEL rời (GPIO26 cũ bỏ) |

> SPI tốc độ mục tiêu 40–80MHz tùy chất lượng dây nối tới màn hình (ESP32-S3 SPI2 hỗ trợ tối đa 80MHz).

## Kiến trúc phần mềm

Nguyên tắc thiết kế: **không thay đổi giao thức dữ liệu** (`CPU:..;RAM:..;...\n`) — chỉ thay đổi
đường truyền (transport). Toàn bộ logic vẽ UI (`PLG_screens`, `PLG_charts`, `PLG_theme`) không cần
biết dữ liệu đến từ USB, BLE hay WiFi.

```
                    ┌──────────────────────────┐
                    │   PLG_transport (mới)     │  dispatcher chọn transport theo
                    │   transport_begin(mode)   │  active_connection_mode (lưu NVS)
                    │   transport_poll()        │
                    └─────────────┬────────────┘
              ┌────────────────────┼─────────────────────┐
              ▼                    ▼                      ▼
   PLG_transport_serial   PLG_transport_ble      PLG_transport_wifi
   (giữ logic parse cũ)   (NimBLE GATT server)   (TCP server + IP tĩnh)
              │                    │                      │
              └────────────────────┴──────────┬───────────┘
                                               ▼
                                  cùng ghi vào state chung:
                          chart_cpu[], battery1, CONNECT_STATUS, current_time_str...
                                               │
                                               ▼
                                  PLG_screens / PLG_charts (không đổi)
```

Thành phần chính:

- `PLG_transport.*` — dispatcher + interface chung (`transport_begin`, `transport_poll`, `transport_is_connected`).
- `PLG_transport_serial.*` — cổng USB Serial, hành vi tương đương `PLG_serial_link.cpp` hiện tại.
- `PLG_transport_ble.*` — BLE GATT server (`NimBLE-Arduino`), 1 characteristic WRITE nhận dữ liệu text.
- `PLG_transport_wifi.*` — TCP server nhận dữ liệu qua IP, cộng thêm phần scan/connect WiFi.
- `PLG_wifi_ui.*` — state machine UI riêng cho wizard chọn SSID + nhập mật khẩu (wheel-picker).
- `PLG_flash_settings.*` — chuyển từ raw flash sector sang `Preferences` (NVS), thêm namespace lưu
  cấu hình WiFi tĩnh (SSID/pass/IP/gateway/subnet/dns) — chỉ 1 bộ duy nhất, ghi đè khi lưu lại.

## 3 chế độ kết nối

### USB (COM)
Giữ nguyên hành vi hiện tại: cắm USB, `pc_monitor.py` mở cổng COM tương ứng và gửi dữ liệu.

### Bluetooth (BLE)
1. Chọn BLUETOOTH trong menu SETTING → CONNECTION.
2. Thiết bị bật advertising ngay (`BLEDevice::init("PLG_TFT_LCD")`), hiện tên thiết bị trên màn hình.
3. Máy tính ghép nối (dùng `bleak` phía Python) và ghi dữ liệu vào characteristic.
4. Màn hình hiện trạng thái "Đang chờ kết nối..." → "Đã kết nối".

### WiFi (kèm lưu IP tĩnh)
1. Chọn WIFI trong menu SETTING → CONNECTION.
2. Nếu **chưa có cấu hình lưu**: chạy wizard —
   - **Scan**: liệt kê SSID quét được, xoay encoder để cuộn, nhấn để chọn.
   - **Nhập mật khẩu**: wheel-picker từng ký tự bằng encoder (xoay = đổi ký tự, nhấn ngắn = chốt
     và sang ký tự kế tiếp, nhấn giữ = xoá lùi, chọn `[DONE]`/`[HỦY]` ở cuối bảng ký tự để kết thúc).
   - **Connecting**: `WiFi.begin()` theo DHCP, timeout ~15s.
   - **Done**: hiện IP đang dùng lên màn hình (`192.168.x.x`), gợi ý "Giữ nút để lưu IP tĩnh".
3. Nhấn giữ ở màn hình Done → lưu SSID/mật khẩu/IP/gateway/subnet/DNS hiện tại thành **1 bộ IP tĩnh**
   duy nhất vào NVS.
4. Từ lần boot sau: nếu có cấu hình đã lưu, gọi thẳng `WiFi.config(ip, gw, subnet, dns)` rồi
   `WiFi.begin(ssid, pass)` — bỏ qua wizard, kết nối thẳng bằng địa chỉ cố định.
5. Máy tính (`pc_monitor.py`) kết nối TCP tới đúng IP tĩnh đó để truyền dữ liệu.
6. Có thể "quên mạng đã lưu" (giữ nút ở màn hình Status khi đã có cấu hình) để chạy lại wizard.

## Cấu trúc thư mục dự kiến

> **Thực tế đã triển khai khác 2 điểm** (quyết định trong Sprint 0):
> 1. Toàn bộ bản ESP32 nằm trong thư mục con `esp32/` thay vì thay thế tại chỗ, để bản Pico
>    đang chạy được vẫn build/nạp bình thường trong suốt quá trình migrate.
> 2. **Không đổi sang `Adafruit_ST7789`/`TFT_eSPI`** như dự kiến bên dưới. Khi khảo sát thì
>    `ST7789_TFT_PICO` chỉ phụ thuộc pico-SDK ở lớp phần cứng rất mỏng (vài macro GPIO, 2 lời
>    gọi SPI, 2 hàm delay) — nên port chính thư viện đó sang Arduino HAL (`esp32/lib/PLG_ST7789/`)
>    rẻ hơn nhiều so với vẽ lại UI, và giữ giao diện giống hệt từng pixel.

```
PLG_TFT_LCD_TASKMANAGER/
├── platformio.ini              # thay CMakeLists.txt, khai báo board esp32-s3-devkitc-1
├── src/
│   ├── PLG_TFT_LCD.cpp         # setup()/loop() (giữ tên, sửa nội dung sang Arduino API)
│   ├── PLG_pins.cpp            # pinout mới (mục Sơ đồ chân)
│   ├── PLG_display.cpp         # port sang Adafruit_ST7789 hoặc TFT_eSPI
│   ├── PLG_input.cpp           # encoder qua attachInterrupt thay vì polling
│   ├── PLG_charts.cpp          # không đổi logic, chỉ đổi API vẽ nếu cần
│   ├── PLG_screens.cpp         # thêm MONITOR_CONNECTION/MONITOR_WIFI_*/MONITOR_BLE_STATUS
│   ├── PLG_state.cpp           # thêm state kết nối + wizard WiFi
│   ├── PLG_lang.cpp            # thêm nhãn cho màn hình kết nối
│   ├── PLG_flash_settings.cpp  # chuyển sang Preferences (NVS)
│   ├── PLG_transport.cpp       # (mới) dispatcher
│   ├── PLG_transport_serial.cpp# (mới)
│   ├── PLG_transport_ble.cpp   # (mới)
│   ├── PLG_transport_wifi.cpp  # (mới)
│   └── PLG_wifi_ui.cpp         # (mới) scan list + wheel-picker password
├── include/                    # header tương ứng
├── pc_monitor/
│   └── monitor.py              # thêm lựa chọn transport: --serial / --ble / --wifi <ip>
└── README_ESP32_MIGRATION.md   # tài liệu này
```

## Kế hoạch triển khai theo Sprint

Giả định 1 sprint = 1 tuần, 1 người phát triển firmware + 1 người hỗ trợ script Python bán thời
gian. Story point theo thang Fibonacci (1,2,3,5,8).

### Sprint 0 — Nền tảng migrate (Foundation)
**Mục tiêu**: build & chạy được UI hiện có (không network) trên ESP32-S3.

| # | User Story | Tiêu chí chấp nhận (AC) | Điểm |
|---|---|---|---|
| 0.1 | *Là dev, tôi muốn dự án build bằng PlatformIO cho board ESP32-S3 để thay thế toolchain CMake/Pico SDK.* | `platformio.ini` build thành công, nạp được firmware trống qua USB-C. | 3 |
| 0.2 | *Là dev, tôi muốn màn hình ST7789 hiển thị được hình ảnh cơ bản trên ESP32-S3* theo pinout mới. | Vẽ được logo splash + text tĩnh, đúng màu, đúng chiều xoay 270°. | 5 |
| 0.3 | *Là dev, tôi muốn encoder xoay/nhấn hoạt động qua ngắt (interrupt)* thay vì polling như bản Pico. | Xoay tăng/giảm giá trị test, nhấn ngắn/nhấn giữ phân biệt đúng như hành vi cũ. | 3 |
| 0.4 | *Là dev, tôi muốn cấu hình lưu (màu/font/ngôn ngữ) chuyển sang NVS (`Preferences`)* để không phụ thuộc raw flash sector cũ. | Lưu và đọc lại đúng giá trị sau khi reset nguồn. | 3 |

**Tổng điểm Sprint 0: 14**

### Sprint 1 — Port toàn bộ UI hiện có (chỉ USB Serial)
**Mục tiêu**: đạt tính năng ngang bằng bản Pico, dùng USB Serial y như cũ.

| # | User Story | AC | Điểm |
|---|---|---|---|
| 1.1 | *Là người dùng, tôi muốn màn hình HOME/Task Manager/CLOCK hoạt động như bản Pico* để không mất tính năng khi đổi board. | 5 biểu đồ CPU/RAM/GPU/GPUMEM/WIFI vẽ đúng, đồng hồ HH:MM đúng. | 8 |
| 1.2 | *Là người dùng, tôi muốn menu SETTING (PLAYER/FUNTION/MODE/CLOCK/COLOR/FONT/TASK/LANGUAGE) hoạt động đầy đủ.* | Duyệt menu, đổi màu/font/ngôn ngữ, lưu flash đều hoạt động đúng như bản gốc. | 5 |
| 1.3 | *Là người dùng, tôi muốn nhận dữ liệu Task Manager qua USB Serial giống hệt trước đây* để `pc_monitor.py` không cần sửa gì trong sprint này. | `pc_monitor.py` bản hiện tại chạy không sửa code, dữ liệu hiển thị đúng. | 3 |
| 1.4 | *Là QA, tôi muốn splash/loading hoạt động đúng hành vi chờ dữ liệu thật hoặc bỏ qua bằng nhấn đúp.* | Giống mô tả README gốc mục "Hành vi khi khởi động". | 2 |

**Tổng điểm Sprint 1: 18**

### Sprint 2 — Kiến trúc Transport Layer + màn hình chọn kết nối
**Mục tiêu**: tách được lớp transport, thêm UI chọn mode (chưa cần BLE/WiFi thật, có thể stub).

| # | User Story | AC | Điểm |
|---|---|---|---|
| 2.1 | *Là dev, tôi muốn 1 interface `transport_begin/poll/is_connected` chung* để UI không phụ thuộc nguồn dữ liệu cụ thể. | `PLG_transport.*` tồn tại, USB hoạt động qua interface mới, không regressions so với Sprint 1. | 5 |
| 2.2 | *Là người dùng, tôi muốn vào SETTING → CONNECTION để chọn USB/Bluetooth/WiFi.* | Màn hình `MONITOR_CONNECTION` hiện 3 lựa chọn, chọn xong lưu vào NVS, áp dụng ngay sau khi chọn. | 5 |
| 2.3 | *Là người dùng, tôi muốn lựa chọn kết nối được nhớ sau khi mất điện.* | Reset nguồn, thiết bị tự vào lại đúng mode đã chọn lần trước. | 2 |

**Tổng điểm Sprint 2: 12**

### Sprint 3 — Bluetooth (BLE) transport
**Mục tiêu**: truyền dữ liệu qua BLE hoạt động đầu-cuối.

| # | User Story | AC | Điểm |
|---|---|---|---|
| 3.1 | *Là người dùng, khi chọn Bluetooth, thiết bị tự phát tín hiệu để máy tính tìm thấy.* | `MONITOR_BLE_STATUS` hiện tên thiết bị + trạng thái "đang chờ"/"đã kết nối". | 5 |
| 3.2 | *Là người dùng, tôi muốn dữ liệu Task Manager gửi qua BLE hiển thị đúng như qua USB.* | Charts/pin/giờ cập nhật đúng khi PC gửi qua BLE. | 5 |
| 3.3 | *Là dev Python, tôi muốn `pc_monitor.py` hỗ trợ gửi qua BLE (`bleak`)* bằng 1 cờ dòng lệnh. | `python monitor.py --ble` kết nối và gửi dữ liệu thành công. | 5 |
| 3.4 | *Là người dùng, tôi muốn biết khi mất kết nối BLE* để không tưởng nhầm dữ liệu bị treo là dữ liệu mới. | Icon trạng thái kết nối (`CONNECT_STATUS`) chuyển đúng khi ngắt kết nối. | 2 |

**Tổng điểm Sprint 3: 17**

### Sprint 4 — WiFi transport: Scan + Connect (DHCP, chưa lưu tĩnh)
**Mục tiêu**: kết nối WiFi và truyền dữ liệu qua TCP, IP có thể đổi mỗi lần (DHCP).

| # | User Story | AC | Điểm |
|---|---|---|---|
| 4.1 | *Là người dùng, tôi muốn xem danh sách WiFi xung quanh và chọn 1 mạng bằng encoder.* | `MONITOR_WIFI_SCAN` liệt kê đúng SSID quét được, cuộn/chọn mượt. | 5 |
| 4.2 | *Là người dùng, tôi muốn nhập mật khẩu WiFi bằng cách xoay encoder chọn từng ký tự.* | Wheel-picker hoạt động: xoay đổi ký tự, nhấn ngắn chốt, nhấn giữ xoá lùi, chọn được DONE/HỦY. | 8 |
| 4.3 | *Là người dùng, tôi muốn thấy trạng thái đang kết nối và biết nếu sai mật khẩu/timeout.* | Có màn hình "Đang kết nối..." và thông báo lỗi quay lại wizard nếu thất bại sau ~15s. | 3 |
| 4.4 | *Là người dùng, sau khi kết nối xong tôi muốn thấy IP hiện tại của thiết bị trên màn hình.* | `MONITOR_WIFI_STATUS` hiện đúng `WiFi.localIP()`. | 2 |
| 4.5 | *Là người dùng, tôi muốn máy tính truyền dữ liệu qua IP đó bằng TCP.* | `PLG_transport_wifi` mở TCP server, `pc_monitor.py --wifi <ip>` gửi dữ liệu, charts cập nhật đúng. | 5 |

**Tổng điểm Sprint 4: 23** *(sprint nặng nhất — có thể tách 4.2 thành sprint riêng nếu cần)*

### Sprint 5 — Lưu IP tĩnh + hoàn thiện vòng đời cấu hình WiFi
**Mục tiêu**: không phải cấu hình lại WiFi mỗi lần khởi động.

| # | User Story | AC | Điểm |
|---|---|---|---|
| 5.1 | *Là người dùng, tôi muốn giữ nút ở màn hình Done để lưu cấu hình WiFi hiện tại thành IP tĩnh cố định.* | Nhấn giữ → ghi NVS (SSID/pass/IP/gateway/subnet/DNS), có phản hồi trên màn hình ("Đã lưu"). | 5 |
| 5.2 | *Là người dùng, lần khởi động sau tôi muốn thiết bị tự kết nối thẳng bằng IP tĩnh đã lưu, không cần chạy lại wizard.* | Boot lên, bỏ qua Scan/Password, `WiFi.config()` + `WiFi.begin()` áp dụng đúng bộ đã lưu, hiện Status trực tiếp. | 5 |
| 5.3 | *Là người dùng, nếu mạng đã lưu không còn khả dụng, tôi muốn thiết bị tự quay lại wizard thay vì treo màn hình.* | Timeout kết nối bằng cấu hình cũ → tự chuyển sang `MONITOR_WIFI_SCAN`. | 3 |
| 5.4 | *Là người dùng, tôi muốn "quên mạng đã lưu" để đổi sang WiFi khác.* | Giữ nút ở màn hình Status (khi đã có cấu hình lưu) → xoá NVS, chạy lại wizard từ đầu. | 3 |

**Tổng điểm Sprint 5: 16**

### Sprint 6 — Hoàn thiện `pc_monitor.py` đa giao thức + tài liệu
**Mục tiêu**: script Python chọn được transport tương ứng, tài liệu đầy đủ cho người dùng cuối.

| # | User Story | AC | Điểm |
|---|---|---|---|
| 6.1 | *Là người dùng máy tính, tôi muốn 1 cờ dòng lệnh duy nhất để chọn Serial/BLE/WiFi khi chạy `monitor.py`.* | `--serial COM3` / `--ble` / `--wifi 192.168.x.x` đều hoạt động, mặc định tự dò COM như hiện tại nếu không truyền cờ. | 5 |
| 6.2 | *Là người dùng cuối, tôi muốn README hướng dẫn đầy đủ cách cấu hình từng loại kết nối.* | README cập nhật mục hướng dẫn WiFi/BLE/USB, có ảnh/sơ đồ nếu cần. | 3 |
| 6.3 | *Là QA, tôi muốn test end-to-end cả 3 chế độ kết nối trên phần cứng thật trước khi release.* | Checklist test 3 mode pass, không phát hiện regression so với bản Pico. | 5 |

**Tổng điểm Sprint 6: 13**

**Tổng cộng backlog: 113 điểm** (~6-7 sprint với 1 dev full-time, có thể co giãn tùy tốc độ thực tế).

## Định nghĩa Hoàn thành (Definition of Done)

Một story được coi là Done khi:
- Code build không lỗi/warning trên PlatformIO cho `esp32-s3-devkitc-1`.
- Test thủ công trên phần cứng thật (không chỉ mô phỏng) đạt đúng AC.
- Không có regression ở các tính năng đã Done từ sprint trước (đặc biệt: đổi mode kết nối không
  làm hỏng menu SETTING, đổi màu/font/ngôn ngữ).
- Cấu hình lưu (flash/NVS) sống sót qua ít nhất 1 lần rút nguồn thật (không chỉ reset mềm).
- Code đã cập nhật comment/docstring nếu có hành vi không hiển nhiên (theo quy ước hiện có của dự án).

## Rủi ro & giả định

- **Giả định**: màn hình ST7789 vật lý hiện tại tương thích chân SPI chuẩn, không cần mạch chuyển
  mức điện áp (ESP32-S3 GPIO là 3.3V, giống Pico nên khả năng cao không cần thêm phần cứng).
- **Rủi ro UX**: wheel-picker nhập mật khẩu WiFi bằng encoder (Sprint 4.2) chậm với mật khẩu dài —
  chấp nhận đánh đổi để không cần thêm phần cứng/màn hình cảm ứng.
- **Rủi ro kỹ thuật**: chạy đồng thời WiFi + vẽ chart tốc độ cao trên 1 lõi có thể giật hình — dự
  kiến xử lý bằng cách đưa việc quét dữ liệu network sang lõi thứ 2 (FreeRTOS task) nếu phát sinh
  vấn đề trong Sprint 4.
- **Không trong phạm vi (out of scope)**: chạy đồng thời BLE + WiFi cùng lúc; đồng bộ nhiều thiết bị
  ESP32 với 1 PC; OTA update qua mạng (có thể là backlog tương lai).

## License

Theo giấy phép của dự án gốc, xem [LICENSE](LICENSE).
