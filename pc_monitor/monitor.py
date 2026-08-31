#!/usr/bin/env python3
"""
PLG PC Task Monitor
--------------------
Doc thong so CPU / RAM / GPU / WIFI tren may tinh va gui qua cong Serial (USB)
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

Dinh dang du lieu gui xuong board, moi dong ket thuc bang '\n':
    CPU:<int>;RAM:<int>;GPU:<int>;WIFI:<int>;TIME:<HH:MM:SS>;DATE:<DD/MM/YYYY>;BAT:<int>

Trong do gia tri phan tram la 0-100. GPU/WIFI/BAT se la -1 neu khong doc duoc
(khong co GPU NVIDIA, khong lay duoc cuong do wifi, hoac may khong co pin nhu
PC ban). TIME/DATE la gio va ngay hien tai cua may tinh, dung de hien thi dong
ho tren man hinh thiet bi. BAT la pin cua laptop, hien thi o icon pin ben trai.
"""

import argparse
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
USE_COLOR = sys.stdout.isatty()
if USE_COLOR:
    colorama_init(autoreset=True)


def _c(text: str, color: str) -> str:
    """Boc mau ANSI quanh text neu dang in ra console that, giu nguyen text neu
    khong (vd khi bi redirect ra file) de khong lam ban log bang ma escape."""
    return f"{color}{text}{Style.RESET_ALL}" if USE_COLOR else text


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
        _fmt_metric("WIFI", m["wifi"], warn=40, danger=20, invert=True),
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


def get_wifi_percent() -> int:
    """Cuong do wifi hien tai theo %. Ho tro Windows (netsh); tra -1 neu khong lay duoc."""
    if sys.platform.startswith("win"):
        try:
            import subprocess

            out = subprocess.check_output(
                ["netsh", "wlan", "show", "interfaces"],
                stderr=subprocess.DEVNULL,
                text=True,
                encoding="utf-8",
                errors="ignore",
            )
            for line in out.splitlines():
                line = line.strip()
                if line.lower().startswith("signal"):
                    value = line.split(":", 1)[1].strip().rstrip("%")
                    return int(value)
        except Exception:
            return -1
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
        "wifi": get_wifi_percent(),
        "time": get_time_str(),
        "date": get_date_str(),
        "bat": get_battery_percent(),
    }


def build_payload(m: dict) -> str:
    return (
        f"CPU:{m['cpu']};RAM:{m['ram']};GPU:{m['gpu']};GPUMEM:{m['gpu_mem']};WIFI:{m['wifi']};"
        f"TIME:{m['time']};DATE:{m['date']};BAT:{m['bat']}\n"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Gui thong so CPU/RAM/GPU/WIFI xuong board PLG qua Serial")
    parser.add_argument("--port", help="Ep dung cong serial nay (vd COM5, /dev/ttyACM0), bo qua buoc tu do tim + xac thuc.")
    parser.add_argument("--baud", type=int, default=115200, help="Toc do baud (mac dinh 115200)")
    parser.add_argument("--interval", type=float, default=0.8, help="Khoang thoi gian gui du lieu, giay (mac dinh 0.8)")
    parser.add_argument("--list", action="store_true", help="Liet ke cac cong serial va thoat")
    args = parser.parse_args()

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


if __name__ == "__main__":
    raise SystemExit(main())
