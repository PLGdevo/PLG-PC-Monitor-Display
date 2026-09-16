# PLG TFT LCD Task Manager — bản ESP32-S3

Bản port firmware sang ESP32-S3 (PlatformIO + Arduino core). Kế hoạch đầy đủ và backlog theo
sprint nằm ở [README_ESP32_MIGRATION.md](../README_ESP32_MIGRATION.md); bản Pico gốc vẫn ở
thư mục cha và không bị đụng tới.

**Trạng thái: Sprint 0-6 đã viết xong, build sạch (0 lỗi/0 warning). Chưa nghiệm thu trên
phần cứng thật — theo Definition of Done thì chưa sprint nào được tính là Done.**

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
| SETTING → CONNECTION (mục cuối menu) | Danh sách USB (COM) / BLUETOOTH / WIFI, xoay để duyệt |
| Chọn USB, nhấn ngắn | Quay lại menu; icon kết nối vẫn phản ánh dữ liệu Serial thật (như trước) |
| Đổi mode rồi rút nguồn, cắm lại | Mở lại đúng mode đã chọn (NVS namespace `plg_net`, độc lập với `plg_ui` của màu/font/ngôn ngữ) |
| Đổi lại về USB sau khi thử mode khác | Serial nhận dữ liệu lại bình thường, icon trở lại "đã kết nối" trong ~3s |

### Bluetooth (Sprint 3)

| Thao tác | Kỳ vọng |
|---|---|
| Chọn BLUETOOTH, nhấn ngắn | Vào thẳng màn hình trạng thái BLE: tên `PLG_TFT_LCD` cỡ lớn + "Dang cho ket noi..." |
| Trên PC: `python monitor.py --ble` | Dò thấy thiết bị, bắt tay `PLG_ID?` thành công, bắt đầu gửi |
| Sau khi PC kết nối | Màn hình đổi sang "Da ket noi" (màu xanh), icon sóng góc phải bật |
| Giữ 2s → về HOME | Chart CPU/RAM/GPU/GPUMEM/WIFI/TEMP + đồng hồ cập nhật **giống hệt** như qua USB |
| Ctrl+C trên PC / tắt Bluetooth PC | Icon kết nối chuyển "mất kết nối"; board tự quảng bá lại, chạy `--ble` lần nữa là kết nối lại được |
| Nhấn ngắn ở màn hình trạng thái BLE | Quay lại menu SETTING (BLE vẫn chạy nền) |
| Xoay encoder ở màn hình trạng thái BLE | Không có gì thay đổi (không âm thầm đổi mục menu bên dưới) |

### WiFi (Sprint 4)

| Thao tác | Kỳ vọng |
|---|---|
| Chọn WIFI, nhấn ngắn | "Dang quet mang..." rồi hiện danh sách SSID kèm dBm |
| Xoay + nhấn ngắn chọn mạng | Sang bước nhập mật khẩu, tên mạng hiện ở trên |
| Xoay ở bánh xe ký tự | Ký tự giữa đổi (a-z → A-Z → 0-9 → ký tự đặc biệt → `[XONG]` → `[HUY]`, cuộn vòng) |
| Nhấn ngắn | Chốt ký tự, nối vào mật khẩu hiện bên trên; **vị trí bánh xe giữ nguyên** (gõ cụm ký tự gần nhau nhanh hơn) |
| **Giữ 2s** | Xoá lùi 1 ký tự — **không** chuyển tab HOME/SETTING như bình thường |
| Xoay tới `[XONG]`, nhấn | "Dang ket noi..." → hiện IP cỡ lớn + `TCP 5005` |
| Sai mật khẩu / quá 15s | "Ket noi that bai", nhấn ngắn để quét lại từ đầu |
| Xoay tới `[HUY]`, nhấn | Thoát wizard, về menu SETTING |
| Trên PC: `python monitor.py --wifi <IP vừa hiện>` | Bắt tay `PLG_ID?` thành công, chart cập nhật giống hệt USB/BLE |
### Lưu IP tĩnh (Sprint 5)

| Thao tác | Kỳ vọng |
|---|---|
| Ở màn hình IP, **giữ 2s** | Hiện "Da luu"; dòng gợi ý đổi thành "Giu nut = quen mang" |
| Rút nguồn, cắm lại | Tự kết nối thẳng bằng IP tĩnh đã lưu — **không** chạy lại wizard, `monitor.py --wifi <IP cũ>` dùng được ngay |
| Vào lại SETTING → CONNECTION → WIFI | Vào thẳng "Dang ket noi..." rồi ra màn hình IP, bỏ qua bước quét/nhập mật khẩu |
| Đổi mật khẩu router / mang board đi chỗ khác | Sau ~15s không vào được → **tự** quay lại bước quét, không kẹt ở màn hình lỗi |
| Ở màn hình IP (đã lưu), **giữ 2s** | Quên mạng, quay lại bước quét để chọn mạng khác |
| Quên mạng rồi rút nguồn, cắm lại | Phải chạy lại wizard (đúng — cấu hình đã bị xoá), nhưng **vẫn nhớ** mode WIFI đã chọn |

## Hướng dẫn kết nối máy tính với board

Board nhận dữ liệu qua **một** trong ba đường truyền, chọn ngay trên thiết bị ở
**SETTING → CONNECTION** (giữ nút 2s ở HOME để vào SETTING, xoay tới CONNECTION, nhấn ngắn).
Lựa chọn được ghi nhớ qua mất nguồn.

Trên máy tính, cài một lần:

```bash
cd pc_monitor
pip install -r requirements.txt
```

### 1. USB (mặc định)

Cắm cáp USB-C, rồi:

```bash
python monitor.py
```

Script tự dò cổng COM và tự xác thực đúng board (gửi `PLG_ID?`, chờ đúng câu trả lời) nên không
cần chọn cổng. Ép cổng cụ thể bằng `--port COM5` nếu cần.

### 2. Bluetooth (BLE)

1. Trên board: **SETTING → CONNECTION → BLUETOOTH**, nhấn ngắn. Màn hình hiện tên thiết bị
   `PLG_TFT_LCD` và "Dang cho ket noi...".
2. Trên máy tính:

   ```bash
   python monitor.py --ble
   ```

Không cần ghép nối (pair) trong Windows Settings trước — script tự dò theo tên và kết nối. Khi
board hiện "Da ket noi" là dữ liệu đã chạy; giữ nút 2s để về HOME xem biểu đồ.

### 3. WiFi

Lần đầu phải chọn mạng và nhập mật khẩu ngay trên thiết bị:

1. **SETTING → CONNECTION → WIFI**, nhấn ngắn → board quét mạng xung quanh.
2. Xoay encoder chọn mạng (có kèm cường độ tín hiệu dBm), nhấn ngắn.
3. Nhập mật khẩu bằng bánh xe ký tự:
   - **xoay** = đổi ký tự (`a-z` → `A-Z` → `0-9` → ký tự đặc biệt → `[XONG]` → `[HUY]`)
   - **nhấn ngắn** = chốt ký tự đang chọn
   - **giữ 2s** = xoá lùi 1 ký tự
   - xoay tới `[XONG]` rồi nhấn để kết nối, hoặc `[HUY]` để thoát
4. Kết nối xong, màn hình hiện **địa chỉ IP** cỡ lớn. Gõ địa chỉ đó sang máy tính:

   ```bash
   python monitor.py --wifi 192.168.1.50
   ```

5. **Giữ nút 2s** ở màn hình IP để lưu lại thành IP tĩnh. Từ lần sau board tự kết nối thẳng
   bằng đúng địa chỉ đó khi cắm điện — không phải nhập lại mật khẩu, và câu lệnh trên máy tính
   vẫn dùng nguyên IP cũ.

Muốn đổi sang mạng khác: vào lại màn hình IP rồi **giữ nút 2s** để quên mạng đã lưu, board sẽ
quét lại từ đầu. Nếu mạng đã lưu không còn dùng được (đổi mật khẩu, mang board đi chỗ khác),
sau khoảng 15 giây board tự quay lại bước quét.

> Giao diện đồ hoạ (`python monitor.py --gui`) hiện **chỉ hỗ trợ USB**. BLE và WiFi dùng qua
> dòng lệnh với `--ble` / `--wifi <IP>`.

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
    ├── PLG_flash_settings.cpp# NVS (Preferences) thay cho raw flash sector — mau/font/ngôn ngữ
    ├── PLG_protocol.cpp      # parse "CPU:..;RAM:..\n" + bắt tay PLG_ID? — dùng chung cho cả 3 đường truyền
    ├── PLG_transport.cpp     # dispatcher chọn USB/BLE/WiFi, riêng NVS namespace "plg_net" (mode)
    ├── PLG_transport_ble.cpp # BLE thật (NimBLE, Nordic UART Service) — Sprint 3
    ├── PLG_transport_wifi.cpp# WiFi thật: quét/kết nối + TCP server 5005 + lưu IP tĩnh (NVS)
    ├── PLG_wifi_ui.cpp       # wizard nhiều bước: quét → chọn SSID → wheel-picker mật khẩu → IP
    ├── PLG_serial_link.cpp   # chỉ còn đọc byte từ Serial rồi đưa vào PLG_protocol
    ├── PLG_screens.cpp       # port gần như nguyên văn (1100 dòng) + MONITOR_CONNECTION/MONITOR_BLE_STATUS
    ├── PLG_charts.cpp        # copy nguyên văn (không đụng phần cứng)
    └── PLG_state/theme/lang.cpp  # copy nguyên văn + field mới cho SETTING > CONNECTION
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
- **`CONNECT_STATUS` giờ mới thực sự được dùng**: trong bản Pico, biến này tồn tại và có vẽ icon
  ở `MONITOR_STATUS()` nhưng **không có nơi nào từng gán `true`** — icon kết nối luôn hiện trạng
  thái "mất kết nối". `PLG_transport.cpp` là nơi đầu tiên gán giá trị này (theo
  `serial_link_is_connected()`/`transport_ble_is_connected()`/`transport_wifi_is_connected()`).
- **NVS tách 2 namespace**: `plg_ui` (màu/font/ngôn ngữ/kiểu đồng hồ, trong `PLG_flash_settings.cpp`)
  và `plg_net` (giao thức kết nối, trong `PLG_transport.cpp`) — độc lập vì khác bản chất cấu hình,
  và Sprint 5 sẽ cần thêm SSID/mật khẩu/IP tĩnh vào đúng namespace `plg_net` này.

- **Callback BLE không được ghi thẳng vào state UI**: callback của NimBLE chạy trong task riêng,
  không phải `loop()`. Ghi thẳng vào `chart_cpu[]`/`current_time_str` từ đó sẽ tranh chấp với
  `loop()` đang đọc chính các biến ấy để vẽ. Nên callback chỉ đẩy byte thô vào một FreeRTOS
  StreamBuffer, còn `transport_ble_poll()` (chạy trong `loop()`) mới parse.

- **Wizard WiFi đảo nghĩa nút giữ**: ở mọi màn hình khác, giữ 2s = chuyển tab HOME/SETTING.
  Trong lúc gõ mật khẩu thì xoá lùi là thao tác cần đến nhiều hơn hẳn, nên `process_input()`
  trao toàn quyền điều khiển cho wizard khi nó đang mở; thoát wizard bằng mục `[HUY]` trong
  bảng ký tự.
- **Quét WiFi chạy bất đồng bộ**: `WiFi.scanNetworks()` mặc định chặn 2-5 giây — đủ để đứng
  hình và treo cả encoder. Dùng bản async rồi hỏi `WiFi.scanComplete()` mỗi vòng `loop()`.

- **Xoá cấu hình WiFi phải xoá từng key, không `prefs.clear()`**: namespace `plg_net` còn giữ cả
  lựa chọn giao thức (`mode`) — xoá sạch sẽ làm "quên mạng" kéo theo mất luôn lựa chọn WIFI,
  đưa thiết bị về USB một cách khó hiểu.

## Còn lại

Toàn bộ backlog trong `README_ESP32_MIGRATION.md` đã được viết. Việc còn lại là **chạy thử
trên board thật** theo bảng nghiệm thu bên trên — không có phần cứng thì không story nào
được tính là Done.
