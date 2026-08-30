# PLG PC Monitor Display (plg-pc-monitor-display)

Firmware cho Raspberry Pi Pico điều khiển màn hình TFT ST7789 (240x320) làm giao diện hiển thị/điều khiển cho máy cầu lông tự động PLG.

## Tính năng

- Màn hình khởi động (splash) hiển thị logo + thanh Loading khi bật nguồn.
- Giao diện HOME: hiển thị sân cầu lông theo player 1/2, pin remote, pin máy, trạng thái kết nối, trạng thái RUN/STOP.
- Giao diện SETTING: chọn chức năng (PLAYER / FUNCTION / MODE / ID) bằng encoder xoay.
- Điều khiển bằng encoder (xoay tăng/giảm giá trị) + nút nhấn (chuyển màn hình / RUN-STOP).
- Nút riêng để vào chế độ nạp firmware qua USB (BOOTSEL) mà không cần tháo board.
- Xuất log trạng thái qua cổng USB Serial để theo dõi từ máy tính.

## Yêu cầu

- Raspberry Pi Pico
- Pico SDK 2.2.0, toolchain ARM GCC 14_2_Rel1 (khai báo sẵn trong `CMakeLists.txt`)
- Màn hình TFT ST7789 240x320, đấu SPI0 theo bảng chân bên dưới
- Encoder xoay có nút nhấn (KY-040 hoặc tương đương)

## Sơ đồ chân

| Chức năng          | GPIO |
|--------------------|------|
| TFT MOSI (SDIN)    | 7    |
| TFT SCLK           | 6    |
| TFT DC             | 4    |
| TFT CS             | 3    |
| TFT RST            | 5    |
| Encoder CLK        | 23   |
| Encoder DT         | 20   |
| Encoder SW (nút)   | 21   |
| Nút vào BOOTSEL    | 26   |
| Đèn báo board      | 17   |

SPI0, tốc độ cấu hình 100 kHz trong `setup_pin()` (`PLG_TFT_LCD.cpp`).

## Build & nạp firmware

```bash
mkdir build && cd build
cmake ..
ninja
```

Sau khi build xong sẽ có file `PLG_TFT_LCD.uf2` trong thư mục `build/`. Giữ nút BOOTSEL trên Pico khi cắm USB (hoặc nhấn nút ở GPIO 26 nếu firmware cũ đã chạy) để vào chế độ mass-storage, rồi copy file `.uf2` vào ổ đĩa RPI-RP2 xuất hiện.

## Hành vi khi khởi động

Khi cấp nguồn, Pico chạy `setup()` → `MONITOR_BEGIN()`:

1. Vẽ logo (`PLG_logo.hpp`) ở giữa màn hình.
2. Hiển thị chữ "Loading" cùng % chạy từ 0 đến 100.
3. Sau khi đạt 100%, xoá màn hình về nền trắng và chuyển sang giao diện HOME.

Màn hình splash này luôn được giữ nguyên mỗi lần khởi động — không bị bỏ qua hay tắt.

## Xem thông số qua máy tính (Serial/USB)

Firmware bật sẵn USB CDC (`pico_enable_stdio_usb(PLG_TFT_LCD 1)` trong `CMakeLists.txt`), UART vật lý bị tắt. Sau khi Pico khởi động xong và cắm vào máy tính qua cáp USB, nó sẽ hiện ra như một cổng COM (Windows) hoặc `/dev/ttyACM0` (Linux/macOS).

Các bước xem log:

1. Cắm Pico vào máy tính bằng cáp USB (không cần giữ BOOTSEL nếu firmware đã nạp sẵn).
2. Mở phần mềm Serial Monitor bất kỳ, ví dụ:
   - Arduino IDE → Tools → Serial Monitor
   - PuTTY (Windows), chọn Serial, đúng số COM
   - `screen /dev/ttyACM0 115200` (Linux/macOS)
   - Serial Monitor tích hợp trong VS Code (Pico extension)
3. Baud rate không bắt buộc khớp chính xác vì là USB CDC, nhưng chọn 115200 để tương thích các tool mặc định.

Log hiện có trong firmware (in qua `printf`, sẽ xuất hiện trên Serial Monitor):

- `PLG_end setup` — báo hoàn tất khởi động.
- `PLG_>>>>>>>>>>>>>>>>>>>` — khi nhấn nút chính, kèm theo trạng thái `status_machine` (RUN/STOP) đổi.
- `value = %d` — giá trị `battery1` hoặc `funtion_mode` mỗi lần xoay encoder (tuỳ đang ở màn hình nào).
- `END_INTERRUPS -------->>>` / `END_INTERRUPS <<<--------` — hướng xoay encoder.
- `PLG_end loop  %d` — thời gian một vòng lặp `loop()` (micro giây), in định kỳ để theo dõi hiệu năng.

> Lưu ý: log qua Serial chỉ xem được sau khi màn hình splash chạy xong và bước vào `loop()`, vì `stdio_init_all()` được gọi trong `setup_pin()` trước khi vẽ splash nên thực tế log có thể xuất hiện ngay từ lúc splash — không bị chặn.

## Gửi thông số Task Manager từ máy tính xuống board

Thư mục `pc_monitor/` chứa script Python chạy trên máy tính, đọc CPU / RAM / GPU / WIFI rồi gửi xuống Pico qua cổng USB Serial.

Cài đặt:

```bash
cd pc_monitor
pip install -r requirements.txt
```

Chạy:

```bash
python monitor.py                  # tự dò cổng Pico
python monitor.py --port COM5      # chỉ định cổng thủ công
python monitor.py --interval 0.5   # đổi tần suất gửi (giây), mặc định 1s
python monitor.py --list           # liệt kê các cổng serial hiện có
```

Mỗi dòng gửi xuống board có dạng:

```
CPU:<int>;RAM:<int>;GPU:<int>;WIFI:<int>\n
```

Giá trị là phần trăm (0-100). `GPU` trả về `-1` nếu máy không có GPU NVIDIA (dùng `pynvml`); `WIFI` hiện chỉ hỗ trợ đọc cường độ tín hiệu trên Windows (qua `netsh wlan show interfaces`), trả `-1` trên hệ điều hành khác hoặc khi không lấy được.

> Lưu ý: firmware hiện tại (`PLG_TFT_LCD.cpp`) chưa đọc dữ liệu từ cổng USB Serial gửi vào — nó chỉ `printf` log ra ngoài. Muốn hiển thị các thông số này lên màn hình TFT, cần thêm code phía Pico để đọc và parse chuỗi `CPU:...;RAM:...;GPU:...;WIFI:...` (ví dụ dùng `stdio` đọc từng dòng trong `loop()`), việc đó chưa được triển khai trong repo này.

## Cấu trúc thư mục

```
PLG_TFT_LCD.cpp        - logic chính (setup/loop, vẽ giao diện, xử lý nút/encoder)
PLG_setup.h             - khai báo chân GPIO, biến trạng thái, cấu hình SPI/TFT
PLG_logo.hpp             - dữ liệu bitmap logo hiển thị lúc khởi động
PLG_lib/ST7789_TFT_PICO-main/ - thư viện driver màn hình ST7789
pc_monitor/              - script Python (PC) gửi CPU/RAM/GPU/WIFI qua Serial
CMakeLists.txt          - cấu hình build Pico SDK
build/                   - output biên dịch (.uf2, .elf, .hex...)
```
