#include "PLG_charts.h"
#include <stdio.h>
#include <string.h>
#include "PLG_state.h"
#include "PLG_theme.h"
#include "PLG_display.h"

void get_chart_layout(ChartLayout out[5])
{
    const int16_t fullW = 300, topH = 80, midH = 58, botH = 54;
    const int16_t startX = 10, gapX = 8, gapY = 6, startY = 26;
    const int16_t halfW = (fullW - gapX) / 2;
    const int16_t midY = startY + topH + gapY;
    const int16_t botY = midY + midH + gapY;

    out[0] = {startX, startY, halfW, topH};                           // CPU
    out[1] = {(int16_t)(startX + halfW + gapX), startY, halfW, topH}; // RAM
    out[2] = {startX, midY, halfW, midH};                             // GPU 3D
    out[3] = {(int16_t)(startX + halfW + gapX), midY, halfW, midH};   // GPU memory
    out[4] = {startX, botY, fullW, botH};                             // WIFI
}

void draw_chart_frame(int16_t x, int16_t y, int16_t w, int16_t h)
{
    myTFT.TFTfillRoundRect(x, y, w, h, 4, UI_PANEL);
    myTFT.TFTdrawRoundRect(x, y, w, h, 4, UI_BORDER);
}

// luu buf da ve lan truoc cho toi da 5 chart (CPU/RAM/GPU/GPUMEM/WIFI) de phat hien "khong doi
// gi ca" va bo qua hoan toan lan ve do (xem draw_chart_data) - -2 (ngoai khoang hop le -1..100)
// danh dau "chua ve lan nao" nen lan dau luon ve du chartId gi.
static int8_t last_drawn_buf[5][CHART_SAMPLES];
static bool last_drawn_valid[5] = {false, false, false, false, false};

void reset_chart_cache()
{
    for (int i = 0; i < 5; i++)
        last_drawn_valid[i] = false;
}

void draw_chart_data(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, int8_t *buf, uint16_t lineColor,
                      int warnAt, const char *warnText, int8_t chartId)
{
    bool haveCache = (chartId >= 0 && chartId < 5);
    if (haveCache && last_drawn_valid[chartId] && memcmp(last_drawn_buf[chartId], buf, CHART_SAMPLES) == 0)
        return; // du lieu giong het lan ve truoc -> bo qua, tranh chop hinh khong can thiet
    if (haveCache)
    {
        memcpy(last_drawn_buf[chartId], buf, CHART_SAMPLES);
        last_drawn_valid[chartId] = true;
    }

    int16_t plotTop = y + 20; // chua cho nhan chu phia tren
    int16_t plotBottom = y + h - 4;
    int16_t plotH = plotBottom - plotTop;
    int last_val = buf[CHART_SAMPLES - 1];
    bool warn = (last_val >= warnAt);
    uint16_t textCol = warn ? UI_DANGER : lineColor; // dong mau nhan chu voi mau ve chart

    // xoa vung do thi cu; rong w-2 (khong phai w-4) de xoa het ca chat diem cuoi (fillCircle
    // ban kinh 2 tai x+w-4) - truoc day rong w-4 hut mat 1-2px ben phai, lam chat diem cu
    // khong bi xoa sach, con vet chong len chat diem moi
    myTFT.TFTfillRect(x + 2, plotTop - 2, w - 2, plotH + 6, UI_PANEL);

    for (int g = 1; g < 4; g++) // luoi mo 25/50/75%
    {
        int16_t gy = plotBottom - (plotH * g) / 4;
        myTFT.TFTdrawFastHLine(x + 4, gy, w - 8, UI_BORDER);
    }

    for (int i = 0; i < CHART_SAMPLES - 1; i++)
    {
        // bo qua doan noi voi diem chua co du lieu that (-1), tranh tao "bac" dung dot ngot
        if (buf[i] < 0 || buf[i + 1] < 0)
            continue;
        int16_t x0 = x + 4 + (i * (w - 8)) / (CHART_SAMPLES - 1);
        int16_t x1 = x + 4 + ((i + 1) * (w - 8)) / (CHART_SAMPLES - 1);
        int16_t y0 = plotBottom - (buf[i] * plotH) / 100;
        int16_t y1 = plotBottom - (buf[i + 1] * plotH) / 100;
        // 1 net (khong ve chong 2px nhu truoc) - giam mot nua so lan goi ve line moi lan cap
        // nhat, ve nhanh hon -> bot chop hinh (thay bang 1 diem tron nho o cuoi cho de nhin)
        myTFT.TFTdrawLine(x0, y0, x1, y1, lineColor);
    }

    if (last_val >= 0)
    {
        int16_t lastX = x + w - 4;
        int16_t lastY = plotBottom - (last_val * plotH) / 100;
        myTFT.TFTfillCircle(lastX, lastY, 2, lineColor); // diem cuoi noi bat
    }

    myTFT.TFTfillRect(x + 2, y + 2, w - 4, 16, UI_PANEL); // xoa dong nhan cu
    char label_txt[28];
    if (last_val < 0)
        snprintf(label_txt, sizeof(label_txt), "%s  --", label);
    else if (warn && warnText != nullptr)
        snprintf(label_txt, sizeof(label_txt), "%s %d%% %s", label, last_val, warnText);
    else
        snprintf(label_txt, sizeof(label_txt), "%s  %d%%", label, last_val);
    myTFT.TFTdrawText(x + 6, y + 3, label_txt, textCol, UI_PANEL, 2);

    myTFT.TFTdrawRoundRect(x, y, w, h, 4, warn ? UI_DANGER : UI_BORDER); // vien do khi bao dong
}
