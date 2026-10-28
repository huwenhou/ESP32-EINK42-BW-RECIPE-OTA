// =====================================================================
//  4.2" e-paper + ESP32 (Waveshare 驱动板)
//  · 按星期自动显示当天食谱（菜单在 menu.h）
//  · 每 10 分钟检查 GitHub Pages 上的新固件，有就自动 OTA 升级
// =====================================================================
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <SPI.h>
#include <time.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "version.h"
#include "menu.h"
#include "icons.h"

// ---------- 固件发布地址（GitHub Pages）----------
#define FW_BASE_URL "https://huwenhou.github.io/ESP32-EINK42-BW-RECIPE-OTA"
const unsigned long OTA_INTERVAL_MS  = 10UL * 60UL * 1000UL;  // 10 分钟查一次新版本
const unsigned long TIME_INTERVAL_MS = 60UL * 1000UL;         // 每分钟看一次是否跨天

// ---------- Waveshare ESP32 驱动板引脚 ----------
#define EPD_BUSY 25
#define EPD_RST  26
#define EPD_DC   27
#define EPD_CS   15
#define EPD_SCK  13
#define EPD_MOSI 14
#define AP_NAME "Robbie's Recipe Board"
#define BOOT_BTN 0   // 运行中长按 5 秒 = 清除 Wi-Fi 设置

// 4.2" V2 屏用 GDEY042T81；老版本改成 GxEPD2_420
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>
  display(GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8f;

// 中文字体（文泉驿点阵，GB2312 字符集）
#define FONT_S u8g2_font_wqy12_t_gb2312   // 小字：千卡、份量
#define FONT_M u8g2_font_wqy16_t_gb2312   // 菜名、份量、底栏（与标题同字号）
#define FONT_L u8g2_font_wqy16_t_gb2312   // 日期、午饭/晚饭

#define BLACK GxEPD_BLACK
#define WHITE GxEPD_WHITE

unsigned long lastOtaCheck = 0, lastTimeCheck = 0;
int shownYday = -1;   // 当前屏幕显示的是一年中的第几天

// ---------------------------------------------------------------------
//  绘图小工具
// ---------------------------------------------------------------------
void textAt(int x, int y, const char* s, const uint8_t* font, uint16_t fg = BLACK) {
  u8f.setFont(font);
  u8f.setFontMode(1);   // setFont 会把背景重置为不透明，导致加粗的第二遍把第一遍盖掉
  u8f.setForegroundColor(fg);
  u8f.setBackgroundColor(fg == BLACK ? WHITE : BLACK);
  // 点阵中文字体没有粗体：把文字错开 1 像素多画几遍形成"加粗"
  // 16 号字横竖各加 1 像素（2×2），12 号小字只横向加 1 像素，避免糊成一团
  bool big = (font != FONT_S);
  for (int dx = 0; dx <= 1; dx++)
    for (int dy = 0; dy <= (big ? 1 : 0); dy++) {
      u8f.setCursor(x + dx, y + dy);
      u8f.print(s);
    }
}
int textW(const char* s, const uint8_t* font) {
  u8f.setFont(font);
  return u8f.getUTF8Width(s);
}
void textRight(int xRight, int y, const char* s, const uint8_t* font, uint16_t fg = BLACK) {
  textAt(xRight - textW(s, font), y, s, font, fg);
}
void textCenter(int xc, int y, const char* s, const uint8_t* font) {
  textAt(xc - textW(s, font) / 2, y, s, font);
}
void thickRect(int x, int y, int w, int h) {
  display.drawRect(x, y, w, h, BLACK);
  display.drawRect(x + 1, y + 1, w - 2, h - 2, BLACK);
}
void hLine2(int x, int y, int w) {
  display.drawFastHLine(x, y, w, BLACK);
  display.drawFastHLine(x, y + 1, w, BLACK);
}
// 从右往左画 "标签 □"，返回下一个元素可用的右边界
int checkBoxLeft(int xRight, int baseline, const char* label) {
  const int sz = 12;
  display.drawRect(xRight - sz, baseline - sz + 1, sz, sz, BLACK);
  display.drawRect(xRight - sz + 1, baseline - sz + 2, sz - 2, sz - 2, BLACK);
  int x = xRight - sz - 4;
  textRight(x, baseline, label, FONT_M);
  return x - textW(label, FONT_M) - 10;
}

// ---------------------------------------------------------------------
//  一个餐盒：标题栏 + 4 道菜
// ---------------------------------------------------------------------
void drawMealBox(int x, int y, int w, int h, const char* title, const char* note,
                 const Dish* dishes, bool relax) {
  thickRect(x, y, w, h);
  textAt(x + 8, y + 21, title, FONT_L);
  textRight(x + w - 8, y + 20, note, FONT_S);
  hLine2(x, y + 28, w);

  if (relax) {
    int cx = x + w / 2;
    textCenter(cx, y + 90, RELAX_TITLE, FONT_L);
    textCenter(cx, y + 124, RELAX_LINE1, FONT_M);
    textCenter(cx, y + 146, RELAX_LINE2, FONT_M);
    return;
  }
  int base = y + 60;
  for (int i = 0; i < 4; i++) {
    if (dishes[i].name[0] == '\0') continue;
    drawIcon(display, iconFor(dishes[i].name), x + 8, base - 15, BLACK, WHITE);
    textAt(x + 32, base, dishes[i].name, FONT_M);
    textRight(x + w - 10, base, dishes[i].qty, FONT_M);
    base += 34;
  }
}

// ---------------------------------------------------------------------
//  整屏：当天食谱
// ---------------------------------------------------------------------
void drawMenuPage(const struct tm& t) {
  const DayMenu& m = MENU[t.tm_wday];
  char date[24];
  snprintf(date, sizeof(date), "%d月%d日", t.tm_mon + 1, t.tm_mday);

  // 先整屏刷成黑、再刷成白，清掉上一屏的残影，背景更干净
  display.clearScreen(0x00);
  display.clearScreen(0xFF);

  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(WHITE);

    // 顶栏
    display.fillRect(0, 0, 400, 36, BLACK);
    textAt(10, 25, date, FONT_L, WHITE);
    textAt(10 + textW(date, FONT_L) + 6, 25, WEEKDAY_CN[t.tm_wday], FONT_S, WHITE);
    const char* salt = "限盐5克";
    int sx = 390 - textW(salt, FONT_S);
    textAt(sx, 24, salt, FONT_S, WHITE);
    display.fillCircle(sx - 7, 19, 2, WHITE);
    textRight(sx - 13, 24, m.headerRight, FONT_S, WHITE);

    // 两个餐盒
    drawMealBox(8,   44, 188, 206, "午饭", m.lunchNote,  m.lunch,  m.relaxDay);
    drawMealBox(204, 44, 188, 206, "晚饭", m.dinnerNote, m.dinner, false);

    // 底栏
    hLine2(0, 258, 400);
    textAt(10, 283, "运动:", FONT_M);
    textAt(10 + textW("运动:", FONT_M) + 4, 283, m.sport, FONT_M);
    if (m.relaxDay) {
      int r = checkBoxLeft(390, 283, "血压");
      checkBoxLeft(r, 283, "称体重");
    } else {
      int r = checkBoxLeft(390, 283, "晚");
      r = checkBoxLeft(r, 283, "早");
      textRight(r, 283, "血压", FONT_M);
    }

    // 右下角极小的版本号，用来确认升级是否成功
    display.setFont(nullptr);
    display.setTextColor(BLACK);
    display.setTextSize(1);
    char ver[12];
    snprintf(ver, sizeof(ver), "v%d", FW_VERSION);
    for (int dx = 0; dx <= 1; dx++) {
      display.setCursor(400 - 6 * strlen(ver) - 3 + dx, 291);
      display.print(ver);
    }
  } while (display.nextPage());
  display.hibernate();
}

void drawStatus(const char* line1, const char* line2) {
  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(WHITE);
    textCenter(200, 135, line1, FONT_L);
    textCenter(200, 165, line2, FONT_M);
  } while (display.nextPage());
  display.hibernate();
}

// ---------------------------------------------------------------------
//  时间（北京时间，阿里云 NTP）
// ---------------------------------------------------------------------
bool getNow(struct tm& t) {
  if (!getLocalTime(&t, 5000)) return false;
  return t.tm_year > (2024 - 1900);
}

void refreshIfNewDay(bool force = false) {
  struct tm t;
  if (!getNow(t)) {
    if (shownYday == -1) drawStatus("正在同步时间", "请确认 Wi-Fi 可以上网");
    shownYday = -2;
    return;
  }
  if (force || t.tm_yday != shownYday) {
    drawMenuPage(t);
    shownYday = t.tm_yday;
  }
}

// ---------------------------------------------------------------------
//  OTA
// ---------------------------------------------------------------------
long fetchRemoteVersion() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // 换自定义域名后旧地址会 301 跳转
  http.setTimeout(15000);
  String url = String(FW_BASE_URL) + "/version.txt?t=" + String(millis());
  if (!http.begin(client, url)) return -1;
  long v = -1;
  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    body.trim();
    v = body.toInt();
  } else {
    Serial.printf("version.txt HTTP %d\n", code);
  }
  http.end();
  return v;
}

void checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    return;
  }
  long remote = fetchRemoteVersion();
  Serial.printf("local v%d, remote v%ld\n", FW_VERSION, remote);
  if (remote <= FW_VERSION) return;

  Serial.println("New firmware found, updating...");
  WiFiClientSecure client;
  client.setInsecure();
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return r =
      httpUpdate.update(client, String(FW_BASE_URL) + "/firmware.bin");
  if (r == HTTP_UPDATE_FAILED) {
    Serial.printf("Update failed (%d): %s\n",
                  httpUpdate.getLastError(),
                  httpUpdate.getLastErrorString().c_str());
  }
}

void checkResetButton() {
  // 运行中长按 BOOT 5 秒：清除 Wi-Fi 并重启进入配网
  // （不能开机时按，否则 ESP32 会进入下载模式）
  static unsigned long pressedAt = 0;
  if (digitalRead(BOOT_BTN) == LOW) {
    if (pressedAt == 0) pressedAt = millis();
    if (millis() - pressedAt >= 5000) {
      WiFiManager wm;
      wm.resetSettings();
      drawStatus("Wi-Fi 已清除", "正在重启...");
      delay(1000);
      ESP.restart();
    }
  } else {
    pressedAt = 0;
  }
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BTN, INPUT_PULLUP);

  SPI.end();
  SPI.begin(EPD_SCK, 12, EPD_MOSI, EPD_CS);
  display.init(115200, true, 2, false);
  u8f.begin(display);
  u8f.setFontMode(1);
  u8f.setFontDirection(0);

  WiFiManager wm;
  wm.setAPCallback([](WiFiManager*) {
    drawStatus("请连接热点 " AP_NAME, "在弹出的页面里设置 Wi-Fi");
  });
  wm.setConfigPortalTimeout(300);
  if (!wm.autoConnect(AP_NAME)) {
    drawStatus("Wi-Fi 连接失败", "正在重启...");
    delay(3000);
    ESP.restart();
  }

  configTzTime("CST-8", "ntp.aliyun.com", "ntp.tencent.com", "cn.pool.ntp.org");

  checkForUpdate();          // 有新版本会直接升级并重启
  refreshIfNewDay(true);
  lastOtaCheck = lastTimeCheck = millis();
}

void loop() {
  checkResetButton();
  unsigned long now = millis();
  if (now - lastTimeCheck >= TIME_INTERVAL_MS) {
    lastTimeCheck = now;
    refreshIfNewDay();
  }
  if (now - lastOtaCheck >= OTA_INTERVAL_MS) {
    lastOtaCheck = now;
    checkForUpdate();
  }
  delay(100);
}
