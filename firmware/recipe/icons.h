// =====================================================================
//  菜品极简图标（18×18，线条 2 像素）
//  根据菜名里的关键词自动选图标，改菜单时不用管这里
// =====================================================================
#pragma once
#include <Adafruit_GFX.h>
#include <string.h>
#include <math.h>

enum IconType {
  IC_RICE, IC_SOUP, IC_NOODLE, IC_FISH, IC_EGG, IC_CHICKEN, IC_MEAT,
  IC_SHRIMP, IC_LEAF, IC_BROCCOLI, IC_CORN, IC_POTATO, IC_CUCUMBER,
  IC_MUSHROOM, IC_TOFU, IC_CUP
};

// 关键词按优先级匹配：先命中的先用（"鸡蛋羹"先匹配"蛋"，不会被当成鸡）
struct IconRule { const char* kw; IconType t; };
static const IconRule ICON_RULES[] = {
  {"酸奶", IC_CUP},   {"汤", IC_SOUP},     {"粥", IC_SOUP},
  {"虾", IC_SHRIMP},  {"鱼", IC_FISH},     {"蛋", IC_EGG},
  {"鸡", IC_CHICKEN}, {"牛", IC_MEAT},     {"肉", IC_MEAT},
  {"西兰花", IC_BROCCOLI}, {"玉米", IC_CORN}, {"薯", IC_POTATO},
  {"黄瓜", IC_CUCUMBER}, {"西葫芦", IC_CUCUMBER},
  {"菇", IC_MUSHROOM}, {"木耳", IC_MUSHROOM}, {"豆腐", IC_TOFU},
  {"面", IC_NOODLE},  {"饭", IC_RICE},
};

inline IconType iconFor(const char* name) {
  for (auto& r : ICON_RULES)
    if (strstr(name, r.kw)) return r.t;
  return IC_LEAF;   // 其余默认当作青菜
}

// ---------- 基础形状 ----------
// 碗：下半圆实心，碗口一条横线
inline void bowl(Adafruit_GFX& g, int x, int y, uint16_t c) {
  const int cx = x + 9, cy = y + 9, r = 8;
  g.fillRect(x, cy, 19, 2, c);
  for (int dy = 0; dy <= r; dy++) {
    int w = (int)sqrtf((float)(r * r - dy * dy));
    g.drawFastHLine(cx - w, cy + dy, 2 * w + 1, c);
  }
}
// 粗线
inline void line2(Adafruit_GFX& g, int x0, int y0, int x1, int y1, uint16_t c) {
  g.drawLine(x0, y0, x1, y1, c);
  g.drawLine(x0 + 1, y0, x1 + 1, y1, c);
}
// 一缕热气（竖向小波浪）
inline void steam(Adafruit_GFX& g, int x, int y, uint16_t c) {
  g.drawLine(x, y, x + 1, y + 2, c);
  g.drawLine(x + 1, y + 2, x, y + 4, c);
  g.drawLine(x, y + 4, x + 1, y + 6, c);
}

// ---------- 画图标：(x,y) 为左上角 ----------
inline void drawIcon(Adafruit_GFX& g, IconType t, int x, int y, uint16_t fg, uint16_t bg) {
  switch (t) {
    case IC_RICE:      // 一碗米饭：碗 + 上面鼓起的饭
      g.fillCircle(x + 5, y + 8, 3, fg);
      g.fillCircle(x + 9, y + 6, 4, fg);
      g.fillCircle(x + 13, y + 8, 3, fg);
      g.drawPixel(x + 8, y + 5, bg); g.drawPixel(x + 11, y + 7, bg);
      g.drawPixel(x + 5, y + 8, bg);
      bowl(g, x, y + 1, fg);
      break;

    case IC_SOUP:      // 一碗汤：碗 + 三缕热气
      steam(g, x + 4, y, fg); steam(g, x + 9, y - 1, fg); steam(g, x + 14, y, fg);
      bowl(g, x, y + 1, fg);
      break;

    case IC_NOODLE:    // 面：碗 + 一双筷子
      line2(g, x + 6, y + 9, x + 15, y, fg);
      line2(g, x + 9, y + 9, x + 17, y + 2, fg);
      bowl(g, x, y + 1, fg);
      break;

    case IC_FISH:      // 鱼：身子 + 尾巴 + 眼睛
      g.fillRoundRect(x, y + 4, 13, 10, 5, fg);
      g.fillTriangle(x + 11, y + 9, x + 17, y + 3, x + 17, y + 15, fg);
      g.fillCircle(x + 4, y + 8, 1, bg);
      break;

    case IC_EGG:       // 切开的鸡蛋：外圈 + 蛋黄
      g.fillCircle(x + 9, y + 9, 8, fg);
      g.fillCircle(x + 9, y + 9, 6, bg);
      g.fillCircle(x + 9, y + 9, 3, fg);
      break;

    case IC_CHICKEN:   // 鸡腿：肉 + 骨头
      g.fillCircle(x + 11, y + 6, 6, fg);
      g.fillTriangle(x + 6, y + 8, x + 11, y + 12, x + 4, y + 13, fg);
      line2(g, x + 5, y + 12, x + 2, y + 15, fg);
      g.fillCircle(x + 1, y + 15, 2, fg);
      g.fillCircle(x + 3, y + 17, 2, fg);
      break;

    case IC_MEAT:      // 牛肉：一块肉排 + 骨眼
      g.fillRoundRect(x, y + 3, 18, 13, 5, fg);
      g.fillCircle(x + 12, y + 9, 3, bg);
      g.fillCircle(x + 12, y + 9, 1, fg);
      break;

    case IC_SHRIMP:    // 虾：C 形身子 + 尾巴
      g.fillCircle(x + 9, y + 9, 8, fg);
      g.fillCircle(x + 9, y + 9, 4, bg);
      g.fillRect(x + 9, y + 9, 10, 10, bg);
      g.fillTriangle(x + 9, y + 13, x + 15, y + 11, x + 15, y + 17, fg);
      g.drawLine(x + 3, y + 5, x + 6, y + 7, bg);    // 节
      g.drawLine(x + 9, y + 1, x + 9, y + 4, bg);
      break;

    case IC_LEAF:      // 青菜：一片叶子 + 叶脉
      g.fillTriangle(x + 2, y + 16, x + 16, y + 2, x + 3, y + 4, fg);
      g.fillTriangle(x + 2, y + 16, x + 16, y + 2, x + 14, y + 15, fg);
      g.drawLine(x + 3, y + 15, x + 15, y + 3, bg);
      line2(g, x, y + 17, x + 3, y + 14, fg);
      break;

    case IC_BROCCOLI:  // 西兰花：三朵花球 + 梗
      g.fillCircle(x + 4, y + 7, 4, fg);
      g.fillCircle(x + 9, y + 5, 5, fg);
      g.fillCircle(x + 14, y + 7, 4, fg);
      g.fillRect(x + 7, y + 9, 5, 9, fg);
      g.drawPixel(x + 9, y + 12, bg);
      break;

    case IC_CORN:      // 玉米：棒子 + 粒 + 两片叶
      g.fillRoundRect(x + 5, y, 8, 15, 4, fg);
      for (int r = 0; r < 4; r++) {
        g.drawPixel(x + 7, y + 3 + r * 3, bg);
        g.drawPixel(x + 10, y + 3 + r * 3, bg);
      }
      line2(g, x + 2, y + 9, x + 8, y + 17, fg);
      line2(g, x + 15, y + 9, x + 9, y + 17, fg);
      break;

    case IC_POTATO:    // 红薯：两头尖的椭圆 + 芽眼
      g.fillRoundRect(x + 2, y + 4, 14, 11, 5, fg);
      g.fillTriangle(x, y + 9, x + 4, y + 5, x + 4, y + 13, fg);
      g.fillTriangle(x + 18, y + 9, x + 14, y + 5, x + 14, y + 13, fg);
      g.drawPixel(x + 6, y + 8, bg); g.drawPixel(x + 11, y + 10, bg);
      g.drawPixel(x + 9, y + 7, bg);
      break;

    case IC_CUCUMBER:  // 黄瓜/西葫芦：长条 + 小刺
      g.fillRoundRect(x, y + 6, 18, 7, 3, fg);
      g.drawPixel(x + 4, y + 8, bg); g.drawPixel(x + 9, y + 10, bg);
      g.drawPixel(x + 13, y + 8, bg);
      g.fillRect(x + 16, y + 4, 2, 3, fg);
      break;

    case IC_MUSHROOM:  // 蘑菇：伞盖 + 梗
      for (int dy = 0; dy <= 8; dy++) {
        int w = (int)sqrtf((float)(64 - dy * dy));
        g.drawFastHLine(x + 9 - w, y + 9 - dy, 2 * w + 1, fg);
      }
      g.fillRect(x + 6, y + 10, 6, 8, fg);
      g.drawPixel(x + 5, y + 5, bg); g.drawPixel(x + 12, y + 4, bg);
      break;

    case IC_TOFU:      // 豆腐：一块立方体
      g.fillRect(x, y + 6, 12, 12, fg);                         // 正面
      g.fillTriangle(x, y + 6, x + 6, y, x + 12, y + 6, fg);    // 顶面
      g.fillTriangle(x + 6, y, x + 18, y, x + 12, y + 6, fg);
      g.fillTriangle(x + 12, y + 6, x + 18, y, x + 18, y + 12, fg);  // 侧面
      g.fillTriangle(x + 12, y + 6, x + 18, y + 12, x + 12, y + 17, fg);
      g.drawLine(x, y + 6, x + 12, y + 6, bg);                  // 棱线
      g.drawLine(x + 12, y + 6, x + 12, y + 17, bg);
      g.drawLine(x + 12, y + 6, x + 18, y, bg);
      break;

    case IC_CUP:       // 酸奶杯：杯身 + 杯盖
      g.fillRect(x + 2, y + 2, 14, 3, fg);
      g.fillTriangle(x + 3, y + 5, x + 15, y + 5, x + 13, y + 17, fg);
      g.fillTriangle(x + 3, y + 5, x + 13, y + 17, x + 5, y + 17, fg);
      g.drawFastHLine(x + 5, y + 10, 9, bg);
      break;
  }
}
