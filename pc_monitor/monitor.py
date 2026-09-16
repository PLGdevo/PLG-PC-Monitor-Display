#!/usr/bin/env python3
"""
PLG PC Task Monitor
--------------------
Doc thong so CPU / RAM / GPU / toc do mang tren may tinh va gui qua cong Serial (USB)
xuong board Raspberry Pi Pico (PLG_TFT_LCD_TASKMANAGER).

Tu dong do va xac thuc dung board PLG (khong can chon cong thu cong): script gui
lenh "PLG_ID?" xuong tung cong serial, chi coi la dung thiet bi khi nhan lai dung
cau tra loi "I AM PLG_TFT_LCD_TASKMANAGER" tu firmware.

Yeu cau cai dat:
    pip install -r requirements.txt

Chay:
    python monitor.py                  # tu do + tu xac thuc board PLG
    python monitor.py --port COM5      # ep dung cong nay, bo qua buoc do/xac thuc
    python monitor.py --interval 0.5   # doi khoang gui du lieu (giay)
    python monitor.py --list           # liet ke cac cong serial dang co
    python monitor.py --ble            # gui qua Bluetooth LE (board ESP32-S3)
    python monitor.py --wifi 192.168.1.50  # gui qua WiFi toi IP hien tren man hinh board

Dinh dang du lieu gui xuong board, moi dong ket thuc bang '\n':
    CPU:<int>;RAM:<int>;GPU:<int>;GPUMEM:<int>;WIFI:<int>;TEMP:<int>;TIME:<HH:MM:SS>;DATE:<DD/MM/YYYY>;BAT:<int>

Trong do CPU/RAM/GPU/GPUMEM la phan tram 0-100, TEMP la do C (kep 0-100), va WIFI
gio la tong bang thong mang (download+upload) theo Mbps, kep 0-127 vi firmware
luu duoi dang int8_t. GPU/WIFI/TEMP/BAT se la -1 neu khong doc duoc (khong co GPU
NVIDIA, chua co moc so sanh toc do mang lan dau, may khong ho tro doc nhiet do CPU
qua ACPI, hoac may khong co pin nhu PC ban).
TIME/DATE la gio va ngay hien tai cua may tinh, dung de hien thi dong ho tren
man hinh thiet bi. BAT la pin cua laptop, hien thi o icon pin ben trai.
"""

import argparse
import asyncio
import re
import sys
import threading
import time

import psutil
import serial
import serial.tools.list_ports
from colorama import Fore, Style
from colorama import init as colorama_init

# Chi bat mau khi in ra console that (isatty); khi bi redirect ra file/log thi
# tat mau de tranh ghi rac ma escape ANSI vao log. colorama_init dich cac ma ANSI
# nay sang WinAPI console tren cmd.exe/PowerShell cu khong tu ho tro ANSI.
USE_COLOR = bool(sys.stdout) and sys.stdout.isatty()
if USE_COLOR:
    colorama_init(autoreset=True)


def _c(text: str, color: str) -> str:
    """Boc mau ANSI quanh text neu dang in ra console that, giu nguyen text neu
    khong (vd khi bi redirect ra file) de khong lam ban log bang ma escape."""
    return f"{color}{text}{Style.RESET_ALL}" if USE_COLOR else text


def _set_console_icon() -> None:
    """Gan icon PLG cho cua so console dang chay (taskbar + title bar).

    Console app (console=True trong PyInstaller) chay trong cua so do conhost.exe
    quan ly rieng, KHONG tu dong lay icon nhung tai nguyen (.ico) da nhung trong file
    .exe - nen dau du .exe da co icon PLG (thay dung trong File Explorer), khi mo/chay
    thu cong thi taskbar/title bar van hien icon console mac dinh cua Windows. Phai
    tu trich icon tu chinh file exe dang chay (ExtractIconW) roi gan bang WM_SETICON."""
    if not sys.platform.startswith("win"):
        return
    try:
        import ctypes

        exe_path = sys.executable  # duong dan .exe da build (khi chay bang PyInstaller)
        hicon_large = ctypes.windll.shell32.ExtractIconW(0, exe_path, 0)
        if hicon_large <= 1:  # 0 = khong co icon, 1 = duong dan khong hop le
            return
        hwnd = ctypes.windll.kernel32.GetConsoleWindow()
        if not hwnd:
            return
        WM_SETICON = 0x0080
        ICON_SMALL, ICON_BIG = 0, 1
        ctypes.windll.user32.SendMessageW(hwnd, WM_SETICON, ICON_SMALL, hicon_large)
        ctypes.windll.user32.SendMessageW(hwnd, WM_SETICON, ICON_BIG, hicon_large)
    except Exception:
        pass


_set_console_icon()


def _fmt_metric(label: str, value: int, warn: int = 80, danger: int = 95, unit: str = "%", invert: bool = False) -> str:
    """Dinh dang 1 chi so (CPU/RAM/GPU/...) co mau theo nguong. Mac dinh (invert=
    False, dung cho CPU/RAM/GPU/VRAM): xanh la binh thuong, vang khi >= warn, do
    khi >= danger - gia tri CAO la dang lo. invert=True (dung cho WIFI/BAT): dao
    nguoc chieu so sanh - gia tri THAP moi la dang lo (vd pin/tin hieu yeu).
    Gia tri -1 (khong doc duoc) hien "N/A" mau xam thay vi "-1%" gay hieu lam."""
    if value is None or value < 0:
        return _c(f"{label} N/A", Fore.LIGHTBLACK_EX)
    if invert:
        color = Fore.RED if value <= danger else Fore.YELLOW if value <= warn else Fore.GREEN
    else:
        color = Fore.RED if value >= danger else Fore.YELLOW if value >= warn else Fore.GREEN
    return _c(f"{label} {value:>3}{unit}", color)


def format_console_line(m: dict) -> str:
    """Dong log console dang bang, day du thong tin, de doc hon nhieu so voi in
    thang chuoi payload tho (CPU:45;RAM:62;...) - dung khi debug bang mat."""
    parts = [
        _c(f"[{m['time']}]", Fore.CYAN),
        _fmt_metric("CPU", m["cpu"]),
        _fmt_metric("RAM", m["ram"]),
        _fmt_metric("GPU", m["gpu"]),
        _fmt_metric("VRAM", m["gpu_mem"]),
        _fmt_metric("TEMP", m["temp"], warn=75, danger=85, unit="C"),
        _fmt_metric("NET", m["wifi"], warn=5, danger=1, unit="Mbps", invert=True),
        _fmt_metric("BAT", m["bat"], warn=30, danger=15, invert=True),
    ]
    return "  ".join(parts)


try:
    import pynvml

    pynvml.nvmlInit()
    _NVML_OK = True
except Exception:
    _NVML_OK = False

# Fallback cho GPU khong phai NVIDIA (Intel/AMD): dung WMI performance counter
# "GPU Engine" / "GPU Adapter Memory" co san tren Windows 10 1803+ (khong can driver rieng).
# Query WMI GPUEngine/GPUAdapterMemory co the mat toi 4-5 giay moi lan (rat cham
# so voi psutil). Neu goi truc tiep tu vong lap gui chinh (moi 0.8s) se lam ca
# chuong trinh dung khung dinh ky moi khi cache het han -> "gui du lieu cham".
# Giai phap: chay query nay o 1 thread nen rieng, cap nhat cache moi WMI_THROTTLE_SEC
# giay; vong lap gui chinh chi doc gia tri cache cuoi cung, khong bao gio phai cho.
WMI_THROTTLE_SEC = 5.0
_gpu_cache = {"val": None}
_gpu_mem_cache = {"val": None}
_WMI_OK = False

if sys.platform.startswith("win"):
    try:
        import threading
        import wmi

        def _gpu_wmi_worker():
            import pythoncom

            pythoncom.CoInitialize()  # can thiet vi conn tao/dung trong 1 thread rieng (COM apartment)
            try:
                conn = wmi.WMI(namespace="root\\cimv2")
            except Exception:
                return
            while True:
                try:
                    total = 0
                    for e in conn.Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine():
                        if "engtype_3D" in e.Name:
                            total += int(e.UtilizationPercentage)
                    _gpu_cache["val"] = min(total, 100)
                except Exception:
                    _gpu_cache["val"] = None
                try:
                    total_ram = 0
                    for c in conn.Win32_VideoController():
                        if c.AdapterRAM:
                            total_ram = max(total_ram, int(c.AdapterRAM))
                    if total_ram <= 0:
                        _gpu_mem_cache["val"] = None
                    else:
                        used = 0
                        for m in conn.Win32_PerfFormattedData_GPUPerformanceCounters_GPUAdapterMemory():
                            used += int(m.DedicatedUsage or 0) + int(m.SharedUsage or 0)
                        _gpu_mem_cache["val"] = min(int(used * 100 / total_ram), 100)
                except Exception:
                    _gpu_mem_cache["val"] = None
                time.sleep(WMI_THROTTLE_SEC)

        threading.Thread(target=_gpu_wmi_worker, daemon=True).start()
        _WMI_OK = True
    except Exception:
        _WMI_OK = False


def _wmi_gpu_percent() -> int | None:
    """Tong % su dung engine 3D cua tat ca process/GPU (khop voi cot GPU trong Task Manager).
    Doc tu cache duoc thread nen cap nhat, khong bao gio block vong lap gui chinh."""
    return _gpu_cache["val"]


def _wmi_gpu_mem_percent() -> int | None:
    """% bo nho GPU da dung (dedicated + shared) tren tong AdapterRAM khai bao.
    Doc tu cache duoc thread nen cap nhat, khong bao gio block vong lap gui chinh."""
    return _gpu_mem_cache["val"]


# Nhiet do CPU: Windows khong co API chuan/on dinh cho viec nay (psutil.sensors_temperatures()
# hau nhu luon rong tren Windows, va namespace WMI cua LibreHardwareMonitor khong phai lam nao/
# ban nao cung tu dong dang ky - da thu va bi loi "invalid namespace" ngay ca khi chay Admin).
# Thay vao do doc tu "Remote Web Server" co san cua LibreHardwareMonitor/OpenHardwareMonitor
# (Options > Remote Web Server > Run trong app, mac dinh port 8085) - cho ra JSON qua HTTP thuan,
# khong can quyen Admin, khong phu thuoc WMI. Neu server chua bat/chua co: TEMP se la -1 nhu cac
# chi so khong doc duoc khac.
TEMP_THROTTLE_SEC = 5.0
HWMONITOR_WEB_URLS = ("http://localhost:8085/data.json", "http://127.0.0.1:8085/data.json")
_temp_cache = {"val": None}
_TEMP_HTTP_OK = False


def _find_cpu_temp_node(node: dict) -> int | None:
    """Duyet de quy cay JSON cua LibreHardwareMonitor Web Server, tim node nhiet do CPU tot nhat.
    Cau truc: moi node co 'Text' (ten), 'Children' (list), va la (leaf) thi co 'Value' dang chuoi
    "45.0 C" hoac tuong tu. Uu tien node ten chua 'package'/'tctl'/'tdie' (nhiet do tong CPU),
    neu khong tim thay uu tien nao thi lay gia tri lon nhat trong cac node nhiet do duoi 1 khoi
    co ten chua 'cpu' (vd cac core rieng le)."""
    best_priority: float | None = None
    best_fallback: float | None = None

    def parse_celsius(text: str) -> float | None:
        # Vi du thuc te tu LibreHardwareMonitor: "74.0 °C" (dau do C). Ky tu "°" doi khi bi
        # loi encoding (vd thanh "�") tuy nguon/console, nen KHONG dua vao vi tri ky tu do -
        # chi lay phan so o DAU chuoi bang regex, va rieng kiem tra co chu 'C' (khong phan biet
        # hoa/thuong) o dau chuoi con lai de chac chan day la don vi nhiet do (khong phai %/V/W/MHz).
        text = text.strip()
        m = re.match(r"^(-?\d+(?:\.\d+)?)\s*(.*)$", text)
        if not m:
            return None
        number_str, unit = m.group(1), m.group(2)
        if "c" not in unit.lower():
            return None
        try:
            return float(number_str)
        except ValueError:
            return None

    def walk(n: dict, under_cpu: bool):
        nonlocal best_priority, best_fallback
        name = (n.get("Text") or "")
        name_l = name.lower()
        is_cpu_branch = under_cpu or "cpu" in name_l
        children = n.get("Children") or []
        if not children:
            val = parse_celsius(n.get("Value", ""))
            if val is not None and is_cpu_branch:
                if "package" in name_l or "tctl" in name_l or "tdie" in name_l:
                    if best_priority is None or val > best_priority:
                        best_priority = val
                else:
                    if best_fallback is None or val > best_fallback:
                        best_fallback = val
            return
        for child in children:
            walk(child, is_cpu_branch)

    walk(node, False)
    chosen = best_priority if best_priority is not None else best_fallback
    return int(round(chosen)) if chosen is not None else None


if sys.platform.startswith("win"):
    try:
        def _temp_http_worker():
            import json
            import urllib.request

            while True:
                val = None
                for url in HWMONITOR_WEB_URLS:
                    try:
                        with urllib.request.urlopen(url, timeout=2) as resp:
                            data = json.load(resp)
                        val = _find_cpu_temp_node(data)
                        break
                    except Exception:
                        continue
                _temp_cache["val"] = max(0, min(100, val)) if val is not None else None
                time.sleep(TEMP_THROTTLE_SEC)

        threading.Thread(target=_temp_http_worker, daemon=True).start()
        _TEMP_HTTP_OK = True
    except Exception:
        _TEMP_HTTP_OK = False


def get_cpu_temp() -> int:
    """Nhiet do CPU theo do C (0-100, kep an toan). Tra -1 neu khong doc duoc (LibreHardwareMonitor/
    OpenHardwareMonitor chua chay, hoac chua bat Options > Remote Web Server > Run trong app)."""
    if _TEMP_HTTP_OK:
        val = _temp_cache["val"]
        if val is not None:
            return val
    return -1


SETTINGS_PATH = __import__("pathlib").Path(
    __import__("os").getenv("APPDATA") or __import__("pathlib").Path.home()
) / "PLG_TFT_LCD_TASKMANAGER" / "monitor_settings.json"


def load_settings() -> dict:
    """Doc lai cau hinh ket noi (mode/port/baud/interval) tu lan chay truoc, luu o
    %APPDATA%\\PLG_TFT_LCD_TASKMANAGER\\monitor_settings.json - de nguoi dung khong
    phai chon lai Manual/cong/baud moi lan mo app. Tra dict rong neu chua co file
    hoac file loi (khong lam sap app vi ly do phu nay)."""
    import json

    try:
        with open(SETTINGS_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def save_settings(settings: dict) -> None:
    """Ghi lai cau hinh ket noi hien tai, bo qua loi (vd khong co quyen ghi) vi
    day chi la tien ich nho, khong anh huong chuc nang chinh cua app."""
    import json

    try:
        SETTINGS_PATH.parent.mkdir(parents=True, exist_ok=True)
        with open(SETTINGS_PATH, "w", encoding="utf-8") as f:
            json.dump(settings, f)
    except Exception:
        pass


PICO_VID_PID = {
    (0x2E8A, 0x0005),  # RP2 Boot (BOOTSEL)
    (0x2E8A, 0x000A),  # Pico CDC (stdio USB mac dinh cua Pico SDK)
}

# Bat tay xac thuc thiet bi: gui cau lenh nay xuong cong serial, chi coi la dung
# board PLG neu nhan lai dung cau tra loi IDENTITY_REPLY. Tranh truong hop VID/PID
# trung voi mot thiet bi USB CDC khac (vd Pico chay firmware khac) roi gui nham
# du lieu xuong do -> khong con phai nguoi dung tu chon cong bang tay.
IDENTITY_CMD = b"PLG_ID?\n"
IDENTITY_REPLY = "I AM PLG_TFT_LCD_TASKMANAGER"

# Khoang cho giua cac lan thu lai khi chua tim/mo duoc cong - dat ngan vi ban than
# identify_device()/find_pico_port() da tu gioi han thoi gian (khong bao gio treo),
# nen khong can chờ them nhieu o day; giup phat hien lai board nhanh hon sau khi cam.
RETRY_DELAY = 0.5


def _identify_device_worker(port: str, baud: int, timeout: float, result: dict) -> None:
    try:
        ser = serial.Serial(port, baud, timeout=0.3)
    except serial.SerialException:
        return
    try:
        time.sleep(0.15)  # cho board on dinh sau khi mo cong (DTR toggle co the reset board)
        ser.reset_input_buffer()
        ser.write(IDENTITY_CMD)
        buf = ""
        deadline = time.time() + timeout
        while time.time() < deadline:
            chunk = ser.read(ser.in_waiting or 1).decode("ascii", errors="ignore")
            if chunk:
                buf += chunk
                if IDENTITY_REPLY in buf:
                    ser.timeout = 1  # tra ve timeout binh thuong dung cho vong gui du lieu
                    result["ser"] = ser
                    return
    except serial.SerialException:
        pass
    safe_close(ser)


def identify_device(port: str, baud: int, timeout: float = 1.0, hard_timeout: float = 1.5) -> serial.Serial | None:
    """Mo cong serial, gui IDENTITY_CMD va cho board tra loi IDENTITY_REPLY.
    Neu dung la board PLG, TRA VE luon doi tuong Serial dang mo (khong dong lai)
    de vong gui du lieu dung tiep ngay - tranh phai dong roi mo lai cong ngay sau
    do, vi tren Windows mo lai qua nhanh mot cong USB CDC vua duoc giai phong
    (nhat la vua cam lai thiet bi) de bi loi "Access is denied" thoang qua.

    Toan bo qua trinh chay trong 1 thread rieng, chi cho toi da `hard_timeout`
    giay. Ly do: cac cong "Standard Serial over Bluetooth link" ao tren Windows
    co the TREO RAT LAU (vai chuc giay) ngay tai buoc mo cong (CreateFile), vuot
    xa timeout doc/ghi thong thuong cua pyserial - neu khong gioi han cung, mot
    cong Bluetooth "ket" se lam nghen ca vong quet, khien no khong kip thu toi
    cong that cua board (nhat la sau khi rut/cam lai, so thu tu cong co the da
    doi). Qua han thi bo qua cong nay, chuyen ngay sang cong tiep theo.

    Tra None neu khong phai board PLG, khong mo/ket noi duoc, hoac qua han."""
    result: dict = {"ser": None}
    t = threading.Thread(target=_identify_device_worker, args=(port, baud, timeout, result), daemon=True)
    t.start()
    t.join(hard_timeout)
    return result["ser"]


def safe_close(ser: serial.Serial, timeout: float = 1.0) -> None:
    """Dong cong serial nhung khong bao gio cho vo han. Tren Windows, dong mot
    handle COM sau khi thiet bi da bi rut vat ly co the TREO VO THOI HAN (khong
    nem exception, khong tu timeout) - lam ca vong lap ket noi lai bi ket cung
    ngay tai buoc dong cong cu, khong bao gio toi duoc buoc mo lai (bien hien
    thanh "cam lai thiet bi nhung khong bao gio ket noi lai duoc"). Chay close()
    trong 1 thread daemon rieng va chi cho toi da `timeout` giay; neu qua han thi
    bo qua (thread rac se tu ket thuc khi OS giai phong handle, hoac khi tien
    trinh chinh thoat), van tiep tuc vong lap thay vi treo may."""
    t = threading.Thread(target=ser.close, daemon=True)
    t.start()
    t.join(timeout)


def find_pico_port(baud: int) -> serial.Serial | None:
    """Tu dong do va xac thuc board PLG, khong can nguoi dung chon cong thu cong.
    Uu tien thu cac cong co VID/PID giong Pico truoc (nhanh), sau do thu them cac
    cong serial con lai (phong khi build/driver khac lam VID/PID khac di). Chi
    nhan cong nao xac thuc thanh cong qua identify_device(), tra ve Serial dang mo."""
    ports = list(serial.tools.list_ports.comports())
    likely = [p for p in ports if p.vid is not None and p.pid is not None and (p.vid, p.pid) in PICO_VID_PID]
    others = [p for p in ports if p not in likely]
    for p in likely + others:
        ser = identify_device(p.device, baud)
        if ser is not None:
            return ser
    return None


def get_cpu_percent() -> int:
    return int(psutil.cpu_percent(interval=None))


def get_ram_percent() -> int:
    return int(psutil.virtual_memory().percent)


def get_gpu_percent() -> int:
    """% su dung GPU 3D. Uu tien NVML (GPU NVIDIA, chinh xac nhat); neu khong co
    NVIDIA thi fallback sang WMI performance counter (Intel/AMD, Windows 10 1803+)."""
    if _NVML_OK:
        try:
            handle = pynvml.nvmlDeviceGetHandleByIndex(0)
            util = pynvml.nvmlDeviceGetUtilizationRates(handle)
            return int(util.gpu)
        except Exception:
            pass
    if _WMI_OK:
        val = _wmi_gpu_percent()
        if val is not None:
            return val
    return -1


def get_gpu_mem_percent() -> int:
    """% VRAM da dung (GPU memory), rieng voi % GPU 3D (usage core). Uu tien NVML,
    fallback WMI (dedicated+shared / AdapterRAM) cho GPU Intel/AMD."""
    if _NVML_OK:
        try:
            handle = pynvml.nvmlDeviceGetHandleByIndex(0)
            mem = pynvml.nvmlDeviceGetMemoryInfo(handle)
            return int(mem.used * 100 / mem.total)
        except Exception:
            pass
    if _WMI_OK:
        val = _wmi_gpu_mem_percent()
        if val is not None:
            return val
    return -1


_net_prev = {"t": None, "bytes": 0}


def get_net_mbps() -> int:
    """Tong bang thong mang hien tai (download+upload) theo Mbps, do bang chenh lech
    byte cua psutil.net_io_counters() giua 2 lan goi lien tiep chia cho thoi gian
    troi qua. Lan goi dau tien chua co moc so sanh nen tra -1 (giong quy uoc "khong
    doc duoc" cua cac chi so khac); firmware hien thi thanh gia tri kep 0-127 (int8_t)
    nen toc do tren ~127 Mbps se bi cat o 127 khi hien thi tren man hinh."""
    try:
        counters = psutil.net_io_counters()
        now = time.monotonic()
        total_bytes = counters.bytes_sent + counters.bytes_recv
        prev_t, prev_bytes = _net_prev["t"], _net_prev["bytes"]
        _net_prev["t"], _net_prev["bytes"] = now, total_bytes
        if prev_t is None:
            return -1
        dt = now - prev_t
        if dt <= 0:
            return -1
        mbps = (total_bytes - prev_bytes) * 8 / 1_000_000 / dt
        return max(0, min(127, int(round(mbps))))
    except Exception:
        return -1


def get_time_str() -> str:
    return time.strftime("%H:%M:%S")


def get_date_str() -> str:
    return time.strftime("%d/%m/%Y")


def get_battery_percent() -> int:
    """Pin cua laptop theo %. Tra -1 neu may khong co pin (PC ban) hoac khong doc duoc."""
    try:
        info = psutil.sensors_battery()
        if info is None:
            return -1
        return int(info.percent)
    except Exception:
        return -1


def gather_metrics() -> dict:
    return {
        "cpu": get_cpu_percent(),
        "ram": get_ram_percent(),
        "gpu": get_gpu_percent(),
        "gpu_mem": get_gpu_mem_percent(),
        "wifi": get_net_mbps(),
        "temp": get_cpu_temp(),
        "time": get_time_str(),
        "date": get_date_str(),
        "bat": get_battery_percent(),
    }


def build_payload(m: dict) -> str:
    return (
        f"CPU:{m['cpu']};RAM:{m['ram']};GPU:{m['gpu']};GPUMEM:{m['gpu_mem']};WIFI:{m['wifi']};TEMP:{m['temp']};"
        f"TIME:{m['time']};DATE:{m['date']};BAT:{m['bat']}\n"
    )


def format_log_message(m: dict) -> str:
    """Dong noi dung ngan gon cho 1 hang trong bang Log cua GUI (cot Message), khong
    mau/khong nhan - danh cho Treeview thay vi Text co mau nhu console."""
    def val(key: str, unit: str) -> str:
        v = m[key]
        return f"{v}{unit}" if v is not None and v >= 0 else "N/A"

    return (f"CPU: {val('cpu', '%')}, RAM: {val('ram', '%')}, GPU: {val('gpu', '%')}, "
            f"VRAM: {val('gpu_mem', '%')}, TEMP: {val('temp', 'C')}, NET: {val('wifi', 'Mbps')}, "
            f"BAT: {val('bat', '%')}")


class ConnectionWorker:
    """Chay vong lap ket noi + gui du lieu (giong het logic console_main) trong 1
    thread nen, bao cao trang thai/log/metric ve GUI qua cac callback thay vi in
    thang ra stdout. Dung chung cho ca 4 che do: Auto/Manual (USB Serial),
    Bluetooth (BLE) va WiFi (TCP) - xem _run()."""

    def __init__(self, mode: str, port: str | None, baud: int, interval: float,
                 on_log, on_metrics, on_status) -> None:
        self.mode = mode  # "auto" | "manual" | "bluetooth" | "wifi"
        # O che do wifi, o nhap dia chi dung chung voi o chon cong nen truong nay giu IP
        # cua board thay vi ten cong COM.
        self.fixed_port = port
        self.baud = baud
        self.interval = interval
        self.on_log = on_log
        self.on_metrics = on_metrics
        self.on_status = on_status
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self, wait: float = 0.0) -> None:
        self._stop.set()
        if wait > 0 and self._thread is not None:
            self._thread.join(wait)

    def _run(self) -> None:
        psutil.cpu_percent(interval=None)  # lan goi dau tra ve 0.0, bo qua de lay mau chuan
        time.sleep(0.2)
        try:
            if self.mode == "bluetooth":
                self._run_ble()
            elif self.mode == "wifi":
                self._run_wifi()
            else:
                self._run_serial()
        finally:
            self.on_status("🔴 Chua ket noi")

    # ---------------- USB Serial (che do auto/manual) ----------------
    def _run_serial(self) -> None:
        try:
            while not self._stop.is_set():
                if self.mode == "manual" and self.fixed_port:
                    ser = identify_device(self.fixed_port, self.baud)
                    if ser is None:
                        try:
                            ser = serial.Serial(self.fixed_port, self.baud, timeout=1)
                            self.on_log("WARN", f"Cong {self.fixed_port} khong tra loi xac thuc "
                                        f"\"{IDENTITY_REPLY}\", co the khong phai board PLG. Van tiep tuc.", "warn")
                        except serial.SerialException as exc:
                            self.on_log("ERROR", f"Loi mo cong {self.fixed_port}: {exc}, thu lai sau {RETRY_DELAY}s", "error")
                            self.on_status("🟡 Dang cho...")
                            if self._stop.wait(RETRY_DELAY):
                                return
                            continue
                    port = self.fixed_port
                else:
                    self.on_status("🟡 Dang tim board PLG...")
                    ser = find_pico_port(self.baud)
                    if ser is None:
                        if self._stop.wait(RETRY_DELAY):
                            return
                        continue
                    port = ser.port

                self.on_status(f"🟢 Da ket noi: {port} @ {self.baud} baud")
                self.on_log("OK", f"Da xac thuc board PLG tren {port} @ {self.baud} baud, gui du lieu moi {self.interval}s.", "ok")
                try:
                    while not self._stop.is_set():
                        m = gather_metrics()
                        payload = build_payload(m)
                        ser.write(payload.encode("ascii"))
                        self.on_metrics(m)
                        self.on_log("DATA", format_log_message(m), "data")
                        if self._stop.wait(self.interval):
                            break
                    safe_close(ser)
                    if self._stop.is_set():
                        self.on_status("🔴 Chua ket noi")
                        return
                except serial.SerialException as exc:
                    self.on_log("ERROR", f"Mat ket noi serial: {exc}, thu ket noi lai...", "error")
                    self.on_status("🟡 Mat ket noi, dang thu lai...")
                    safe_close(ser)
                    if self._stop.wait(RETRY_DELAY):
                        return
                    continue
        except Exception as exc:  # loi ngoai du tinh: bao ra GUI thay vi chet lang le trong thread
            self.on_log("ERROR", f"Loi khong mong doi: {exc}", "error")

    # ---------------- Bluetooth LE ----------------
    def _run_ble(self) -> None:
        try:
            import bleak  # noqa: F401  chi de bao loi som, ro rang neu chua cai
        except ImportError:
            self.on_log("ERROR", "Che do Bluetooth can goi 'bleak': chay "
                                 "'pip install bleak' rui mo lai.", "error")
            return
        # bleak chay tren asyncio; thread nen nay chua co event loop nao nen asyncio.run()
        # tu tao mot cai rieng - khong dung cham gi toi vong lap cua Tkinter o thread chinh.
        asyncio.run(self._ble_loop())

    async def _ble_loop(self) -> None:
        from bleak import BleakClient, BleakScanner

        while not self._stop.is_set():
            self.on_status(f"🟡 Dang do tim \"{BLE_DEVICE_NAME}\"...")
            try:
                device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME, timeout=10.0)
                if device is None:
                    self.on_log("WARN", f"Khong thay \"{BLE_DEVICE_NAME}\". Kiem tra board da chon "
                                        f"SETTING > CONNECTION > BLUETOOTH chua.", "warn")
                    await asyncio.sleep(RETRY_DELAY)
                    continue

                async with BleakClient(device) as client:
                    reply_seen = asyncio.Event()
                    buf = ""

                    def on_notify(_sender, data: bytearray) -> None:
                        nonlocal buf
                        buf += data.decode("ascii", errors="ignore")
                        if IDENTITY_REPLY in buf:
                            reply_seen.set()

                    await client.start_notify(NUS_TX_CHAR_UUID, on_notify)
                    await _ble_send_line(client, IDENTITY_CMD.decode("ascii"))
                    try:
                        await asyncio.wait_for(reply_seen.wait(), timeout=5.0)
                    except asyncio.TimeoutError:
                        self.on_log("ERROR", f"Thiet bi khong tra loi xac thuc \"{IDENTITY_REPLY}\", "
                                             f"co the dang chay firmware khac.", "error")
                        await asyncio.sleep(RETRY_DELAY)
                        continue

                    self.on_status(f"🟢 Da ket noi BLE: {device.address}")
                    self.on_log("OK", f"Da xac thuc board PLG qua BLE ({device.address}), "
                                      f"gui du lieu moi {self.interval}s.", "ok")
                    while client.is_connected and not self._stop.is_set():
                        m = gather_metrics()
                        await _ble_send_line(client, build_payload(m))
                        self.on_metrics(m)
                        self.on_log("DATA", format_log_message(m), "data")
                        await asyncio.sleep(self.interval)

                    if not self._stop.is_set():
                        self.on_log("ERROR", "Mat ket noi BLE, thu ket noi lai...", "error")
                        self.on_status("🟡 Mat ket noi, dang thu lai...")
            except asyncio.CancelledError:
                raise
            except Exception as exc:  # loi BLE rat da dang theo OS/adapter, khong the liet ke het
                self.on_log("ERROR", f"Loi BLE: {exc}, thu lai sau {RETRY_DELAY}s", "error")
                await asyncio.sleep(RETRY_DELAY)

    # ---------------- WiFi (TCP) ----------------
    def _run_wifi(self) -> None:
        import socket

        ip = (self.fixed_port or "").strip()
        if not ip:
            self.on_log("ERROR", "Chua nhap dia chi IP cua board (xem tren man hinh board).", "error")
            return

        while not self._stop.is_set():
            self.on_status(f"🟡 Dang ket noi {ip}:{WIFI_TCP_PORT}...")
            try:
                with socket.create_connection((ip, WIFI_TCP_PORT), timeout=5) as sock:
                    sock.sendall(IDENTITY_CMD)
                    sock.settimeout(5)
                    buf = ""
                    while IDENTITY_REPLY not in buf:
                        chunk = sock.recv(256)
                        if not chunk:
                            raise ConnectionError("board dong ket noi giua chung")
                        buf += chunk.decode("ascii", errors="ignore")

                    # Tu day chi GUI, khong doc nua -> bo timeout doc de khong bi ngat oan;
                    # mat ket noi se lo ra ngay o lenh sendall().
                    sock.settimeout(None)
                    self.on_status(f"🟢 Da ket noi WiFi: {ip}:{WIFI_TCP_PORT}")
                    self.on_log("OK", f"Da xac thuc board PLG tai {ip}:{WIFI_TCP_PORT}, "
                                      f"gui du lieu moi {self.interval}s.", "ok")
                    while not self._stop.is_set():
                        m = gather_metrics()
                        sock.sendall(build_payload(m).encode("ascii"))
                        self.on_metrics(m)
                        self.on_log("DATA", format_log_message(m), "data")
                        if self._stop.wait(self.interval):
                            break
            except OSError as exc:
                # gom ca ConnectionRefused/timeout/mang khong toi duoc: board co the chua vao
                # WiFi xong, hoac vua mat song - cu thu lai nhu che do USB van lam.
                self.on_log("ERROR", f"Khong ket noi duoc toi {ip}:{WIFI_TCP_PORT} ({exc}), "
                                     f"thu lai sau {RETRY_DELAY}s", "error")
                self.on_status("🟡 Mat ket noi, dang thu lai...")
                if self._stop.wait(RETRY_DELAY):
                    return


# Bang mau "modern dashboard" dark mode: nen tim-than gan den (thay vi xam VSCode),
# accent xanh-indigo sang lam diem nhan, cac sac do/vang/xanh la tuoi hon de tuong
# phan ro rang tren nen toi. Tach rieng thanh hang so o muc module (thay vi khai
# bao trong run_gui) de de doi mau sau nay ma khong phai lan trong ham dung widget.
BG = "#0F1117"          # Background - tim-than gan den, sau va dju hon xam thuan
PANEL_BG = "#171A24"    # Card - khung nhom (LabelFrame)
FIELD_BG = "#1E212E"    # Tile phu - o nhap/combobox/canvas/card thong so
BORDER = "#2A2E3D"      # Border - vien mong tach bach cac khoi
TEXT = "#E7E9F3"        # Text chinh - trang nga, de doc tren nen toi
SUBTEXT = "#8B90A8"     # Text phu - xam-tim nhat, dung cho nhan/label phu
ACCENT = "#7C9CFF"      # Primary - xanh-indigo sang, mau nhan dien chinh
ACCENT_DIM = "#242A4A"  # nen mo cua Accent (selection, nut o trang thai thuong)
ACCENT_ACTIVE = "#A9BCFF"  # Accent sang hon khi hover/active
DANGER = "#FF6B81"      # Danger - do-hong tuoi
GOOD = "#3DDC97"        # Success - xanh la ngoc
WARN_COLOR = "#FFC65C"  # Warning - vang cam
CYAN = "#5FE3E0"        # Secondary - xanh ngoc, dung cho du lieu/thong tin phu
NET_BLUE = "#7C9CFF"    # bieu do NET dung mau Primary cho dong bo voi accent
LOG_BG = "#12141C"
LOG_BG_ALT = "#171A24"  # nen xen ke (zebra stripe) cho hang chan trong bang Log
LOG_COLORS = {
    "ok": GOOD,
    "error": DANGER,
    "warn": WARN_COLOR,
    "data": CYAN,
    "info": ACCENT,
}
METRIC_ICONS = {
    "CPU": "🧠", "RAM": "💾", "GPU": "🎮", "VRAM": "🗄️",
    "TEMP": "🌡️", "NET": "📶", "BAT": "🔋", "TIME": "🕒",
}

# Khi build bang PyInstaller (--onefile), cac file asset (.ico/.png) duoc giai nen
# vao thu muc tam sys._MEIPASS luc chay, KHONG nam canh file .exe - phai uu tien
# doc tu do neu dang chay ban build, nguoc lai (chay truc tiep monitor.py) thi
# lay theo thu muc chua script nhu binh thuong.
_ASSETS_DIR = __import__("pathlib").Path(getattr(sys, "_MEIPASS", None) or __file__).resolve()
if _ASSETS_DIR.is_file():
    _ASSETS_DIR = _ASSETS_DIR.parent
ICON_PATH = _ASSETS_DIR / "PLG_logoV2.ico"   # icon cua so (taskbar/title bar)
# Logo header: uu tien ban da resize san bang Pillow/LANCZOS (PLG_logoV2_header.png,
# ~96px ngang) - anh net, khong rang cua nhu khi Tk tu "subsample" (chi lay mau nearest-
# neighbor, khong lam min) truc tiep tu file logo goc kich thuoc lon (1370x784).
LOGO_HEADER_PATH = _ASSETS_DIR / "PLG_logoV2_header.png"
LOGO_PATH = LOGO_HEADER_PATH if LOGO_HEADER_PATH.exists() else _ASSETS_DIR / "PLG_logoV2.png"


def _apply_dark_neon_theme(root, ttk) -> None:
    """Tuy bien ttk.Style thanh "modern dashboard" dark mode: bo goc mem hon ve
    mat thi giac (padding rong, vien mong, khong con vien vuong cung nhu ban cu),
    font Segoe UI Variable/Semibold cho tieu de. Dung theme 'clam' lam nen vi day
    la theme duy nhat cho phep doi mau nen/vien cua tung widget (theme mac dinh
    'vista'/'winnative' tren Windows khoa cung mau he thong)."""
    root.configure(bg=BG)
    style = ttk.Style(root)
    style.theme_use("clam")

    style.configure(".", background=BG, foreground=TEXT, fieldbackground=FIELD_BG,
                     bordercolor=BORDER, darkcolor=BG, lightcolor=BG, font=("Segoe UI", 9))
    style.configure("TFrame", background=BG)
    style.configure("TLabel", background=BG, foreground=TEXT)
    style.configure("Sub.TLabel", background=BG, foreground=SUBTEXT, font=("Segoe UI", 9))
    style.configure("Title.TLabel", background=BG, foreground=TEXT, font=("Segoe UI Semibold", 17))
    style.configure("Status.TLabel", background=BG, foreground=TEXT, font=("Segoe UI", 9, "bold"))
    style.configure("SectionTitle.TLabel", background=BG, foreground=SUBTEXT, font=("Segoe UI", 8, "bold"))

    style.configure("TLabelframe", background=PANEL_BG, bordercolor=BORDER, relief="solid", borderwidth=1)
    style.configure("TLabelframe.Label", background=PANEL_BG, foreground=ACCENT, font=("Segoe UI Semibold", 9))
    style.configure("Card.TFrame", background=PANEL_BG)
    style.configure("Card.TLabel", background=PANEL_BG, foreground=TEXT)
    style.configure("CardName.TLabel", background=PANEL_BG, foreground=SUBTEXT, font=("Segoe UI", 8, "bold"))
    style.configure("CardValue.TLabel", background=PANEL_BG, foreground=ACCENT, font=("Consolas", 12, "bold"))
    # Tile: nen sang hon 1 chut so voi Card, dung cho tung o metric rieng le - cho
    # cam giac "dashboard" chuyen nghiep hon la mang chu phang.
    style.configure("Tile.TFrame", background=FIELD_BG)
    style.configure("TileName.TLabel", background=FIELD_BG, foreground=SUBTEXT, font=("Segoe UI", 8, "bold"))
    style.configure("TileValue.TLabel", background=FIELD_BG, foreground=TEXT, font=("Consolas", 14, "bold"))

    style.configure("TButton", background=FIELD_BG, foreground=TEXT, bordercolor=BORDER,
                     focusthickness=0, relief="flat", padding=(16, 8))
    style.map("TButton", background=[("active", ACCENT_DIM), ("disabled", PANEL_BG)],
              foreground=[("disabled", SUBTEXT)])
    style.configure("Accent.TButton", background=ACCENT, foreground="#0B0C12", bordercolor=ACCENT,
                     relief="flat", padding=(16, 8), font=("Segoe UI Semibold", 9))
    style.map("Accent.TButton", background=[("active", ACCENT_ACTIVE), ("disabled", PANEL_BG)],
              foreground=[("disabled", SUBTEXT)])
    style.configure("Danger.TButton", background=PANEL_BG, foreground=DANGER, bordercolor=DANGER,
                     relief="flat", padding=(16, 8), font=("Segoe UI Semibold", 9))
    style.map("Danger.TButton", background=[("active", DANGER), ("disabled", PANEL_BG)],
              foreground=[("active", "#0B0C12"), ("disabled", SUBTEXT)])

    style.configure("TCombobox", fieldbackground=FIELD_BG, background=FIELD_BG, foreground=TEXT,
                     arrowcolor=ACCENT, bordercolor=BORDER, selectbackground=FIELD_BG, selectforeground=TEXT,
                     padding=(8, 4))
    style.map("TCombobox", fieldbackground=[("readonly", FIELD_BG), ("disabled", PANEL_BG)],
              foreground=[("disabled", SUBTEXT)])
    root.option_add("*TCombobox*Listbox.background", FIELD_BG)
    root.option_add("*TCombobox*Listbox.foreground", TEXT)
    root.option_add("*TCombobox*Listbox.selectBackground", ACCENT_DIM)
    root.option_add("*TCombobox*Listbox.selectForeground", ACCENT)

    style.configure("TEntry", fieldbackground=FIELD_BG, foreground=TEXT, bordercolor=BORDER,
                     insertcolor=ACCENT, padding=(8, 4))
    style.map("TEntry", fieldbackground=[("disabled", PANEL_BG)])

    style.configure("Vertical.TScrollbar", background=FIELD_BG, troughcolor=BG, bordercolor=BG,
                     arrowcolor=ACCENT, relief="flat")
    style.map("Vertical.TScrollbar", background=[("active", ACCENT_DIM)])

    style.configure("TSeparator", background=BORDER)


def run_gui() -> int:
    import pathlib  # noqa: F401 (giu import de _ASSETS_DIR o tren hoat dong ro rang)
    import queue
    import tkinter as tk
    from tkinter import ttk

    events: "queue.Queue" = queue.Queue()
    worker: ConnectionWorker | None = None

    root = tk.Tk()
    root.title("PLG PC Task Monitor")
    root.geometry("720x660")
    root.minsize(640, 540)
    _apply_dark_neon_theme(root, ttk)

    if ICON_PATH.exists():
        try:
            root.iconbitmap(default=str(ICON_PATH))
        except Exception:
            pass

    # Thanh accent mong o mep tren cung, dac trung cho phong cach "modern dashboard"
    # (tuong tu title bar co gradient/accent cua nhieu app hien dai).
    tk.Frame(root, bg=ACCENT, height=3).pack(fill="x", side="top")

    header = ttk.Frame(root)
    header.pack(fill="x", padx=18, pady=(16, 0))
    logo_img = None
    if LOGO_PATH.exists():
        try:
            logo_img = tk.PhotoImage(file=str(LOGO_PATH))
            # Chi "subsample" (giam mau nearest-neighbor cua Tk, de bi rang/mo) khi
            # dang phai dung anh logo GOC kich thuoc lon (khong tim thay ban da
            # resize san PLG_logoV2_header.png). Ban header da duoc resize truoc
            # bang Pillow/LANCZOS nen giu nguyen, hien thi net hon nhieu.
            if LOGO_PATH != LOGO_HEADER_PATH and logo_img.width() > 48:
                factor = max(1, logo_img.width() // 48)
                logo_img = logo_img.subsample(factor, factor)
            logo_tile = tk.Frame(header, bg=FIELD_BG, highlightbackground=BORDER, highlightthickness=1)
            logo_tile.pack(side="left", padx=(0, 14))
            tk.Label(logo_tile, image=logo_img, bg=FIELD_BG).pack(padx=8, pady=8)
        except Exception:
            logo_img = None
    title_box = ttk.Frame(header)
    title_box.pack(side="left")
    ttk.Label(title_box, text="PLG PC Task Monitor", style="Title.TLabel").pack(anchor="w")
    ttk.Label(title_box, text="Serial link toi board PLG_TFT_LCD_TASKMANAGER", style="Sub.TLabel").pack(anchor="w", pady=(2, 0))
    root._logo_img_ref = logo_img  # giu tham chieu, tranh bi garbage-collect mat anh

    conn_frame = ttk.LabelFrame(root, text="  KET NOI  ")
    conn_frame.pack(fill="x", padx=18, pady=(16, 8))

    saved_settings = load_settings()
    mode_var = tk.StringVar(value=saved_settings.get("mode", "auto"))
    baud_var = tk.StringVar(value=str(saved_settings.get("baud", "115200")))
    interval_var = tk.StringVar(value=str(saved_settings.get("interval", "0.8")))
    port_var = tk.StringVar(value=saved_settings.get("port", ""))
    status_var = tk.StringVar(value="🔴 Chua ket noi")

    # Nhan hien thi cho tung che do (de hieu hon "auto"/"manual"/"bluetooth"/"wifi" tran trui);
    # 2 bang tra nguoc nhau de doi qua lai giua nhan tren man hinh va gia tri luu vao file.
    MODE_LABELS = {
        "auto": "USB - tu do cong",
        "manual": "USB - chon cong",
        "bluetooth": "Bluetooth (BLE)",
        "wifi": "WiFi (TCP)",
    }
    MODE_FROM_LABEL = {v: k for k, v in MODE_LABELS.items()}

    mode_label_var = tk.StringVar(value=MODE_LABELS.get(mode_var.get(), MODE_LABELS["auto"]))

    ttk.Label(conn_frame, text="Che do:").grid(row=0, column=0, sticky="w", padx=6, pady=4)
    mode_combo = ttk.Combobox(conn_frame, textvariable=mode_label_var, state="readonly",
                               values=list(MODE_LABELS.values()), width=18)
    mode_combo.grid(row=0, column=1, sticky="w", padx=6, pady=4)

    # O nay dung chung cho 2 muc dich tuy che do: chon cong COM (USB) hoac go dia chi IP (WiFi).
    # Gop lam 1 thay vi them o rieng de khung ket noi khong phinh to voi 1 o luon bi khoa.
    target_label = ttk.Label(conn_frame, text="Cong:")
    target_label.grid(row=0, column=2, sticky="w", padx=6, pady=4)
    port_combo = ttk.Combobox(conn_frame, textvariable=port_var, state="disabled", width=14)
    port_combo.grid(row=0, column=3, sticky="w", padx=6, pady=4)

    def refresh_ports() -> None:
        ports = [p.device for p in serial.tools.list_ports.comports()]
        port_combo["values"] = ports
        if ports and not port_var.get():
            port_var.set(ports[0])

    refresh_btn = ttk.Button(conn_frame, text="Lam moi", command=refresh_ports)
    refresh_btn.grid(row=0, column=4, padx=6, pady=4)

    ttk.Label(conn_frame, text="Baudrate:").grid(row=1, column=0, sticky="w", padx=6, pady=4)
    baud_combo = ttk.Combobox(conn_frame, textvariable=baud_var, width=10,
                               values=["9600", "19200", "38400", "57600", "115200", "230400"])
    baud_combo.grid(row=1, column=1, sticky="w", padx=6, pady=4)

    ttk.Label(conn_frame, text="Khoang gui (s):").grid(row=1, column=2, sticky="w", padx=6, pady=4)
    interval_entry = ttk.Entry(conn_frame, textvariable=interval_var, width=10)
    interval_entry.grid(row=1, column=3, sticky="w", padx=6, pady=4)

    def on_mode_change(*_a) -> None:
        mode = MODE_FROM_LABEL.get(mode_label_var.get(), "auto")
        mode_var.set(mode)

        if mode == "manual":
            target_label.configure(text="Cong:")
            port_combo.configure(state="readonly", values=[])
            refresh_ports()
            refresh_btn.configure(state="normal")
        elif mode == "wifi":
            # Combobox o che do "normal" = go tay duoc; danh sach goi y bo trong vi IP cua
            # board do nguoi dung doc tren man hinh, khong the do ra tu may tinh.
            target_label.configure(text="IP board:")
            port_combo.configure(state="normal", values=[])
            port_var.set(saved_settings.get("wifi_ip", ""))
            refresh_btn.configure(state="disabled")
        else:  # auto (tu do cong) hoac bluetooth (tu do theo ten thiet bi) - khong can nhap gi
            target_label.configure(text="Cong:")
            port_combo.configure(state="disabled")
            refresh_btn.configure(state="disabled" if mode == "bluetooth" else "normal")

        # Baudrate chi co nghia voi USB Serial; BLE/WiFi khong dung toi.
        baud_combo.configure(state="disabled" if mode in ("bluetooth", "wifi") else "normal")

    mode_combo.bind("<<ComboboxSelected>>", on_mode_change)
    on_mode_change()

    btn_frame = ttk.Frame(root)
    btn_frame.pack(fill="x", padx=18, pady=(2, 8))
    connect_btn = ttk.Button(btn_frame, text="▸  CONNECT", style="Accent.TButton")
    disconnect_btn = ttk.Button(btn_frame, text="■  DISCONNECT", style="Danger.TButton", state="disabled")
    connect_btn.pack(side="left", padx=(0, 10))
    disconnect_btn.pack(side="left")
    status_dot = tk.Canvas(btn_frame, width=12, height=12, bg=BG, highlightthickness=0)
    status_dot.pack(side="left", padx=(18, 8))
    status_dot_id = status_dot.create_oval(1, 1, 11, 11, fill=DANGER, outline="")
    ttk.Label(btn_frame, textvariable=status_var, style="Status.TLabel").pack(side="left")

    # Bang hien thi 8 Card thong so, luoi 4 cot x 2 hang dung theo dac ta: CPU/RAM/GPU/
    # VRAM o hang tren, TEMP/NET/BAT/TIME o hang duoi. Moi card la 1 "Card phu"
    # (FIELD_BG) co vien mong (BORDER) tren nen Card (PANEL_BG) cua ca khung.
    metrics_frame = ttk.LabelFrame(root, text="  THONG SO DANG GUI  ")
    metrics_frame.pack(fill="x", padx=18, pady=8)
    metrics_inner = ttk.Frame(metrics_frame, style="Card.TFrame")
    metrics_inner.pack(fill="x", padx=12, pady=12)

    SPARK_KEYS = ("cpu", "gpu_mem")
    HIST_LEN = 40
    history = {k: [] for k in SPARK_KEYS}

    metric_vars: dict[str, tk.StringVar] = {"TIME": tk.StringVar(value="--:--:--")}
    metric_canvases: dict[str, tk.Canvas] = {}
    # (nhan, kind) - kind: "spark" = bieu do duong, "bar" = thanh ngang, "gauge" =
    # nua vong tron, "netbars" = mini bar chart NET, "battery" = icon pin, "text".
    cards = [("CPU", "spark"), ("RAM", "bar"), ("GPU", "bar"), ("VRAM", "spark"),
             ("TEMP", "gauge"), ("NET", "netbars"), ("BAT", "battery"), ("TIME", "text")]
    for i, (label, kind) in enumerate(cards):
        tile = tk.Frame(metrics_inner, bg=FIELD_BG, highlightbackground=BORDER, highlightthickness=1)
        tile.grid(row=i // 4, column=i % 4, sticky="nsew", padx=6, pady=6)
        cell = ttk.Frame(tile, style="Tile.TFrame")
        cell.pack(fill="both", expand=True, padx=14, pady=12)
        name_row = ttk.Frame(cell, style="Tile.TFrame")
        name_row.pack(fill="x", anchor="w")
        ttk.Label(name_row, text=METRIC_ICONS.get(label, ""), background=FIELD_BG,
                  font=("Segoe UI Emoji", 10)).pack(side="left", padx=(0, 5))
        ttk.Label(name_row, text=label, style="TileName.TLabel").pack(side="left")
        if kind == "text":
            ttk.Label(cell, textvariable=metric_vars["TIME"], style="TileValue.TLabel",
                      font=("Consolas", 17, "bold")).pack(anchor="w", pady=(8, 0))
        else:
            var = tk.StringVar(value="--")
            metric_vars[label] = var
            ttk.Label(cell, textvariable=var, style="TileValue.TLabel").pack(anchor="w", pady=(6, 0))
            canvas_h = {"spark": 28, "bar": 12, "gauge": 30, "netbars": 24, "battery": 22}[kind]
            canvas = tk.Canvas(cell, height=canvas_h, bg=FIELD_BG, highlightthickness=0)
            canvas.pack(fill="x", pady=(8, 0))
            metric_canvases[label] = canvas
    for col in range(4):
        metrics_inner.columnconfigure(col, weight=1)

    def _color_for_percent(v: float) -> str:
        """Xanh duong(Primary) binh thuong -> vang(Warning) -> do(Danger) khi gia tri % tang cao."""
        if v >= 85:
            return DANGER
        if v >= 65:
            return WARN_COLOR
        return ACCENT

    def draw_sparkline(canvas: tk.Canvas, values: list[int], color: str) -> None:
        canvas.delete("all")
        canvas.update_idletasks()
        w, h = canvas.winfo_width() or 200, int(canvas["height"])
        pts = [v for v in values if v is not None and v >= 0]
        if len(pts) < 2:
            return
        lo, hi = 0, 100
        span = max(1, hi - lo)
        n = len(values)
        coords = []
        for i, v in enumerate(values):
            x = 2 + i * (w - 4) / max(1, n - 1)
            vv = v if (v is not None and v >= 0) else 0
            y = h - 2 - (vv - lo) / span * (h - 4)
            coords.extend([x, y])
        area = coords + [w - 2, h - 1, 2, h - 1]
        canvas.create_polygon(*area, fill=ACCENT_DIM, outline="")
        canvas.create_line(*coords, fill=color, width=2, smooth=True)

    def draw_bar_h(canvas: tk.Canvas, value: int, color: str) -> None:
        canvas.delete("all")
        canvas.update_idletasks()
        w, h = canvas.winfo_width() or 200, int(canvas["height"])
        canvas.create_rectangle(0, 0, w, h, fill=BG, outline="")
        if value is not None and value >= 0:
            frac = max(0, min(100, value)) / 100
            canvas.create_rectangle(0, 0, max(2, w * frac), h, fill=color, outline="")

    def draw_gauge(canvas: tk.Canvas, value: int) -> None:
        canvas.delete("all")
        canvas.update_idletasks()
        w, h = canvas.winfo_width() or 200, int(canvas["height"])
        cx, cy, r = w / 2, h - 1, min(w / 2 - 3, h * 2 - 4)
        canvas.create_arc(cx - r, cy - r, cx + r, cy + r, start=0, extent=180,
                           style="arc", outline=BORDER, width=5)
        if value is not None and value >= 0:
            frac = max(0, min(100, value)) / 100
            canvas.create_arc(cx - r, cy - r, cx + r, cy + r, start=180, extent=-180 * frac,
                               style="arc", outline=_color_for_percent(value), width=5)

    def draw_net_bars(canvas: tk.Canvas, mbps: int) -> None:
        """Mini bar chart 5 cot cho NET, tang dan theo nguong Mbps (▂▃▅▇ style don gian)."""
        canvas.delete("all")
        canvas.update_idletasks()
        w, h = canvas.winfo_width() or 200, int(canvas["height"])
        thresholds = (1, 5, 20, 50)
        active = 0 if mbps is None or mbps < 0 else sum(1 for t in thresholds if mbps >= t) + 1
        active = min(active, 5)
        n = 5
        gap = 4
        bar_w = max(4, (w - gap * (n - 1)) / n)
        for i in range(n):
            bh = h * (0.3 + 0.7 * (i + 1) / n)
            x0 = i * (bar_w + gap)
            y1 = h
            y0 = y1 - bh
            color = ACCENT if i < active else BORDER
            canvas.create_rectangle(x0, y0, x0 + bar_w, y1, fill=color, outline="")

    def draw_battery(canvas: tk.Canvas, pct: int) -> None:
        canvas.delete("all")
        canvas.update_idletasks()
        w, h = canvas.winfo_width() or 200, int(canvas["height"])
        body_w, body_h = min(70, w - 10), h - 4
        x0, y0 = 0, (h - body_h) / 2
        x1, y1 = x0 + body_w, y0 + body_h
        canvas.create_rectangle(x0, y0, x1, y1, outline=BORDER, width=2)
        canvas.create_rectangle(x1, y0 + body_h * 0.25, x1 + 4, y1 - body_h * 0.25, fill=BORDER, outline="")
        if pct is not None and pct >= 0:
            frac = max(0, min(100, pct)) / 100
            color = GOOD if pct > 30 else (WARN_COLOR if pct > 15 else DANGER)
            fw = max(0, (body_w - 4) * frac)
            canvas.create_rectangle(x0 + 2, y0 + 2, x0 + 2 + fw, y1 - 2, fill=color, outline="")

    def update_metric_labels(m: dict) -> None:
        for k in SPARK_KEYS:
            history[k].append(m[k])
            if len(history[k]) > HIST_LEN:
                history[k].pop(0)

        metric_vars["CPU"].set(f"{m['cpu']}%" if m["cpu"] >= 0 else "N/A")
        metric_vars["RAM"].set(f"{m['ram']}%" if m["ram"] >= 0 else "N/A")
        metric_vars["GPU"].set(f"{m['gpu']}%" if m["gpu"] >= 0 else "N/A")
        metric_vars["VRAM"].set(f"{m['gpu_mem']}%" if m["gpu_mem"] >= 0 else "N/A")
        metric_vars["TEMP"].set(f"{m['temp']}C" if m["temp"] >= 0 else "N/A")
        metric_vars["NET"].set(f"{m['wifi']}Mbps" if m["wifi"] >= 0 else "N/A")
        metric_vars["BAT"].set(f"{m['bat']}%" if m["bat"] >= 0 else "N/A")
        metric_vars["TIME"].set(m["time"])

        draw_sparkline(metric_canvases["CPU"], history["cpu"], _color_for_percent(m["cpu"] if m["cpu"] >= 0 else 0))
        draw_sparkline(metric_canvases["VRAM"], history["gpu_mem"], ACCENT)
        draw_bar_h(metric_canvases["RAM"], m["ram"], _color_for_percent(m["ram"] if m["ram"] >= 0 else 0))
        draw_bar_h(metric_canvases["GPU"], m["gpu"], ACCENT)
        draw_gauge(metric_canvases["TEMP"], m["temp"])
        draw_net_bars(metric_canvases["NET"], m["wifi"])
        draw_battery(metric_canvases["BAT"], m["bat"])

    # Log dang bang 3 cot (Time / Status / Message), giong may giam sat chuyen dung
    # hon la 1 khoi text tho - de loc/doc theo tung loai su kien bang mat.
    log_frame = ttk.LabelFrame(root, text="  LOG  ")
    log_frame.pack(fill="both", expand=True, padx=18, pady=(0, 16))
    log_inner = ttk.Frame(log_frame, style="Card.TFrame")
    log_inner.pack(fill="both", expand=True, padx=4, pady=4)
    log_tree = ttk.Treeview(log_inner, columns=("time", "status", "message"), show="headings", height=10)
    log_tree.heading("time", text="Time")
    log_tree.heading("status", text="Status")
    log_tree.heading("message", text="Message")
    log_tree.column("time", width=90, anchor="w", stretch=False)
    log_tree.column("status", width=90, anchor="w", stretch=False)
    log_tree.column("message", width=420, anchor="w", stretch=True)
    log_tree.pack(fill="both", expand=True, side="left")
    log_scroll = ttk.Scrollbar(log_inner, command=log_tree.yview)
    log_scroll.pack(fill="y", side="right")
    log_tree.configure(yscrollcommand=log_scroll.set)

    style = ttk.Style(root)
    style.configure("Treeview", background=LOG_BG, fieldbackground=LOG_BG, foreground=TEXT,
                     rowheight=24, borderwidth=0, font=("Consolas", 9))
    style.configure("Treeview.Heading", background=PANEL_BG, foreground=ACCENT,
                     font=("Segoe UI Semibold", 9), relief="flat", padding=(6, 6))
    style.map("Treeview", background=[("selected", ACCENT_DIM)], foreground=[("selected", ACCENT)])
    log_tree.tag_configure("even", background=LOG_BG)
    log_tree.tag_configure("odd", background=LOG_BG_ALT)
    for tag, color in LOG_COLORS.items():
        log_tree.tag_configure(tag, foreground=color)

    MAX_LOG_ROWS = 500  # gioi han so dong log, tranh Treeview phinh to sau nhieu gio chay
    _log_row_count = {"n": 0}

    def append_log(status: str, message: str, tag: str = "info") -> None:
        # Tag mau theo loai su kien (ok/error/warn/...) chi anh huong mau CHU; tag
        # zebra (even/odd) o dong sau quyet dinh mau NEN xen ke - Treeview cho phep
        # ap nhieu tag cung luc, tag dung sau trong tuple ghi de thuoc tinh trung.
        zebra = "even" if _log_row_count["n"] % 2 == 0 else "odd"
        _log_row_count["n"] += 1
        log_tree.insert("", "end", values=(time.strftime("%H:%M:%S"), status, message), tags=(tag, zebra))
        children = log_tree.get_children()
        if len(children) > MAX_LOG_ROWS:
            log_tree.delete(children[0])
        log_tree.yview_moveto(1.0)

    def on_log(status: str, message: str, tag: str) -> None:
        events.put(("log", (status, message), tag))

    def on_metrics(m: dict) -> None:
        events.put(("metrics", m, None))

    def on_status(text: str) -> None:
        events.put(("status", text, None))

    def set_status_dot(color: str) -> None:
        status_dot.itemconfigure(status_dot_id, fill=color)

    def poll_events() -> None:
        try:
            while True:
                kind, payload, tag = events.get_nowait()
                if kind == "log":
                    status, message = payload
                    append_log(status, message, tag)
                elif kind == "metrics":
                    update_metric_labels(payload)
                elif kind == "status":
                    status_var.set(payload)
                    if payload.startswith("🟢"):
                        set_status_dot(GOOD)
                    elif payload.startswith("🔴"):
                        set_status_dot(DANGER)
                    else:
                        set_status_dot(WARN_COLOR)
        except queue.Empty:
            pass
        root.after(100, poll_events)

    def do_connect() -> None:
        nonlocal worker
        try:
            baud = int(baud_var.get())
        except ValueError:
            append_log("ERROR", "Baudrate khong hop le.", "error")
            return
        try:
            interval = float(interval_var.get())
        except ValueError:
            append_log("ERROR", "Khoang gui khong hop le.", "error")
            return
        mode = mode_var.get()
        # O nhap dia chi mang y nghia khac nhau tuy che do (cong COM hay IP), xem on_mode_change.
        target = port_var.get().strip() if mode in ("manual", "wifi") else None

        if mode == "manual" and not target:
            append_log("ERROR", "Vui long chon cong o che do USB - chon cong.", "error")
            return
        if mode == "wifi" and not target:
            append_log("ERROR", "Vui long nhap dia chi IP dang hien tren man hinh board.", "error")
            return

        settings = {"mode": mode, "baud": baud, "interval": interval}
        # Nho rieng cong COM va IP: doi qua lai giua 2 che do khong lam mat gia tri con lai.
        settings["port"] = target if mode == "manual" else saved_settings.get("port", "")
        settings["wifi_ip"] = target if mode == "wifi" else saved_settings.get("wifi_ip", "")
        saved_settings.update(settings)
        save_settings(settings)
        port = target
        worker = ConnectionWorker(mode, port, baud, interval, on_log, on_metrics, on_status)
        worker.start()
        connect_btn.configure(state="disabled")
        disconnect_btn.configure(state="normal")
        mode_combo.configure(state="disabled")
        port_combo.configure(state="disabled")
        baud_combo.configure(state="disabled")
        interval_entry.configure(state="disabled")
        refresh_btn.configure(state="disabled") # dang chay thi do lai cong cung vo nghia
        status_var.set("🟡 Dang ket noi...")
        set_status_dot(WARN_COLOR)

    def do_disconnect() -> None:
        nonlocal worker
        if worker is not None:
            worker.stop()
            worker = None
        connect_btn.configure(state="normal")
        disconnect_btn.configure(state="disabled")
        mode_combo.configure(state="readonly")
        interval_entry.configure(state="normal")
        # on_mode_change() tu quyet dinh o nao duoc bat lai theo che do dang chon (vd BLE thi
        # khong bat lai o cong/baudrate) - phai goi SAU cung, dung bat tay baud_combo o day.
        on_mode_change()
        status_var.set("🔴 Chua ket noi")
        set_status_dot(DANGER)

    connect_btn.configure(command=do_connect)
    disconnect_btn.configure(command=do_disconnect)

    def on_close() -> None:
        # Nguoi dung muon dong app la tu dong disconnect: cho worker thuc su dong
        # xong cong serial (toi da 1.5s) truoc khi huy cua so, thay vi chi bao
        # dung roi thoat ngay - tranh bo lai 1 handle COM dang mo phia sau.
        if worker is not None:
            worker.stop(wait=1.5)
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    refresh_ports()
    root.after(100, poll_events)
    root.mainloop()
    return 0


# ---------------------------------------------------------------------------
# Che do BLE (board ESP32-S3)
# ---------------------------------------------------------------------------
# Board ESP32 phat mot Nordic UART Service (NUS) - "cong COM ao tren BLE". Ta ghi
# du lieu vao characteristic RX cua no va nhan tra loi qua notify tren TX, dung
# het cac ham gather_metrics/build_payload san co: giao thuc du lieu khong doi,
# chi doi duong truyen.
BLE_DEVICE_NAME = "PLG_TFT_LCD"
NUS_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # PC ghi vao day
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # board notify ve day

# Kich thuoc goi ghi an toan khi khong biet MTU da thoa thuan duoc bao nhieu: MTU
# BLE toi thieu la 23 byte, tru 3 byte header ATT con 20. Payload cua ta (~90 ky
# tu) luon dai hon nen phai cat nho; firmware gom lai theo ky tu '\n' nen viec cat
# o dau cung khong anh huong.
BLE_MIN_CHUNK = 20


async def _ble_send_line(client, text: str) -> None:
    """Ghi 1 dong xuong board, tu cat nho theo MTU thuc te neu biet."""
    chunk_size = BLE_MIN_CHUNK
    mtu = getattr(client, "mtu_size", None)
    if isinstance(mtu, int) and mtu > 23:
        chunk_size = mtu - 3

    data = text.encode("ascii")
    for i in range(0, len(data), chunk_size):
        # response=False (write without response): nhanh hon nhieu cho luong du lieu
        # dinh ky, khong can cho board xac nhan tung goi.
        await client.write_gatt_char(NUS_RX_CHAR_UUID, data[i:i + chunk_size], response=False)


async def _ble_session(interval: float) -> None:
    """Do tim board, ket noi, xac thuc rui gui du lieu den khi mat ket noi."""
    from bleak import BleakClient, BleakScanner

    print(_c(f"Dang do tim thiet bi BLE \"{BLE_DEVICE_NAME}\"...", Fore.YELLOW))
    device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME, timeout=15.0)
    if device is None:
        print(_c(f"Khong thay \"{BLE_DEVICE_NAME}\". Kiem tra board da chon SETTING > CONNECTION > "
                 f"BLUETOOTH chua, thu lai sau {RETRY_DELAY}s", Fore.YELLOW))
        await asyncio.sleep(RETRY_DELAY)
        return

    async with BleakClient(device) as client:
        # Bat tay xac thuc giong het che do USB: gui PLG_ID?, cho dung cau tra loi.
        # Khac USB o cho da loc theo ten thiet bi BLE nen kha nang nham thiet bi rat
        # thap - van kiem tra de bao loi som neu firmware tren board da cu/khac.
        reply_seen = asyncio.Event()
        buf = ""

        def on_notify(_sender, data: bytearray) -> None:
            nonlocal buf
            buf += data.decode("ascii", errors="ignore")
            if IDENTITY_REPLY in buf:
                reply_seen.set()

        await client.start_notify(NUS_TX_CHAR_UUID, on_notify)
        await _ble_send_line(client, IDENTITY_CMD.decode("ascii"))
        try:
            await asyncio.wait_for(reply_seen.wait(), timeout=5.0)
        except asyncio.TimeoutError:
            print(_c(f"Thiet bi BLE khong tra loi xac thuc \"{IDENTITY_REPLY}\", "
                     f"co the dang chay firmware khac. Ngat ket noi.", Fore.RED))
            return

        print(_c(f"Da xac thuc board PLG qua BLE ({device.address}), gui du lieu moi {interval}s. "
                 f"Ctrl+C de dung.", Fore.GREEN))
        while client.is_connected:
            m = gather_metrics()
            await _ble_send_line(client, build_payload(m))
            print(format_console_line(m))
            await asyncio.sleep(interval)

        print(_c("Mat ket noi BLE, thu ket noi lai...", Fore.RED))


async def _ble_main_loop(interval: float) -> None:
    """Vong ngoai: tu do tim/ket noi lai khi chua thay board hoac bi ngat giua chung,
    giong tinh than vong lap cua che do USB."""
    while True:
        try:
            await _ble_session(interval)
        except asyncio.CancelledError:
            raise
        except Exception as exc:  # loi BLE rat da dang theo OS/adapter, khong the liet ke het
            print(_c(f"Loi BLE: {exc}, thu lai sau {RETRY_DELAY}s", Fore.RED))
            await asyncio.sleep(RETRY_DELAY)


def run_ble(interval: float) -> int:
    try:
        import bleak  # noqa: F401  chi de bao loi som, ro rang neu chua cai
    except ImportError:
        print(_c("Che do --ble can goi 'bleak': chay 'pip install bleak' (hoac "
                 "'pip install -r requirements.txt') rui thu lai.", Fore.RED))
        return 1

    psutil.cpu_percent(interval=None)  # lan goi dau tra ve 0.0, bo qua de lay mau chuan
    time.sleep(0.2)
    try:
        asyncio.run(_ble_main_loop(interval))
    except KeyboardInterrupt:
        print("\nDa dung.")
    return 0


# ---------------------------------------------------------------------------
# Che do WiFi (board ESP32-S3)
# ---------------------------------------------------------------------------
# Board dong vai tro TCP server; ta la client go toi dia chi IP hien tren man hinh
# cua no (SETTING > CONNECTION > WIFI). Dung lai y nguyen gather_metrics/build_payload
# va ca bat tay PLG_ID? - chi doi duong truyen.
WIFI_TCP_PORT = 5005


def _wifi_session(ip: str, port: int, interval: float) -> None:
    """Ket noi toi board, xac thuc rui gui du lieu den khi mat ket noi."""
    import socket

    print(_c(f"Dang ket noi toi board tai {ip}:{port}...", Fore.YELLOW))
    with socket.create_connection((ip, port), timeout=5) as sock:
        # Bat tay xac thuc giong het che do USB/BLE: tranh go nham vao mot dich vu
        # khac dang mo cung cong tren may khac trong mang.
        sock.sendall(IDENTITY_CMD)
        sock.settimeout(5)
        buf = ""
        try:
            while IDENTITY_REPLY not in buf:
                chunk = sock.recv(256)
                if not chunk:
                    raise ConnectionError("board dong ket noi giua chung")
                buf += chunk.decode("ascii", errors="ignore")
        except socket.timeout:
            print(_c(f"Dia chi {ip}:{port} khong tra loi xac thuc \"{IDENTITY_REPLY}\", "
                     f"co the khong phai board PLG. Ngat ket noi.", Fore.RED))
            return

        print(_c(f"Da xac thuc board PLG tai {ip}:{port}, gui du lieu moi {interval}s. Ctrl+C de dung.",
                 Fore.GREEN))
        # Tu day tro di chi GUI, khong cho doc nua -> bo timeout doc de khong bi
        # ngat oan; mat ket noi se lo ra ngay o lenh sendall().
        sock.settimeout(None)
        while True:
            m = gather_metrics()
            sock.sendall(build_payload(m).encode("ascii"))
            print(format_console_line(m))
            time.sleep(interval)


def run_wifi(ip: str, port: int, interval: float) -> int:
    psutil.cpu_percent(interval=None)  # lan goi dau tra ve 0.0, bo qua de lay mau chuan
    time.sleep(0.2)
    try:
        while True:
            try:
                _wifi_session(ip, port, interval)
            except KeyboardInterrupt:
                raise
            except OSError as exc:
                # gom ca ConnectionRefused/timeout/mang khong toi duoc: board co the chua
                # vao WiFi xong, hoac vua mat song - cu thu lai nhu che do USB van lam.
                print(_c(f"Khong ket noi duoc toi {ip}:{port} ({exc}), thu lai sau {RETRY_DELAY}s",
                         Fore.RED))
            time.sleep(RETRY_DELAY)
    except KeyboardInterrupt:
        print("\nDa dung.")
    return 0


def console_main() -> int:
    parser = argparse.ArgumentParser(description="Gui thong so CPU/RAM/GPU/WIFI xuong board PLG qua Serial, BLE hoac WiFi")
    parser.add_argument("--port", help="Ep dung cong serial nay (vd COM5, /dev/ttyACM0), bo qua buoc tu do tim + xac thuc.")
    parser.add_argument("--baud", type=int, default=115200, help="Toc do baud (mac dinh 115200)")
    parser.add_argument("--interval", type=float, default=0.8, help="Khoang thoi gian gui du lieu, giay (mac dinh 0.8)")
    parser.add_argument("--list", action="store_true", help="Liet ke cac cong serial va thoat")
    parser.add_argument("--gui", action="store_true", help="Mo giao dien GUI (chon che do/cong/baud, nut Connect/Disconnect)")
    parser.add_argument("--ble", action="store_true",
                        help=f"Gui qua Bluetooth LE thay vi USB (board ESP32-S3 da chon SETTING > "
                             f"CONNECTION > BLUETOOTH, quang ba ten \"{BLE_DEVICE_NAME}\")")
    parser.add_argument("--wifi", metavar="IP",
                        help="Gui qua WiFi toi dia chi IP hien tren man hinh board "
                             "(SETTING > CONNECTION > WIFI)")
    parser.add_argument("--wifi-port", type=int, default=WIFI_TCP_PORT,
                        help=f"Cong TCP cua board o che do WiFi (mac dinh {WIFI_TCP_PORT})")
    args = parser.parse_args()

    if args.gui:
        return run_gui()

    if args.ble and args.wifi:
        print(_c("Chi chon MOT duong truyen: --ble hoac --wifi, khong dung ca hai.", Fore.RED))
        return 2

    if args.ble:
        return run_ble(args.interval)

    if args.wifi:
        return run_wifi(args.wifi, args.wifi_port, args.interval)

    if args.list:
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            print("Khong tim thay cong serial nao.")
        for p in ports:
            print(f"{p.device}\t{p.description}\tVID:PID={p.vid}:{p.pid}")
        return 0

    background = not sys.stdin.isatty()
    fixed_port = args.port

    # lan goi dau cpu_percent tra ve 0.0, bo qua de lay mau chuan
    psutil.cpu_percent(interval=None)
    time.sleep(0.2)

    # Vong lap ngoai: tu dong do/xac thuc lai cong khi chua cam dung board (hoac chua
    # cam gi), va tu ket noi lai neu bi rut day/mat ket noi giua chung - khong con can
    # nguoi dung tu chon cong; quan trong khi chay nen (Startup) vi nguoi dung co the
    # bat may truoc, cam Pico vao sau (vd sau 30 phut).
    #
    # Luu y: identify_device()/find_pico_port() tra ve LUON doi tuong Serial dang mo
    # (khong dong roi mo lai cong ngay sau do) - tren Windows mo lai qua nhanh mot
    # cong USB CDC vua duoc giai phong (nhat la ngay sau khi cam lai thiet bi) hay bi
    # loi "Access is denied" thoang qua, day chinh la nguyen nhan lam lan ket noi lai
    # (sau khi rut/cam thiet bi) bi that bai du board da san sang.
    try:
        while True:
            if fixed_port:
                ser = identify_device(fixed_port, args.baud)
                if ser is None:
                    try:
                        ser = serial.Serial(fixed_port, args.baud, timeout=1)
                        print(_c(f"Canh bao: cong {fixed_port} khong tra loi xac thuc \"{IDENTITY_REPLY}\", "
                                 f"co the khong phai board PLG. Van tiep tuc vi da chi dinh --port thu cong.",
                                 Fore.YELLOW))
                    except serial.SerialException as exc:
                        if not background:
                            print(_c(f"Loi mo cong {fixed_port}: {exc}, thu lai sau {RETRY_DELAY}s", Fore.RED))
                        time.sleep(RETRY_DELAY)
                        continue
                port = fixed_port
            else:
                ser = find_pico_port(args.baud)
                if ser is None:
                    if not background:
                        print(_c("Khong tim/xac thuc duoc board PLG, dang cho... (cam thiet bi vao)", Fore.YELLOW))
                    time.sleep(RETRY_DELAY)
                    continue
                port = ser.port

            print(_c(f"Da xac thuc board PLG tren {port} @ {args.baud} baud, gui du lieu moi {args.interval}s. Ctrl+C de dung.",
                     Fore.GREEN))
            try:
                while True:
                    m = gather_metrics()
                    payload = build_payload(m)
                    ser.write(payload.encode("ascii"))
                    print(format_console_line(m))
                    time.sleep(args.interval)
            except serial.SerialException as exc:
                print(_c(f"Mat ket noi serial: {exc}, thu ket noi lai...", Fore.RED))
                safe_close(ser)
                time.sleep(RETRY_DELAY)
                continue
            except KeyboardInterrupt:
                safe_close(ser)
                raise
    except KeyboardInterrupt:
        print("\nDa dung.")
        return 0


def main() -> int:
    # Chay khong doi so (vd nhan dup vao monitor.exe) -> mo GUI, mac dinh Auto +
    # 115200 baud giong het hanh vi console cu, chi khac la co nut Connect/Disconnect
    # va bang hien thi thong so thay vi phai mo terminal. Khi co doi so dong lenh
    # (--port, --list, --gui, chay tu Startup...) thi giu nguyen console_main nhu cu
    # de khong pha hanh vi chay nen/script hien co.
    if len(sys.argv) == 1:
        return run_gui()
    return console_main()


if __name__ == "__main__":
    raise SystemExit(main())
