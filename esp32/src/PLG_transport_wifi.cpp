#include "PLG_transport_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "PLG_protocol.h"

// Cung namespace voi giao thuc ket noi dang chon (PLG_transport.cpp), khac key - deu la "cau
// hinh mang", tach ra 2 namespace chi lam kho theo doi.
static const char *NVS_NAMESPACE = "plg_net";

static WiFiServer tcp_server(WIFI_TCP_PORT);
static WiFiClient tcp_client;
static bool server_started = false;

// Cung nguong voi USB: PC gui moi ~0.8s (mac dinh monitor.py) nen 3s du de khong nhap nhay
// "mat ket noi" gia khi chi tre 1-2 chu ky gui.
static const uint32_t CONNECTION_TIMEOUT_MS = 3000;

// Gui tra loi bat tay nguoc lai PC qua chinh ket noi TCP dang mo.
static void wifi_reply(const char *text)
{
    if (tcp_client && tcp_client.connected())
        tcp_client.print(text);
}

void transport_wifi_begin()
{
    // STA = station: board tham gia mang WiFi san co (khong tu phat mang rieng).
    WiFi.mode(WIFI_STA);
    // Tat tiet kiem dien cua WiFi: che do mac dinh cho chip ngu giua cac beacon, lam tre goi
    // TCP toi vai tram ms - du de bieu do Task Manager giat cuc theo tung nhip cap nhat.
    WiFi.setSleep(false);

    if (!server_started)
    {
        tcp_server.begin();
        tcp_server.setNoDelay(true); // gui ngay, khong gom goi (Nagle) - du lieu cua ta nho va can tuc thi
        server_started = true;
    }

    // Da co cau hinh luu tu lan truoc -> ket noi lai ngay khi khoi dong, khong cho nguoi dung
    // phai vao SETTING > CONNECTION > WIFI moi chiu ket noi. Day la ca diem cua Sprint 5:
    // cam dien la chay, PC go thang toi dung IP tinh cu.
    if (wifi_config_exists())
        wifi_connect_saved();
}

void transport_wifi_poll()
{
    if (!server_started)
        return;

    // Chua co client hoac client cu da roi -> nhan ket noi moi neu co.
    if (!tcp_client || !tcp_client.connected())
    {
        WiFiClient incoming = tcp_server.available();
        if (incoming)
        {
            tcp_client.stop(); // dong han client cu truoc khi thay bang cai moi
            tcp_client = incoming;
            tcp_client.setNoDelay(true);
            Serial.printf("PLG_>>>> WIFI: PC da ket noi tu %s\n", tcp_client.remoteIP().toString().c_str());
        }
        return;
    }

    uint8_t buf[128];
    while (tcp_client.available() > 0)
    {
        int n = tcp_client.read(buf, sizeof(buf));
        if (n <= 0)
            break;
        protocol_feed(buf, (size_t)n, wifi_reply);
    }
}

bool transport_wifi_is_connected()
{
    // Doi HAI dieu kien: con ket noi TCP VA van dang nhan duoc du lieu. Chi kiem tra TCP la
    // khong du - mot socket bi "treo" (PC sleep, mat song giua chung) van bao connected rat
    // lau truoc khi TCP tu phat hien dut.
    return tcp_client && tcp_client.connected() && protocol_ms_since_rx() < CONNECTION_TIMEOUT_MS;
}

/*------------------- Phuc vu wizard chon mang -------------------*/

void wifi_scan_start()
{
    WiFi.scanDelete();          // xoa ket qua lan truoc, tranh doc nham du lieu cu
    WiFi.scanNetworks(true);    // true = khong chan, theo doi bang wifi_scan_status()
}

int wifi_scan_status()
{
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING)
        return WIFI_SCAN_RUNNING;
    if (n < 0)
        return WIFI_SCAN_FAILED;
    return n;
}

const char *wifi_scan_ssid(int index)
{
    // WiFi.SSID() tra ve String tam; giu lai vao bo dem tinh de con tro con hop le sau khi
    // String do bi huy (nguoi goi chi dung de ve len man hinh ngay sau do).
    static char ssid_buf[33]; // SSID toi da 32 ky tu + '\0'
    String s = WiFi.SSID(index);
    strncpy(ssid_buf, s.c_str(), sizeof(ssid_buf) - 1);
    ssid_buf[sizeof(ssid_buf) - 1] = '\0';
    return ssid_buf;
}

int wifi_scan_rssi(int index)
{
    return WiFi.RSSI(index);
}

void wifi_scan_clear()
{
    WiFi.scanDelete();
}

void wifi_connect(const char *ssid, const char *password)
{
    WiFi.begin(ssid, password);
}

bool wifi_is_online()
{
    return WiFi.status() == WL_CONNECTED;
}

const char *wifi_local_ip_str()
{
    static char ip_buf[16]; // "255.255.255.255" + '\0'
    String s = WiFi.localIP().toString();
    strncpy(ip_buf, s.c_str(), sizeof(ip_buf) - 1);
    ip_buf[sizeof(ip_buf) - 1] = '\0';
    return ip_buf;
}

/*------------------- Cau hinh WiFi da luu -------------------*/

bool wifi_config_exists()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) // true = chi doc
        return false;
    bool exists = prefs.isKey("ssid");
    prefs.end();
    return exists;
}

bool wifi_config_save(const char *ssid, const char *password)
{
    if (!wifi_is_online())
        return false; // chua online thi cac dia chi duoi day deu la 0.0.0.0, luu vao vo nghia

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) // false = mo o che do ghi
        return false;

    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    // Luu dang so 32-bit thay vi chuoi: dung dinh dang WiFi.config() nhan vao, khoi phai
    // parse lai chuoi "192.168.1.5" luc doc ra.
    prefs.putUInt("ip", (uint32_t)WiFi.localIP());
    prefs.putUInt("gw", (uint32_t)WiFi.gatewayIP());
    prefs.putUInt("mask", (uint32_t)WiFi.subnetMask());
    prefs.putUInt("dns", (uint32_t)WiFi.dnsIP());
    prefs.end();

    Serial.printf("PLG_>>>> WIFI: da luu cau hinh tinh \"%s\" @ %s\n", ssid, wifi_local_ip_str());
    return true;
}

void wifi_config_clear()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false))
        return;
    // Xoa tung key thay vi prefs.clear(): namespace nay con giu ca giao thuc ket noi dang
    // chon (key "mode" cua PLG_transport.cpp), xoa sach se lam mat luon lua chon do.
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.remove("ip");
    prefs.remove("gw");
    prefs.remove("mask");
    prefs.remove("dns");
    prefs.end();
    Serial.println("PLG_>>>> WIFI: da quen mang da luu");
}

const char *wifi_config_ssid()
{
    static char ssid_buf[33];
    ssid_buf[0] = '\0';

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true))
        return ssid_buf;
    prefs.getString("ssid", ssid_buf, sizeof(ssid_buf));
    prefs.end();
    return ssid_buf;
}

bool wifi_connect_saved()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true))
        return false;
    if (!prefs.isKey("ssid"))
    {
        prefs.end();
        return false;
    }

    char ssid[33] = "";
    char pass[64] = "";
    prefs.getString("ssid", ssid, sizeof(ssid));
    prefs.getString("pass", pass, sizeof(pass));
    IPAddress ip(prefs.getUInt("ip", 0));
    IPAddress gw(prefs.getUInt("gw", 0));
    IPAddress mask(prefs.getUInt("mask", 0));
    IPAddress dns(prefs.getUInt("dns", 0));
    prefs.end();

    // Dat dia chi tinh TRUOC khi begin(): goi sau se bi DHCP ghi de.
    WiFi.config(ip, gw, mask, dns);
    WiFi.begin(ssid, pass);
    Serial.printf("PLG_>>>> WIFI: ket noi lai \"%s\" bang IP tinh %s\n", ssid, ip.toString().c_str());
    return true;
}
