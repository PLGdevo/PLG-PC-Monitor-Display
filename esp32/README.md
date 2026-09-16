# PLG TFT LCD Task Manager — bản ESP32-S3

Bản port firmware sang ESP32-S3 (PlatformIO + Arduino core). Kế hoạch đầy đủ và backlog theo
sprint nằm ở [README_ESP32_MIGRATION.md](../README_ESP32_MIGRATION.md); bản Pico gốc vẫn ở
thư mục cha và không bị đụng tới.

**Trạng thái: Sprint 1 (tính năng ngang bằng bản Pico qua USB Serial) — code đã viết, build
sạch (0 lỗi/0 warning), chưa nghiệm thu trên phần cứng thật.**

## Quyết định kiến trúc: giữ nguyên thư viện màn hình

Tài liệu migration dự kiến chuyển sang `Adafruit_ST7789` hoặc `TFT_eSPI`. Thực tế khi khảo sát,
thư viện `ST7789_TFT_PICO` đang dùng chỉ phụ thuộc pico-SDK ở **lớp phần cứng rất mỏng** — vài
macro GPIO, 2 lời gọi SPI và 2 hàm delay — còn toàn bộ lớp đồ hoạ + 12 bộ font là code thuần.

Nên `lib/PLG_ST7789/` là bản port của chính thư viện đó: chỉ lớp phần cứng được thay bằng
Arduino API (`pinMode`/`digitalWrite`/`SPI`/`delay`), giữ nguyên lớp vẽ và font. Đổi lấy:

- Giao diện ra **giống hệt từng pixel** bản Pico, không phải căn lại toạ độ/font.
- `PLG_screens.cpp` (1100 dòng) ở Sprint 1 port được gần như nguyên văn.

## Build & nạp

```
cd esp32
pio run                 # build
pio run -t upload       # nạp qua USB-C
pio device monitor      # xem log Serial (115200)
```

Nếu `pio` chưa có trong PATH, dùng `python -m platformio` thay cho `pio`.

## Bài test nghiệm thu

`src/main.cpp` giờ là giao diện thật (giống bản Pico), không còn là màn hình test riêng của
Sprint 0. Cắm PC chạy `pc_monitor/monitor.py` (bản hiện tại, không cần sửa) rồi kiểm tra:

| Thao tác | Kỳ vọng |
|---|---|
| Cắm nguồn | Splash logo + thanh loading, dừng ở ~98% chờ dữ liệu thật (nhấn 2 lần để bỏ qua) |
| `monitor.py` gửi dữ liệu | 5 chart CPU/RAM/GPU/GPUMEM/WIFI + TEMP cập nhật, đồng hồ góc trên-phải chạy |
| Nhấn ngắn ở HOME | Chuyển sang màn hình CLOCK, nhấn ngắn lần nữa quay lại Task Manager |
| Giữ 2s | Chuyển tab HOME ↔ SETTING |
| Trong SETTING: xoay + nhấn ngắn | Duyệt PLAYER/FUNTION/MODE/ID/COLOR/FONT/TASK/LANGUAGE/CLOCK STYLE, đổi màu/font/ngôn ngữ/kiểu đồng hồ |
| Đổi màu/font/ngôn ngữ rồi rút nguồn, cắm lại | Giữ đúng lựa chọn đã chọn (NVS) |
| Icon góc phải trên | Bánh răng hiện khi ở SETTING; icon sóng/gạch chéo phản ánh `CONNECT_STATUS` |

Đây vẫn là Sprint 1 (chỉ USB Serial) — chưa có màn hình chọn kết nối (`CONNECTION`), BLE, hay
WiFi; các mục đó thuộc Sprint 2 trở đi trong `README_ESP32_MIGRATION.md`.

## Cấu trúc

```
esp32/
├── platformio.ini            # board esp32-s3-devkitc-1, USB CDC native
├── lib/PLG_ST7789/           # thư viện ST7789 port sang Arduino HAL (xem mục trên)
├── include/                  # header — hầu hết chép nguyên từ bản Pico (xem bảng dưới)
└── src/
    ├── main.cpp              # setup()/loop() — port từ PLG_TFT_LCD.cpp
    ├── PLG_pins.cpp          # pinout ESP32-S3 mới (khác bản Pico)
    ├── PLG_display.cpp       # khởi tạo SPI/TFT + ui_drawText (port sang Arduino API)
    ├── PLG_input.cpp         # encoder qua attachInterrupt + dispatch UI (port + tách tầng, xem dưới)
    ├── PLG_flash_settings.cpp# NVS (Preferences) thay cho raw flash sector
    ├── PLG_serial_link.cpp   # Serial.available()/read() thay getchar_timeout_us (port 1:1)
    ├── PLG_screens.cpp       # port gần như nguyên văn (1100 dòng, chỉ đổi sleep_ms→delay, gpio_get→digitalRead)
    ├── PLG_charts.cpp        # copy nguyên văn (không đụng phần cứng)
    └── PLG_state/theme/lang.cpp  # copy nguyên văn (không đụng phần cứng)
```

## Khác biệt so với bản Pico (đã có chủ đích)

- **Không còn `button_1`**: DevKitC-1 có sẵn nút BOOT + RESET nên bỏ nút rời vào chế độ nạp
  firmware (GPIO26 cũ).
- **`setup_pin()` tách thành `setup_display()` + `setup_input()`**: bản Pico gộp cả cấu hình
  màn hình lẫn encoder vào một hàm nằm trong `PLG_display.cpp`.
- **Lớp input tách 2 tầng**: bản Pico gọi thẳng `key_value_tang/giam()` ngay trong ISR encoder.
  Ở ESP32, ISR **bắt buộc** phải cực ngắn và nằm trong IRAM, nên ISR ở đây chỉ tăng/giảm 1 bộ
  đếm nguyên tử; toàn bộ logic điều phối (tăng giá trị đang chọn, chuyển màn hình, áp dụng +
  lưu NVS) chuyển sang `process_input()` gọi từ `loop()`. Hành vi quan sát được — mỗi nấc quay,
  mỗi lần nhấn ngắn/giữ — giống hệt bản Pico; chỉ khác *nơi* xử lý, không khác *lúc nào*.
- **Tốc độ SPI 40MHz** thay vì truyền 125000 kHz như bản Pico: trường `_speedSPIKHz` của thư
  viện là `uint16_t` (tối đa 65535) nên giá trị lớn hơn sẽ **bị tràn**, không phải bị kẹp xuống
  trần phần cứng như pico-SDK vẫn làm.

## Tiếp theo

Sprint 2 — tách lớp Transport (`PLG_transport.*`) + màn hình `SETTING → CONNECTION` chọn
USB/Bluetooth/WiFi (UI chọn mode trước, BLE/WiFi thật làm ở Sprint 3-5).
