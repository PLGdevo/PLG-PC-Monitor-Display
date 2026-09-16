#pragma once
#include <stdint.h>
#include <stddef.h>
// Phan tich giao thuc du lieu Task Manager, KHONG phu thuoc duong truyen.
//
// Tach ra khoi PLG_serial_link tu Sprint 3: dinh dang du lieu ("CPU:..;RAM:..;...\n" + bat tay
// "PLG_ID?") giong het nhau du den tu USB Serial, BLE hay WiFi - chi cach nhan byte la khac.
// Moi transport chi viec dua byte tho vao protocol_feed() va cung cap 1 ham gui tra loi nguoc
// lai PC; toan bo viec gom dong/parse/day vao chart nam o mot cho duy nhat.

// Ham gui chuoi tra loi nguoc lai PC qua DUNG transport dang nhan du lieu (dung cho bat tay
// "PLG_ID?"). Moi transport tu hien thuc theo cach cua no (Serial.print, BLE notify...).
typedef void (*ProtocolReplyFn)(const char *text);

// Day 1 gia tri moi vao cuoi bieu do (dich mang sang trai), kep trong [0,cap].
// cap mac dinh 100 (dung cho cac chi so phan tram); WIFI (toc do mang Mbps) dung
// cap 127 (gioi han cua int8_t) vi toc do mang co the vuot qua 100.
void chart_push(int8_t *buf, int value, int cap = 100);

// Nap them du lieu tho vua nhan duoc tu 1 transport. Tu gom thanh dong (ket thuc bang '\n'/'\r'),
// parse khi du 1 dong, cap nhat chart/dong ho/pin va moc thoi gian nhan (xem protocol_ms_since_rx).
// reply duoc goi khi PC hoi "PLG_ID?" - truyen nullptr neu transport khong the tra loi.
void protocol_feed(const uint8_t *data, size_t len, ProtocolReplyFn reply);

// Xoa dong dang do dang + quen moc thoi gian da nhan. Goi khi chuyen transport, de dong bi cat
// giua chung cua transport cu khong dinh vao dong dau tien cua transport moi.
void protocol_reset();

// So mili giay ke tu dong hop le gan nhat. Tra ve UINT32_MAX neu CHUA TUNG nhan duoc gi
// (khac han voi "vua nhan xong" = 0), de goi y dung khi transport kiem tra ket noi.
uint32_t protocol_ms_since_rx();
