#pragma once
#include <stdint.h>
// Ve bieu do dang duong (line chart) cho Task Manager: CPU/RAM/GPU/GPUMEM/WIFI.

// vi tri + kich thuoc 1 the chart trong man hinh Task Manager
struct ChartLayout
{
    int16_t x, y, w, h;
};

// vi tri 6 the chart (CPU/RAM/GPU/GPUMEM/WIFI/TEMP), tinh 1 lan dung chung cho ve khung lan dau va cap nhat du lieu.
// WIFI truoc day chiem tron hang duoi (fullW), gio chia doi voi TEMP (chia task WIFI lam 2).
void get_chart_layout(ChartLayout out[6]);

// khung "card" cua 1 bieu do: nen + vien, chi ve 1 lan khi vao man hinh
void draw_chart_frame(int16_t x, int16_t y, int16_t w, int16_t h);

// cap nhat du lieu ben trong 1 chart: chi xoa/ve lai vung do thi, khong dung lai ca card.
// warnAt: nguong bao dong (vd 90), warnText: dong chu hien khi vuot nguong (vd "BOOST NOW")
// chartId (0-4, ung voi CPU/RAM/GPU/GPUMEM/WIFI trong MONITOR_TASKMANAGER): dung de cache mau
// du lieu da ve lan truoc - neu buf khong doi so voi lan ve truoc thi bo qua hoan toan (khong
// xoa/ve lai), tranh chop hinh khi mot chart dung yen trong khi cac chart khac van cap nhat.
// unit: hau to don vi hien sau %/gia tri (vd "%" cho CPU/RAM/GPU/WIFI, "C" cho nhiet do)
void draw_chart_data(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, int8_t *buf, uint16_t lineColor,
                      int warnAt = 101, const char *warnText = nullptr, int8_t chartId = -1, const char *unit = "%");

// bat buoc lan goi draw_chart_data ke tiep (voi chartId hop le) phai ve lai du buf co giong lan
// truoc hay khong - goi khi vung ve bi xoa tu ben ngoai (vd MONITOR_TASKMANAGER xoa toan man
// hinh luc vua vao lai), neu khong cache se lam chart "tuong" du lieu khong doi va bo qua ve,
// de lai vung trong tren nen vua xoa.
void reset_chart_cache();
