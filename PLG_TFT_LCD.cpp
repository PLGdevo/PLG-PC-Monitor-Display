#include "PLG_setup.h"
#include <string.h>
#include <stdio.h>

/*!
		@param X,Y toa do
		@param  W,H width hight
		@param img toa do mau
		ve 1 pixel theo width het hang thi xuong dong tiep theo
*/
void drawLogoFull(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *img)
{
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			myTFT.TFTdrawPixel(x + i, y + j, img[j * w + i]);
		}
	}
}
// nen den tuyet doi, dong bo voi nen den cua logo -> khong con khung sang phia sau logo
static const uint16_t SPLASH_BG = RGB565(0, 0, 0);
void read_taskmanager_serial(); // forward declare, dinh nghia phia duoi file

// ve 1 khung loading (thanh bar + chu "Loading...N%"), dung chung cho ca doan chay
// nhanh 0-90% va doan cho du lieu dao dong 90-98%
static void draw_loading_frame(int16_t barX, int16_t barY, int16_t barW, int16_t barH, int a)
{
	char load[20] = "";
	// %3d: do dai chuoi luon co dinh (khong doi tu 1 sang 2, 3 chu so) -> khong can
	// fillRect xoa ca vung truoc khi ve lai, tranh hien tuong chop den moi lan cap nhat
	snprintf(load, sizeof(load), "Loading...%3d%%", a);
	myTFT.TFTdrawText(76, 208, load, UI_TEXT, SPLASH_BG, 2);

	int16_t fillW = (barW - 4) * a / 100;
	myTFT.TFTfillRect(barX + 2, barY + 2, fillW, barH - 4, UI_ACCENT);
}

void MONITOR_BEGIN()
{
	myTFT.TFTfillScreen(SPLASH_BG);
	drawLogoFull(100, 50, 120, 100, PLG_logo);

	// thanh loading ngay duoi logo, chu "Loading...N%" nam duoi thanh
	int16_t barX = 60, barY = 180, barW = 200, barH = 12;
	myTFT.TFTdrawRoundRect(barX, barY, barW, barH, 4, UI_BORDER);

	for (int a = 0; a <= 90; a++)
	{
		draw_loading_frame(barX, barY, barW, barH, a);
		sleep_ms(10);
	}

	// Doan cho du lieu that tu PC (monitor.py/.exe): chay cham dan tu 90% len 98% roi
	// dung lai o 98% (chi cap nhat lai hinh, khong lui xuong) cho den khi:
	//  - nhan duoc du lieu that qua Serial (taskmanager_dirty = true), hoac
	//  - nguoi dung nhan nut encoder 2 lan de bo qua thu cong (vd khi chua muon bat monitor.py)
	// Tranh truong hop vao thang man hinh chinh voi du lieu cu/gia (-1) khi PC chua gui gi.
	bool last_btn = gpio_get(button);
	int click_count = 0;
	int a = 90;
	while (!taskmanager_dirty && click_count < 2)
	{
		read_taskmanager_serial();

		bool now_btn = gpio_get(button);
		if (last_btn == 1 && now_btn == 0) // canh nhan xuong = 1 lan click
			click_count++;
		last_btn = now_btn;

		draw_loading_frame(barX, barY, barW, barH, a);
		if (a < 98)
			a++;

		sleep_ms(250);
	}

	for (int a2 = 99; a2 <= 100; a2++)
	{
		draw_loading_frame(barX, barY, barW, barH, a2);
		sleep_ms(40);
	}
	// giu hinh 100% hien ro mot nhip truoc khi chuyen man hinh, tranh cam giac "ket o 99%"
	sleep_ms(400);
	myTFT.TFTfillScreen(UI_BG);
}
void live_pico()
{
	if (to_ms_since_boot(get_absolute_time()) - timer1 > 1000)
	{
		flat1 = !flat1;
		gpio_put(PIN_LIGHT_BOARD, flat1);
		// printf("PLG_>>>>\n\r");
		timer1 = to_ms_since_boot(get_absolute_time());

		// battery1 = battery1 + 10;
		// battery2 = battery2 + 10;
		// CONNECT_STATUS = !CONNECT_STATUS;
		// status_machine = !status_machine;
		// if (battery1 >= 105)
		// {
		// 	battery1 = 0;
		// 	battery2 = 0;
		// }
		// if (flat1 == 1)
		// {
		// 	player = 1;
		// }
		// else
		// {
		// 	player = 2;
		// }
		EN_CLK = gpio_get(CLK);
		EN_DT = gpio_get(DT);
		EN_BT = gpio_get(button);
		printf("ENA:%d  ENB:%d  SW:%d\n", EN_CLK, EN_DT, EN_BT);
	}
}
// ve 1 icon pin (4 vach) + so % tai vi tri iconX, dung chung cho battery1 va battery2
void draw_battery(int16_t iconX, int16_t textX, int8_t &value, int8_t &lastValue)
{
	if (value < 0)
		value = 0;
	if (value > 100)
		value = 100;
	if (lastValue != value)
	{
		myTFT.TFTfillRect(textX, 5, 40, 17, UI_BG);
		lastValue = value;
	}

	char text[4] = "";
	itoa(value, text, 10);
	myTFT.TFTdrawText(textX, 5, text, UI_TEXT, UI_BG, 2);

	uint16_t col = (value <= 20) ? UI_DANGER : UI_CPU; // luon xanh la, khong doi theo UI_ACCENT
	myTFT.TFTdrawRoundRect(iconX, 5, 34, 15, 3, UI_BORDER);
	myTFT.TFTfillRect(iconX + 34, 8, 4, 8, UI_BORDER);

	int bars = (value <= 25) ? 1 : (value <= 50) ? 2
							   : (value <= 75)	  ? 3
												  : 4;
	for (int i = 0; i < 4; i++)
	{
		myTFT.TFTfillRoundRect(iconX + 1 + i * 8, 6, 7, 13, 2, (i < bars) ? col : UI_PANEL);
	}
}

void MONITOR_STATUS()
{
	if (last_status != status)
	{
		myTFT.TFTdrawLine(0, 23, 320, 23, UI_BORDER);
		last_status = status;
	}

	draw_battery(20, 60, battery1, last_battery1);	 // pin remote
	draw_battery(100, 140, battery2, last_battery2); // pin may

	/*---------------------status connected------------------------------*/
	if (LAST_CONNECT_STATUS != CONNECT_STATUS)
	{
		myTFT.TFTfillRect(288, 0, 32, 20, UI_BG);
		if (CONNECT_STATUS)
		{
			myTFT.TFTfillRoundRect(290, 15, 8, 5, 0, UI_ACCENT);	// song min
			myTFT.TFTfillRoundRect(300, 10, 8, 10, 0, UI_ACCENT); // song mid
			myTFT.TFTfillRoundRect(310, 5, 8, 15, 0, UI_ACCENT);	// song max
		}
		else
		{
			myTFT.TFTdrawLine(290, 5, 318, 15, UI_DANGER);
			myTFT.TFTdrawLine(318, 5, 290, 15, UI_DANGER);
		}
		LAST_CONNECT_STATUS = CONNECT_STATUS;
	}
}
/*------------------- PC Task Manager: nhan du lieu serial + ve chart -------------------*/
void chart_push(int8_t *buf, int value)
{
	if (value < 0)
		value = 0;
	if (value > 100)
		value = 100;
	memmove(buf, buf + 1, (CHART_SAMPLES - 1) * sizeof(int8_t));
	buf[CHART_SAMPLES - 1] = (int8_t)value;
}

// doc tung ky tu tu USB stdio (khong block), gom thanh dong, parse khi gap '\n'
void read_taskmanager_serial()
{
	int c;
	while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT)
	{
		if (c == '\n' || c == '\r')
		{
			if (serial_line_len > 0)
			{
serial_line_buf[serial_line_len] = '\0';
				// Bat tay nhan dien thiet bi: monitor.py gui "PLG_ID?" khi do cong tu dong,
				// board tra loi cau co dinh nay de PC xac nhan dung la board PLG (khong
				// phai thiet bi USB Serial khac) ma khong can nguoi dung tu chon cong.
				if (strcmp(serial_line_buf, "PLG_ID?") == 0)
				{
					printf("I AM PLG_TFT_LCD_TASKMANAGER\n");
				}
				else
				{
					int cpu = -1, ram = -1, gpu = -1, gpumem = -1, wifi = -1, bat = -1;
					char time_buf[9] = "", date_buf[11] = "";
					int n = sscanf(serial_line_buf, "CPU:%d;RAM:%d;GPU:%d;GPUMEM:%d;WIFI:%d;TIME:%8[^;];DATE:%10[^;];BAT:%d",
									&cpu, &ram, &gpu, &gpumem, &wifi, time_buf, date_buf, &bat);
					if (n >= 5)
					{
						chart_push(chart_cpu, cpu);
						chart_push(chart_ram, ram);
						chart_push(chart_gpu, gpu);
						chart_push(chart_gpumem, gpumem);
						chart_push(chart_wifi, wifi);
						taskmanager_dirty = true;
					}
					if (n >= 6 && strcmp(current_time_str, time_buf) != 0)
					{
						strncpy(current_time_str, time_buf, sizeof(current_time_str) - 1);
						current_time_str[sizeof(current_time_str) - 1] = '\0';
						clock_dirty = true;
					}
					if (n >= 7 && strcmp(current_date_str, date_buf) != 0)
					{
						strncpy(current_date_str, date_buf, sizeof(current_date_str) - 1);
						current_date_str[sizeof(current_date_str) - 1] = '\0';
						clock_dirty = true;
					}
					// pin ben trai = pin laptop nhan tu monitor.py; -1 nghia la may khong co pin (PC ban) -> giu nguyen gia tri cu
					if (n == 8 && bat >= 0)
					{
						battery1 = (int8_t)bat;
					}
				}
				serial_line_len = 0;
			}
		}
		else if (serial_line_len < sizeof(serial_line_buf) - 1)
		{
			serial_line_buf[serial_line_len++] = (char)c;
		}
		else
		{
			serial_line_len = 0; // dong qua dai, bo qua
		}
	}
}

// khung "card" cua 1 bieu do: nen + vien, chi ve 1 lan khi vao man hinh
// (khong dung lai moi lan cap nhat du lieu -> tranh chop, man hinh muot hon)
void draw_chart_frame(int16_t x, int16_t y, int16_t w, int16_t h)
{
	myTFT.TFTfillRoundRect(x, y, w, h, 4, UI_PANEL);
	myTFT.TFTdrawRoundRect(x, y, w, h, 4, UI_BORDER);
}

// cap nhat du lieu ben trong 1 chart: chi xoa/ve lai vung do thi, khong dung lai ca card
// warnAt: nguong bao dong (vd 90), warnText: dong chu hien khi vuot nguong (vd "BOOST NOW")
void draw_chart_data(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, int8_t *buf, uint16_t lineColor,
					  int warnAt = 101, const char *warnText = nullptr)
{
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
		myTFT.TFTdrawLine(x0, y0, x1, y1, lineColor);
		myTFT.TFTdrawLine(x0, y0 + 1, x1, y1 + 1, lineColor); // net day 2px cho de nhin
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

// vi tri 5 the chart (CPU/RAM/GPU/GPUMEM/WIFI), tinh 1 lan dung chung cho ve khung lan dau
// va cap nhat du lieu. GPU tach lam 2 nua: GPU 3D (usage core) va GPU memory (VRAM).
struct ChartLayout
{
	int16_t x, y, w, h;
};
void get_chart_layout(ChartLayout out[5])
{
	const int16_t fullW = 300, topH = 80, midH = 58, botH = 54;
	const int16_t startX = 10, gapX = 8, gapY = 6, startY = 26;
	const int16_t halfW = (fullW - gapX) / 2;
	const int16_t midY = startY + topH + gapY;
	const int16_t botY = midY + midH + gapY;

	out[0] = {startX, startY, halfW, topH};						  // CPU
	out[1] = {(int16_t)(startX + halfW + gapX), startY, halfW, topH}; // RAM
	out[2] = {startX, midY, halfW, midH};								  // GPU 3D
	out[3] = {(int16_t)(startX + halfW + gapX), midY, halfW, midH};	  // GPU memory
	out[4] = {startX, botY, fullW, botH};								  // WIFI
}

// gio hien tai o goc tren-phai thanh trang thai, dat vao khoang trong giua pin may
// (ket thuc ~x174) va icon ket noi (bat dau x288) -> khong chong len noi dung pin
static char last_time_str_top[9] = "";
void draw_taskmanager_clock()
{
	if (strcmp(last_time_str_top, current_time_str) != 0)
	{
		// size2, chi hien HH:MM (bo giay) de vua khoang trong: 5 ky tu * size*(5+1)=12px = 60px,
		// x=190 -> ket thuc 250px, khong vuot nguong tran/wrap cua TFTdrawText va khong de len pin may
		char hm[6] = "";
		strncpy(hm, current_time_str, 5);
		hm[5] = '\0';
		myTFT.TFTfillRect(188, 4, 65, 17, UI_BG);
		myTFT.TFTdrawText(190, 5, hm, UI_ACCENT, UI_BG, 2); // dong bo mau voi mau nhan dien da chon
		strncpy(last_time_str_top, current_time_str, sizeof(last_time_str_top) - 1);
		last_time_str_top[sizeof(last_time_str_top) - 1] = '\0';
	}
}

void MONITOR_TASKMANAGER()
{
	ChartLayout c[5];
	get_chart_layout(c);

	if (last_desktop != desktop)
	{
		myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
		for (int i = 0; i < 5; i++)
			draw_chart_frame(c[i].x, c[i].y, c[i].w, c[i].h);
		last_desktop = desktop;
		taskmanager_dirty = true;
		strcpy(last_time_str_top, ""); // buoc ve lai gio sau khi xoa man hinh
	}

	if (taskmanager_dirty)
	{
		draw_chart_data(c[0].x, c[0].y, c[0].w, c[0].h, "CPU", chart_cpu, UI_CPU);
		draw_chart_data(c[1].x, c[1].y, c[1].w, c[1].h, "RAM", chart_ram, UI_RAM, 90, "BOOST NOW");
		draw_chart_data(c[2].x, c[2].y, c[2].w, c[2].h, "GPU", chart_gpu, UI_GPU);
		// "MEM" (3 ky tu, khong phai "VRAM" 4 ky tu): o khung ben phai (x~164) TFTdrawText dung
		// con tro X kieu uint8_t (toi da 255) - nhan dai hon 1 ky tu la du lam tran/wrap ve dau man hinh
		draw_chart_data(c[3].x, c[3].y, c[3].w, c[3].h, "MEM", chart_gpumem, UI_GPUMEM);
		draw_chart_data(c[4].x, c[4].y, c[4].w, c[4].h, "WIFI", chart_wifi, UI_WIFI);

		taskmanager_dirty = false;
	}

	MONITOR_STATUS();
	draw_taskmanager_clock();
}
// vi tri + kich thuoc 1 dong trong menu SETTING, tinh 1 lan dung chung cho ve khung
// va cho cap nhat khi doi lua chon (rowH/rowGap nho lai de FUNTION_MODE_COUNT dong
// deu vua man hinh, khong bi tran ra ngoai / bi den do ve qua vung hien thi)
const int16_t MENU_ROW_X = 55, MENU_ROW_W = 220, MENU_ROW_H = 28, MENU_ROW_GAP = 4, MENU_START_Y = 26;
void draw_menu_row(int8_t i, bool selected)
{
	const char *labels[FUNTION_MODE_COUNT] = {"PLAYER", "FUNTION", "MODE", "CLOCK", "COLOR", "TASK"};
	int16_t rowY = MENU_START_Y + i * (MENU_ROW_H + MENU_ROW_GAP);
	uint16_t bg = selected ? UI_PANEL_HI : UI_PANEL;
	uint16_t border = selected ? UI_ACCENT : UI_BORDER;
	uint16_t fg = selected ? UI_ACCENT : UI_TEXT_DIM;
	myTFT.TFTfillRoundRect(MENU_ROW_X, rowY, MENU_ROW_W, MENU_ROW_H, 4, bg);
	myTFT.TFTdrawRoundRect(MENU_ROW_X, rowY, MENU_ROW_W, MENU_ROW_H, 4, border);
	myTFT.TFTdrawText(MENU_ROW_X + 15, rowY + 7, (char *)labels[i], fg, bg, 2);
}
bool menu_needs_full_draw = true; // true khi vua vao menu SETTING (hoac quay lai tu CLOCK/COLOR) -> ve lai tat ca
void MONITOR_FUNTION()
{
	MONITOR_STATUS();

	// cuon vong: cuoi danh sach quay len dau va nguoc lai (thay vi dung khung o 2 dau khi quay nhanh).
	// dung % thay vi if de xu ly dung ca khi quay rat nhanh khien funtion_mode nhay qua nhieu buoc cung luc.
	funtion_mode = ((funtion_mode % FUNTION_MODE_COUNT) + FUNTION_MODE_COUNT) % FUNTION_MODE_COUNT;

	if (menu_needs_full_draw)
	{
		myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
		for (int i = 0; i < FUNTION_MODE_COUNT; i++)
		{
			draw_menu_row(i, i == funtion_mode);
		}
		last_funtion_mode = funtion_mode;
		menu_needs_full_draw = false;
	}
	else if (last_funtion_mode != funtion_mode)
	{
		// chi ve lai 2 dong bi anh huong (dong cu bo chon + dong moi duoc chon)
		// thay vi xoa toan bo man hinh -> chuyen doi giua cac muc muot, khong nhap nhay
		draw_menu_row(last_funtion_mode, false);
		draw_menu_row(funtion_mode, true);
		last_funtion_mode = funtion_mode;
	}
}
// man hinh dong ho: hien gio + ngay hien tai (nhan tu monitor.py qua serial, truong TIME/DATE)
// giao dien toi gian: chi 2 dong chu can giua, khong khung khong tieu de
void MONITOR_CLOCK()
{
	MONITOR_STATUS();

	if (last_show_clock != show_clock)
	{
		last_show_clock = show_clock;
		myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
		strcpy(last_time_str, "");
		strcpy(last_date_str, "");
		clock_dirty = true;
	}

	if (clock_dirty)
	{
		if (strcmp(last_time_str, current_time_str) != 0)
		{
			myTFT.TFTfillRect(0, 100, 320, 45, UI_BG);
			// can giua: size4, moi ky tu chiem size*(5+1)=24px. "HH:MM:SS" (8 ky tu) rong 192px
			// -> x=(320-192)/2=64. Dung size4 (thay vi 5) de vi tri ky tu cuoi cung (64+7*24=232)
			// nam an toan duoi nguong 255 cua con tro X kieu uint8_t trong TFTdrawText, tranh tran/wrap.
			myTFT.TFTdrawText(64, 108, current_time_str, UI_ACCENT, UI_BG, 4);
			strcpy(last_time_str, current_time_str);
		}
		if (strcmp(last_date_str, current_date_str) != 0)
		{
			myTFT.TFTfillRect(0, 155, 320, 25, UI_BG);
			// x=70 de can giua "DD/MM/YYYY" (10 ky tu, size 3 -> rong ~177px tren man 320px).
			// Khong duoc dat x qua ~90: TFTdrawText dung con tro X kieu uint8_t, neu x + do rong
			// vuot qua 255 no se tran so va ky tu cuoi nhay ve sat le trai man hinh.
			myTFT.TFTdrawText(70, 158, current_date_str, UI_TEXT_DIM, UI_BG, 3);
			strcpy(last_date_str, current_date_str);
		}
		clock_dirty = false;
	}
}
// man hinh chinh mau giao dien: dung encoder chon 1 trong cac mau nhan dien co san,
// nhan nut de ap dung va quay lai menu SETTING
void MONITOR_COLOR()
{
	MONITOR_STATUS();

	// cuon vong nhu menu SETTING; dung % de dung ca khi quay rat nhanh nhay qua nhieu buoc
	color_index = (int8_t)(((color_index % (int8_t)UI_ACCENT_PRESET_COUNT) + (int8_t)UI_ACCENT_PRESET_COUNT) % (int8_t)UI_ACCENT_PRESET_COUNT);

	if (last_show_color != show_color)
	{
		last_show_color = show_color;
		myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
		last_color_index = -1;
	}

	if (last_color_index != color_index)
	{
		last_color_index = color_index;

		myTFT.TFTfillRect(0, 40, 320, 130, UI_BG);

		const int16_t swW = 26, swGap = 6, startX = 12, swY = 55, swH = 26;
		for (uint8_t i = 0; i < UI_ACCENT_PRESET_COUNT; i++)
		{
			int16_t sx = startX + i * (swW + swGap);
			bool selected = (i == color_index);
			myTFT.TFTfillRoundRect(sx, swY, swW, swH, 4, UI_ACCENT_PRESETS[i]);
			myTFT.TFTdrawRoundRect(sx, swY, swW, swH, 4, selected ? UI_TEXT : UI_BORDER);
			if (selected)
				myTFT.TFTdrawRoundRect(sx - 2, swY - 2, swW + 4, swH + 4, 5, UI_TEXT);
		}

		myTFT.TFTdrawText(100, 110, (char *)UI_ACCENT_NAMES[color_index], UI_ACCENT_PRESETS[color_index], UI_BG, 3);
		myTFT.TFTdrawText(40, 145, (char *)"Nhan nut de ap dung", UI_TEXT_DIM, UI_BG, 1);
	}
}
void key_value_tang()
{
	switch (desktop_state)
	{
	case DESKTOP_HOME:
		// pin ben trai (battery1) gio lay tu pin laptop qua serial, encoder o HOME
		// chi con dung de chinh pin may (battery2) khi chua co cam bien that
		battery2++;
		break;
	case DESKTOP_SETING:
		if (show_color)
			color_index++;
		else
			funtion_mode++;
		break;
	default:
		break;
	}
}
void key_value_giam()
{
	switch (desktop_state)
	{
	case DESKTOP_HOME:
		battery2--;
		break;
	case DESKTOP_SETING:
		if (show_color)
			color_index--;
		else
			funtion_mode--;
		break;
	default:
		break;
	}
}
void encoder_isr(uint gpio, uint32_t events)
{
	// KHONG duoc goi printf() trong ISR (in qua USB CDC rat cham), se lam tre xu ly canh xung tiep theo
	absolute_time_t now = get_absolute_time();
	// debounce ~5ms de loc rung tiep diem co khi khi quay nhanh
	if (absolute_time_diff_us(last_time, now) < 5000)
		return;
	last_time = now;

	if (gpio_get(CLK) == 0)
		key_value_tang();
	else
		key_value_giam();
}

// nhan giu nut > 2s => chuyen tab (HOME <-> SETTING); nha nut som hon => nhan ngan (chon/xac nhan).
// Truoc day 2 loai thao tac nay dung chung mot dieu kien va co the cung fire trong 1 lan giu nut
// (vua chon/xac nhan, vua doi tab), khien man hinh ve chong len nhau khi chuyen tab ("chong cheo").
// Gio dung 1 co button_long_fired de dam bao MOI LAN NHAN chi sinh ra DUNG 1 HANH DONG.
void read_button()
{
	/*---------------UPDATE FIRMWARE-----------------------*/
	now_button_1 = gpio_get(button_1);
	if (now_button_1 == 0)
	{
		if (last_button_1 != now_button_1)
		{
			reset_usb_boot(0, 0);
			last_button_1 = now_button_1;
		}
	}
	/*-----------------------------------------------------*/

	now_button = gpio_get(button);

	if (last_button == 1 && now_button == 0)
	{
		// canh nhan xuong: bat dau dem thoi gian giu nut
		timer2 = to_ms_since_boot(get_absolute_time());
		button_long_fired = false;
	}

	if (now_button == 0 && !button_long_fired &&
		to_ms_since_boot(get_absolute_time()) - timer2 > 2000)
	{
		// giu du 2s: chuyen tab, danh dau da xu ly de khi tha nut khong con fire hanh dong chon/xac nhan nua
		display_number++;
		button_long_fired = true;
	}

	if (last_button == 0 && now_button == 1)
	{
		// canh tha nut: neu chua bi xu ly bang giu lau thi day la 1 lan nhan ngan -> chon/xac nhan
		if (!button_long_fired)
		{
			if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_TASK)
			{
				printf("PLG_>>>> back to TASK MANAGER (home)\n\r");
				display_number = 0; // man hinh chinh (HOME) chinh la Task Manager
			}
			else if (desktop_state == DESKTOP_SETING && show_clock)
			{
				printf("PLG_>>>> exit CLOCK, back to SETTING menu\n\r");
				show_clock = false;
				last_show_clock = false; // dong bo lai co, de lan sau vao CLOCK duoc xoa man hinh dung cach
				menu_needs_full_draw = true;
			}
			else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_ID)
			{
				printf("PLG_>>>> enter CLOCK\n\r");
				show_clock = true;
			}
			else if (desktop_state == DESKTOP_SETING && show_color)
			{
				printf("PLG_>>>> apply COLOR #%d, back to SETTING menu\n\r", color_index);
				UI_ACCENT = UI_ACCENT_PRESETS[color_index];
				save_color_to_flash(color_index); // luu lai, mat nguon van giu mau da chon
				show_color = false;
				last_show_color = false; // dong bo lai co, de lan sau vao COLOR duoc xoa man hinh dung cach
				// buoc ve lai toan bo cac man hinh dung UI_ACCENT
				menu_needs_full_draw = true;
				last_desktop = !desktop;
				last_status = !status;
				taskmanager_dirty = true;
			}
			else if (desktop_state == DESKTOP_SETING && funtion_mode == FUNTION_MODE_COLOR)
			{
				printf("PLG_>>>> enter COLOR\n\r");
				for (uint8_t i = 0; i < UI_ACCENT_PRESET_COUNT; i++)
				{
					if (UI_ACCENT_PRESETS[i] == UI_ACCENT)
					{
						color_index = i;
						break;
					}
				}
				show_color = true;
			}
		}
	}

	last_button = now_button;
}
void DISPLAY_ROLL()
{
	switch (display_number)
	{
	case 0:
		desktop_state = DESKTOP_HOME; // man hinh chinh = Task Manager
		break;
	case 1:
		desktop_state = DESKTOP_SETING;
		break;

	default:
		break;
	}
	if (last_display_number != display_number)
	{
		myTFT.TFTfillRect(0, 24, 320, 216, UI_BG);
		last_display_number = display_number;
		desktop = !last_desktop;

		if (display_number == 1)
		{
			funtion_mode = 0;
			menu_needs_full_draw = true; // buoc ve lai toan bo menu 1 lan khi vao SETTING
			show_clock = false;			 // luon vao menu truoc, khong vao thang man hinh dong ho
			show_color = false;			 // luon vao menu truoc, khong vao thang man hinh chon mau

			// xoa gio Task Manager o goc tren-phai khi roi HOME, tranh no bi dinh lai
			// (khong duoc xoa) tren cac man hinh khac nhu SETTING/CLOCK
			myTFT.TFTfillRect(188, 4, 65, 17, UI_BG);
			strcpy(last_time_str_top, "");
		}
	}
	if (display_number > 1)
	{
		display_number = 0;
	}
}
void setup()
{
	setup_pin();
	load_color_from_flash(); // khoi phuc mau da chon lan truoc (neu co)
	// -1 = "chua co du lieu that" (xem giai thich o PLG_setup.h canh khai bao chart_cpu...)
	memset(chart_cpu, -1, CHART_SAMPLES);
	memset(chart_ram, -1, CHART_SAMPLES);
	memset(chart_gpu, -1, CHART_SAMPLES);
	memset(chart_gpumem, -1, CHART_SAMPLES);
	memset(chart_wifi, -1, CHART_SAMPLES);
	myTFT.TFTsetRotation(myTFT.TFT_Degrees_270);
	MONITOR_BEGIN();
	printf("PLG_end setup\n\r");
}
uint32_t lastTime = 0;
uint32_t loopCount = 0;
void loop()
{
	// live_pico();
	loopCount++;

	read_button();
	read_taskmanager_serial();
	DISPLAY_ROLL();

	switch (desktop_state)
	{
	case DESKTOP_HOME:
		MONITOR_TASKMANAGER();
		break;
	case DESKTOP_SETING:
		if (show_clock)
			MONITOR_CLOCK();
		else if (show_color)
			MONITOR_COLOR();
		else
			MONITOR_FUNTION();
		break;
	default:
		break;
	}
	
	if (to_us_since_boot(get_absolute_time()) - lastTime >= 1000)
	{
		printf("PLG_end loop  %d \n\r",to_us_since_boot(get_absolute_time()) - lastTime);
		// printf(loopCount);

		loopCount = 0;
		lastTime =to_us_since_boot(get_absolute_time());
	}
	// MONITOR_FUNTION();

	// if (battery1 < 0)
	// {
	// 	battery1 = 0;
	// }
	// if (battery1 > 100)
	// {
	// 	battery1 = 100;
	// }
	// if (last_battery1 != battery1)
	// {
	// 	myTFT.TFTfillRect(60, 5, 40, 17, ST7789_WHITE);
	// 	last_battery1 = battery1;
	// }
	// char batteri1[3] = "";
	// itoa(battery1, batteri1, 10);
	// myTFT.TFTdrawText(60, 5, batteri1, ST7789_BLACK, ST7789_WHITE, 2);

	// myTFT.TFTdrawBitmap16Data(60, 100,logo, 120, 100);
	// myTFT.TFTdrawRectWH(65, 65, 20, 20, ST7789_RED);
	// if (myTFT.TFTfillRectBuffer(105, 75, 20, 20, ST7789_YELLOW) != 0) // uses spiwrite
	// {
	// 	printf("Error Test902 1: Error in the TFTfillRectangle function\r\n");
	// }
	// drawLogoFull(180, 0, 20, 30, PLG_ICON_DISCONNECTED);

	// myTFT.TFTfillRect(180, 5, 20, 20, 0x00ff);
	// myTFT.TFTdrawRoundRect(15, 160, 50, 50, 5, ST7789_CYAN);
	// myTFT.TFTfillRoundRect(70, 160, 50, 50, 10, ST7789_BROWN);

	// myTFT.TFTdrawText(40, 40, "Rotate 0", ST7789_GREEN, ST7789_BLACK, 2);
	// myTFT.TFTfillScreen(ST7789_BLACK);

	// myTFT.TFTdrawPixel(85, 55, ST7789_WHITE);
	// myTFT.TFTdrawPixel(87, 59, ST7789_WHITE);
}
