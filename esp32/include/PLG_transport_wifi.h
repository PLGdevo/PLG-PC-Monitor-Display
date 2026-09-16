#pragma once
#include <stdint.h>
// Transport WiFi: PC gui du lieu Task Manager toi board qua TCP thay vi cap USB.
//
// Board dong vai tro TCP SERVER (PC chu dong ket noi toi IP cua board) chu khong phai client:
// nhu vay board khong can biet truoc dia chi cua PC, va chinh la ly do Sprint 5 phai luu duoc
// 1 bo IP tinh - de PC luon biet go dung dia chi nao.

// Cong TCP board lang nghe. 5005 nam ngoai vung cong he thong (<1024) va khong trung cac dich
// vu pho bien, de khong dinh tuong lua mac dinh cua Windows hon cac cong quen thuoc.
#define WIFI_TCP_PORT 5005

/*------------------- Vong doi transport (goi tu PLG_transport.cpp) -------------------*/

// Bat WiFi o che do STA (station). KHONG tu ket noi mang - viec chon mang do wizard
// (PLG_wifi_ui.*) hoac cau hinh da luu (Sprint 5) quyet dinh.
void transport_wifi_begin();

// Nhan ket noi TCP tu PC + doc du lieu dang den, dua vao PLG_protocol. Goi moi vong loop().
void transport_wifi_poll();

// true khi PC dang thuc su gui du lieu qua TCP (khong chi la "da vao duoc WiFi").
bool transport_wifi_is_connected();

/*------------------- Ham phuc vu wizard chon mang (PLG_wifi_ui.cpp) -------------------*/

// Bat dau quet mang o che do KHONG CHAN. WiFi.scanNetworks() dang dong bo mat 2-5 giay -
// du de lam dung hinh va treo ca encoder neu goi thang trong loop().
void wifi_scan_start();

// Trang thai quet: >=0 la so mang tim duoc, hoac 1 trong 2 gia tri duoi day.
#define WIFI_SCAN_RUNNING (-1)
#define WIFI_SCAN_FAILED (-2)
int wifi_scan_status();

// Thong tin 1 mang trong ket qua quet (index hop le: 0 .. wifi_scan_status()-1).
const char *wifi_scan_ssid(int index);
int wifi_scan_rssi(int index);

// Giai phong bo nho ket qua quet. Goi khi roi man hinh chon mang.
void wifi_scan_clear();

// Bat dau ket noi toi mang (khong chan - theo doi bang wifi_is_online()).
void wifi_connect(const char *ssid, const char *password);

// true khi da vao duoc mang WiFi (khac transport_wifi_is_connected: chua chac PC da gui gi).
bool wifi_is_online();

// Dia chi IP board dang duoc cap, dang chuoi "192.168.x.x". Tra ve "0.0.0.0" khi chua vao mang.
const char *wifi_local_ip_str();

/*------------------- Cau hinh WiFi da luu (1 bo duy nhat) -------------------*/
// Luu vao cung namespace NVS "plg_net" voi giao thuc dang chon (PLG_transport.cpp), khac key.
// Chi giu DUNG 1 bo: thiet bi nay gan co dinh voi 1 may tinh/1 mang, danh sach nhieu mang chi
// lam UI phuc tap them ma khong giai quyet nhu cau that nao.
//
// Luu ca IP/gateway/subnet/DNS (khong chi SSID/mat khau) de nhung lan sau ket noi bang dia chi
// TINH: PC luon go toi dung mot dia chi, khong phai do lai moi khi router cap IP khac.

// true neu da co cau hinh luu (du de bo qua wizard va ket noi thang).
bool wifi_config_exists();

// Luu SSID/mat khau + dia chi IP/gateway/subnet/DNS DANG DUNG thanh cau hinh tinh.
// Goi khi dang online (cac dia chi duoc lay tu ket noi hien tai), ghi de bo cu neu co.
bool wifi_config_save(const char *ssid, const char *password);

// Xoa cau hinh da luu ("quen mang"), buoc lan sau phai chay lai wizard.
void wifi_config_clear();

// Ket noi bang cau hinh da luu (WiFi.config dia chi tinh + WiFi.begin). Khong chan - theo doi
// bang wifi_is_online() nhu wifi_connect(). Tra ve false neu chua co cau hinh nao duoc luu.
bool wifi_connect_saved();

// SSID trong cau hinh da luu, de hien len man hinh. Tra ve "" neu chua luu gi.
const char *wifi_config_ssid();
