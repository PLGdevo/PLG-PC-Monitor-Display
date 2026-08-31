# PLG PC Monitor Display

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-8f4fbc.svg)]()
[![Pico SDK](https://img.shields.io/badge/Pico%20SDK-2.2.0-1a8cff.svg)]()

Firmware cho Raspberry Pi Pico điều khiển màn hình TFT ST7789 (240×320), đóng vai trò giao diện hiển thị/điều khiển cho máy cầu lông tự động PLG, đồng thời có thể hoạt động như một màn hình phụ hiển thị thông số hệ thống (CPU/RAM/GPU/WiFi/pin/giờ) đọc trực tiếp từ máy tính qua USB Serial.

## Mục lục

- [Tính năng chính](#tính-năng-chính)
- [Yêu cầu phần cứng](#yêu-cầu-phần-cứng)
- [Sơ đồ chân](#sơ-đồ-chân)
- [Build & nạp firmware](#build--nạp-firmware)
- [Hành vi khi khởi động](#hành-vi-khi-khởi-động)
- [Xem log qua Serial](#xem-log-qua-serial)
- [Gửi thông số PC xuống board](#gửi-thông-số-pc-xuống-board)
- [Cấu trúc thư mục](#cấu-trúc-thư-mục)
- [License](#license)

## Tính năng chính

- **Splash khởi động**: hiển thị logo dự án kèm thanh Loading khi bật nguồn, luôn chạy đầy đủ mỗi lần khởi động.
- **Giao diện HOME**: hiển thị trạng thái sân cầu lông theo player 1/2, pin remote, pin máy, trạng thái kết nối và trạng thái RUN/STOP.
- **Giao diện SETTING**: menu dạng danh sách gồm PLAYER / FUNTION / MODE / CLOCK / COLOR / TASK, chọn bằng encoder xoay; CLOCK mở màn hình đồng hồ, COLOR mở màn hình chọn màu nhận diện, TASK quay lại thẳng màn hình Task Manager (HOME).
- **Task Manager PC**: đọc dữ liệu CPU / RAM / GPU (usage 3D + VRAM riêng) / WiFi / pin / giờ / ngày gửi lên từ máy tính qua USB Serial, vẽ 5 biểu đồ (chart) theo thời gian thực và hiển thị đồng hồ HH:MM ở góc trên-phải màn hình.
- **Điều khiển bằng encoder**: xoay để tăng/giảm giá trị, nhấn để chuyển màn hình hoặc RUN/STOP.
- **Nút BOOTSEL rời**: vào chế độ nạp firmware qua USB mà không cần tháo board ra khỏi máy.
- **Log chẩn đoán qua USB Serial**: xuất trạng thái vòng lặp, encoder, nút nhấn... để debug từ máy tính.

## Yêu cầu phần cứng

- Raspberry Pi Pico
- Pico SDK 2.2.0, toolchain ARM GCC 14_2_Rel1 (đã khai báo sẵn trong `CMakeLists.txt`)
- Màn hình TFT ST7789 240×320, đấu SPI0 theo bảng chân bên dưới
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

SPI0, tốc độ cấu hình 125 MHz (bị phần cứng RP2040 tự động giới hạn về mức tối đa khả dụng, ~62.5MHz) trong `setup_pin()` (`src/PLG_display.cpp`).

## Build & nạp firmware

```bash
mkdir build && cd build
cmake ..
ninja
```

Sau khi build xong sẽ có file `PLG_TFT_LCD.uf2` trong thư mục `build/`. Giữ nút BOOTSEL trên Pico khi cắm USB (hoặc nhấn nút ở GPIO 26 nếu firmware cũ đã chạy) để vào chế độ mass-storage, rồi copy file `.uf2` vào ổ đĩa RPI-RP2 xuất hiện.

## Hành vi khi khởi động

Khi cấp nguồn, Pico chạy `setup()` → `MONITOR_BEGIN()` (`src/PLG_screens.cpp`):

1. Vẽ logo (`PLG_logo.hpp`) ở giữa màn hình, nền đen.
2. Hiển thị chữ "Loading" cùng % chạy nhanh từ 0 đến 90%.
3. Chạy chậm dần từ 90% lên 98% và giữ ở 98%, chờ đến khi nhận được dữ liệu Task Manager thật từ `pc_monitor/monitor.py` qua Serial, hoặc người dùng nhấn nút encoder 2 lần để bỏ qua thủ công.
4. Chạy nốt lên 100%, xoá màn hình về nền giao diện (`UI_BG`, tối) và chuyển sang giao diện HOME (Task Manager).

Màn hình splash này luôn được giữ nguyên mỗi lần khởi động — không bị bỏ qua hay tắt (chỉ đoạn chờ dữ liệu thật ở bước 3 mới có thể bỏ qua bằng nút nhấn).

## Xem log qua Serial

Firmware bật sẵn USB CDC (`pico_enable_stdio_usb(PLG_TFT_LCD 1)` trong `CMakeLists.txt`), UART vật lý bị tắt. Sau khi Pico khởi động xong và cắm vào máy tính qua cáp USB, nó sẽ hiện ra như một cổng COM (Windows) hoặc `/dev/ttyACM0` (Linux/macOS).

Các bước xem log:

1. Cắm Pico vào máy tính bằng cáp USB (không cần giữ BOOTSEL nếu firmware đã nạp sẵn).
2. Mở phần mềm Serial Monitor bất kỳ, ví dụ:
   - Arduino IDE → Tools → Serial Monitor
   - PuTTY (Windows), chọn Serial, đúng số COM
   - `screen /dev/ttyACM0 115200` (Linux/macOS)
   - Serial Monitor tích hợp trong VS Code (Pico extension)
3. Baud rate không bắt buộc khớp chính xác vì là USB CDC, nhưng chọn 115200 để tương thích các tool mặc định.

Log hiện có trong firmware (in qua `printf`, xuất hiện trên Serial Monitor):

- `PLG_end setup` — báo hoàn tất khởi động (`PLG_TFT_LCD.cpp`).
- `PLG_end loop  %d` — thời gian tính từ lần in log trước (micro giây), in định kỳ mỗi khi vòng lặp `loop()` đã chạy được hơn 1ms, để theo dõi hiệu năng (`PLG_TFT_LCD.cpp`).
- `I AM PLG_TFT_LCD_TASKMANAGER` — phản hồi bắt tay khi nhận được `PLG_ID?` từ `monitor.py` (`src/PLG_serial_link.cpp`).
- `PLG_>>>> back to TASK MANAGER (home)` / `PLG_>>>> enter CLOCK` / `PLG_>>>> exit CLOCK, back to SETTING menu` / `PLG_>>>> enter COLOR` / `PLG_>>>> apply COLOR #%d, back to SETTING menu` — nhấn ngắn nút encoder trong menu SETTING, tuỳ mục đang chọn (`src/PLG_input.cpp`).

## Gửi thông số PC xuống board

Thư mục `pc_monitor/` chứa script Python chạy trên máy tính, đọc CPU / RAM / GPU (usage 3D + VRAM) / WiFi / pin / giờ / ngày rồi gửi xuống Pico qua cổng USB Serial. Phía firmware (`read_taskmanager_serial()` trong `src/PLG_serial_link.cpp`) đọc, parse chuỗi này không chặn (non-blocking), đẩy dữ liệu vào biểu đồ (`chart_push`) và cập nhật đồng hồ trên màn hình.

Cài đặt:

```bash
cd pc_monitor
pip install -r requirements.txt
```

Chạy:

```bash
python monitor.py                  # tự dò + tự xác thực board PLG, không cần chọn cổng
python monitor.py --port COM5      # ép dùng cổng này, bỏ qua bước dò/xác thực tự động
python monitor.py --interval 0.5   # đổi tần suất gửi (giây), mặc định 0.8s
python monitor.py --list           # liệt kê các cổng serial hiện có
```

**Tự động xác thực thiết bị**: thay vì phải tự chọn đúng cổng COM, script gửi lệnh `PLG_ID?` xuống lần lượt các cổng serial đang cắm (ưu tiên cổng có VID/PID giống Pico trước, sau đó thử các cổng còn lại) và chỉ coi là board hợp lệ khi nhận lại đúng câu trả lời `I AM PLG_TFT_LCD_TASKMANAGER` từ firmware (`read_taskmanager_serial()` xử lý lệnh này trong `src/PLG_serial_link.cpp`). Nhờ vậy tránh được trường hợp gửi nhầm dữ liệu xuống một thiết bị USB Serial khác có cùng VID/PID (ví dụ Pico khác chạy firmware khác).

Chạy nền (không có terminal tương tác, ví dụ khi đặt vào Startup): script tự dò + tự xác thực, chờ board PLG được cắm vào, đồng thời tự kết nối lại nếu bị rút dây hoặc mất Serial giữa chừng — thời gian kết nối lại sau khi rút/cắm dây thường trong khoảng 10-15 giây (phần lớn là thời gian USB tự nhận diện lại thiết bị, không phải độ trễ của script).

**Dùng bản `.exe` dựng sẵn** (không cần cài Python): tải file `PLG_monitor.zip` trong [Releases](https://github.com/PLGdevo/PLG-PC-Monitor-Display/releases/latest), giải nén rồi chạy `PLG_monitor.exe` — cách dùng và các cờ (`--port`, `--interval`, `--list`) giống hệt `python monitor.py`.

> ⚠️ File tải từ Internet chưa có chữ ký số (code signing certificate), nên Windows sẽ chặn bằng SmartScreen với thông báo "Windows protected your PC". Cách bỏ chặn:
> - Chuột phải file `.exe` → **Properties** → tick **Unblock** ở cuối tab **General** → **OK**, rồi chạy lại; hoặc
> - Khi thấy thông báo SmartScreen → bấm **More info** → **Run anyway**.
>
> Đây không phải virus, chỉ là do file thực thi chưa được ký số — nếu ngại thao tác này, có thể chạy trực tiếp `python monitor.py` thay vì dùng bản `.exe`.

Mỗi dòng gửi xuống board có dạng:

```
CPU:<int>;RAM:<int>;GPU:<int>;GPUMEM:<int>;WIFI:<int>;TIME:<HH:MM:SS>;DATE:<DD/MM/YYYY>;BAT:<int>
```

Giá trị CPU/RAM/GPU/GPUMEM/WIFI/BAT là phần trăm (0–100). `GPU` là % sử dụng engine 3D, `GPUMEM` là % VRAM đã dùng (dedicated + shared / tổng AdapterRAM) — cả hai ưu tiên đọc qua `pynvml` (GPU NVIDIA), nếu không có NVIDIA thì fallback sang WMI performance counter (`GPU Engine` / `GPU Adapter Memory`, có sẵn từ Windows 10 1803+, không cần driver riêng, chạy trong thread nền để không làm chậm vòng gửi chính); trả `-1` nếu không lấy được trên cả hai đường. `WIFI` hiện chỉ hỗ trợ đọc cường độ tín hiệu trên Windows (qua `netsh wlan show interfaces`), trả `-1` trên hệ điều hành khác hoặc khi không lấy được; `BAT` trả `-1` nếu máy không có pin (PC bàn) — board sẽ giữ nguyên giá trị pin cũ.

## Cấu trúc thư mục

Firmware được chia theo từng tác vụ (module), mỗi module có 1 cặp header (`include/`) + file cài đặt (`src/`):

```
PLG_TFT_LCD.cpp                - entry point: setup()/loop()/main(), lắp ráp các module lại với nhau
include/PLG_pins.h             - khai báo chân GPIO + chân SPI tới TFT
include/PLG_theme.h            - bảng màu giao diện (RGB565) + danh sách màu nhận diện chọn được
include/PLG_state.h            - toàn bộ biến trạng thái + enum dùng chung giữa các module
include/PLG_display.h          - instance TFT dùng chung (myTFT) + setup_pin() (khởi tạo GPIO/SPI/màn hình)
include/PLG_flash_settings.h   - lưu/đọc màu giao diện đã chọn vào flash (giữ lại sau khi mất nguồn)
include/PLG_serial_link.h      - đọc dữ liệu Task Manager (CPU/RAM/GPU/WIFI/giờ/pin) qua USB Serial
include/PLG_charts.h           - vẽ biểu đồ dạng đường cho Task Manager
include/PLG_screens.h          - các màn hình: splash, status bar, Task Manager, menu SETTING, CLOCK, COLOR
include/PLG_input.h            - xử lý encoder xoay + nút nhấn (ngắn/giữ lâu), chuyển màn hình
src/*.cpp                      - phần cài đặt tương ứng với từng header ở trên
PLG_logo.hpp                   - dữ liệu bitmap logo hiển thị lúc khởi động
PLG_lib/ST7789_TFT_PICO-main/  - thư viện driver màn hình ST7789
pc_monitor/                    - script Python (PC) gửi CPU/RAM/GPU/WiFi/pin/giờ qua Serial
CMakeLists.txt                 - cấu hình build Pico SDK
build/                         - output biên dịch (.uf2, .elf, .hex...), không commit vào git
```

Sơ đồ phụ thuộc giữa các module (mũi tên = "include"):

```
PLG_TFT_LCD.cpp
   ├─ PLG_display   (myTFT, setup_pin)  ──> PLG_input, PLG_pins
   ├─ PLG_flash_settings                ──> PLG_theme, PLG_state
   ├─ PLG_input      (encoder/nút)      ──> PLG_pins, PLG_state, PLG_theme, PLG_display, PLG_flash_settings
   ├─ PLG_serial_link (đọc Task Manager)──> PLG_state
   ├─ PLG_charts     (vẽ biểu đồ)       ──> PLG_state, PLG_theme, PLG_display
   └─ PLG_screens    (các màn hình)     ──> PLG_state, PLG_theme, PLG_pins, PLG_display, PLG_charts, PLG_serial_link
```

`PLG_state.h` là header trung tâm (không phụ thuộc module nào khác) chứa toàn bộ biến toàn cục và enum, để mọi module có thể chia sẻ trạng thái mà không tạo phụ thuộc vòng.

## License

Phát hành theo giấy phép [MIT](LICENSE) — tự do sử dụng, sửa đổi và phân phối lại, chỉ cần giữ nguyên thông báo bản quyền.
