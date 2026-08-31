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
import time

import psutil
import serial
import serial.tools.list_ports

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


def identify_device(port: str, baud: int, timeout: float = 1.5) -> bool:
    """Mo thu cong serial, gui IDENTITY_CMD va cho board tra loi IDENTITY_REPLY.
    Tra True neu xac nhan dung la board PLG, False neu khong phai/khong phan hoi."""
    try:
        with serial.Serial(port, baud, timeout=0.3) as ser:
            time.sleep(0.3)  # cho board on dinh sau khi mo cong (DTR toggle co the reset board)
            ser.reset_input_buffer()
            ser.write(IDENTITY_CMD)
            buf = ""
            deadline = time.time() + timeout
            while time.time() < deadline:
                chunk = ser.read(ser.in_waiting or 1).decode("ascii", errors="ignore")
                if chunk:
                    buf += chunk
                    if IDENTITY_REPLY in buf:
                        return True
    except serial.SerialException:
        return False
    return False


def find_pico_port(baud: int) -> str | None:
    """Tu dong do va xac thuc board PLG, khong can nguoi dung chon cong thu cong.
    Uu tien thu cac cong co VID/PID giong Pico truoc (nhanh), sau do thu them cac
    cong serial con lai (phong khi build/driver khac lam VID/PID khac di). Chi
    nhan cong nao xac thuc thanh cong qua identify_device()."""
    ports = list(serial.tools.list_ports.comports())
    likely = [p for p in ports if p.vid is not None and p.pid is not None and (p.vid, p.pid) in PICO_VID_PID]
    others = [p for p in ports if p not in likely]
    for p in likely + others:
        if identify_device(p.device, baud):
            return p.device
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


def build_payload() -> str:
    cpu = get_cpu_percent()
    ram = get_ram_percent()
    gpu = get_gpu_percent()
    gpu_mem = get_gpu_mem_percent()
    wifi = get_wifi_percent()
    now = get_time_str()
    today = get_date_str()
    bat = get_battery_percent()
    return (
        f"CPU:{cpu};RAM:{ram};GPU:{gpu};GPUMEM:{gpu_mem};WIFI:{wifi};"
        f"TIME:{now};DATE:{today};BAT:{bat}\n"
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
    if fixed_port and not identify_device(fixed_port, args.baud):
        print(f"Canh bao: cong {fixed_port} khong tra loi xac thuc \"{IDENTITY_REPLY}\", "
              f"co the khong phai board PLG. Van tiep tuc vi da chi dinh --port thu cong.")

    # lan goi dau cpu_percent tra ve 0.0, bo qua de lay mau chuan
    psutil.cpu_percent(interval=None)
    time.sleep(0.2)

    # Vong lap ngoai: tu dong do/xac thuc lai cong khi chua cam dung board (hoac chua
    # cam gi), va tu ket noi lai neu bi rut day/mat ket noi giua chung - khong con can
    # nguoi dung tu chon cong; quan trong khi chay nen (Startup) vi nguoi dung co the
    # bat may truoc, cam Pico vao sau (vd sau 30 phut).
    try:
        while True:
            port = fixed_port or find_pico_port(args.baud)
            if port is None:
                if not background:
                    print("Khong tim/xac thuc duoc board PLG, dang cho... (cam thiet bi vao)")
                time.sleep(2)
                continue

            try:
                ser = serial.Serial(port, args.baud, timeout=1)
            except serial.SerialException as exc:
                if not background:
                    print(f"Loi mo cong {port}: {exc}, thu lai sau 2s")
                time.sleep(2)
                continue

            print(f"Da xac thuc board PLG tren {port} @ {args.baud} baud, gui du lieu moi {args.interval}s. Ctrl+C de dung.")
            try:
                with ser:
                    while True:
                        payload = build_payload()
                        ser.write(payload.encode("ascii"))
                        print(payload.strip())
                        time.sleep(args.interval)
            except serial.SerialException as exc:
                print(f"Mat ket noi serial: {exc}, thu ket noi lai...")
                time.sleep(2)
                continue
    except KeyboardInterrupt:
        print("\nDa dung.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
