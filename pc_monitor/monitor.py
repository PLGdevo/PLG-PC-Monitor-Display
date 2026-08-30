#!/usr/bin/env python3
"""
PLG PC Task Monitor
--------------------
Doc thong so CPU / RAM / GPU / WIFI tren may tinh va gui qua cong Serial (USB)
xuong board Raspberry Pi Pico (PLG_TFT_LCD_TASKMANAGER).

Yeu cau cai dat:
    pip install -r requirements.txt

Chay:
    python monitor.py                  # tu do tim cong Pico
    python monitor.py --port COM5      # chi dinh cong thu cong
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


PICO_VID_PID = {
    (0x2E8A, 0x0005),  # RP2 Boot (BOOTSEL)
    (0x2E8A, 0x000A),  # Pico CDC (stdio USB mac dinh cua Pico SDK)
}


def find_pico_port() -> str | None:
    """Tu dong do tim cong serial cua Pico theo VID/PID; None neu khong thay."""
    for p in serial.tools.list_ports.comports():
        if p.vid is not None and p.pid is not None and (p.vid, p.pid) in PICO_VID_PID:
            return p.device
    return None


def get_cpu_percent() -> int:
    return int(psutil.cpu_percent(interval=None))


def get_ram_percent() -> int:
    return int(psutil.virtual_memory().percent)


def get_gpu_percent() -> int:
    if not _NVML_OK:
        return -1
    try:
        handle = pynvml.nvmlDeviceGetHandleByIndex(0)
        util = pynvml.nvmlDeviceGetUtilizationRates(handle)
        return int(util.gpu)
    except Exception:
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
    wifi = get_wifi_percent()
    now = get_time_str()
    today = get_date_str()
    bat = get_battery_percent()
    return f"CPU:{cpu};RAM:{ram};GPU:{gpu};WIFI:{wifi};TIME:{now};DATE:{today};BAT:{bat}\n"


def choose_port_interactively() -> str | None:
    """Hien menu cho nguoi dung chon cong COM khi khoi dong (dung cho ban .exe)."""
    auto = find_pico_port()
    while True:
        ports = list(serial.tools.list_ports.comports())
        print("\n=== PLG_monitor - Chon cong COM ===")
        if not ports:
            print("Khong tim thay cong serial nao dang cam vao may.")
        for i, p in enumerate(ports):
            mark = "  <== Pico tu phat hien" if p.device == auto else ""
            print(f"  [{i}] {p.device}\t{p.description}{mark}")
        print("  [r] Quet lai")
        print("  [q] Thoat")
        choice = input("Chon so thu tu cong (Enter de dung cong Pico tu phat hien): ").strip().lower()

        if choice == "":
            if auto:
                return auto
            print("Khong co cong tu phat hien, hay chon mot so thu tu ben tren.")
            continue
        if choice == "q":
            return None
        if choice == "r":
            continue
        if choice.isdigit() and 0 <= int(choice) < len(ports):
            return ports[int(choice)].device
        print("Lua chon khong hop le, thu lai.")


def main() -> int:
    parser = argparse.ArgumentParser(description="Gui thong so CPU/RAM/GPU/WIFI xuong board PLG qua Serial")
    parser.add_argument("--port", help="Cong serial (vd COM5, /dev/ttyACM0). Bo trong se tu do tim.")
    parser.add_argument("--baud", type=int, default=115200, help="Toc do baud (mac dinh 115200)")
    parser.add_argument("--interval", type=float, default=1.0, help="Khoang thoi gian gui du lieu, giay (mac dinh 1.0)")
    parser.add_argument("--list", action="store_true", help="Liet ke cac cong serial va thoat")
    args = parser.parse_args()

    if args.list:
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            print("Khong tim thay cong serial nao.")
        for p in ports:
            print(f"{p.device}\t{p.description}\tVID:PID={p.vid}:{p.pid}")
        return 0

    port = args.port
    if port is None:
        port = choose_port_interactively()
    if port is None:
        print("Da huy, khong chon cong nao.")
        return 1

    print(f"Ket noi {port} @ {args.baud} baud, gui du lieu moi {args.interval}s. Ctrl+C de dung.")

    try:
        ser = serial.Serial(port, args.baud, timeout=1)
    except serial.SerialException as exc:
        print(f"Loi mo cong {port}: {exc}")
        return 1

    # lan goi dau cpu_percent tra ve 0.0, bo qua de lay mau chuan
    psutil.cpu_percent(interval=None)
    time.sleep(0.2)

    try:
        with ser:
            while True:
                payload = build_payload()
                ser.write(payload.encode("ascii"))
                print(payload.strip())
                time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nDa dung.")
        return 0
    except serial.SerialException as exc:
        print(f"Mat ket noi serial: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
