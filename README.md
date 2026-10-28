# 4.2" 黑白墨水屏 · 每日食谱

ESP32 驱动板 + 4.2 寸黑白屏（GDEY042T81）。按星期自动显示当天的午饭、晚饭、运动和血压打卡，每天零点换页。

## 刷机
电脑 Chrome 打开 https://huwenhou.github.io/ESP32-EINK42-BW-RECIPE-OTA ，点安装，选驱动板串口。
之后用手机连热点 **Robbie's Recipe Board** 配 Wi-Fi。

## 目录
```
firmware/recipe/recipe.ino  主程序（排版、联网、OTA）
firmware/recipe/menu.h      一周食谱数据（改菜单只改这里）
firmware/recipe/icons.h     菜品图标（按菜名关键词自动匹配）
web/index.html              网页刷机页
.github/workflows/build.yml 编译 → 发布到 GitHub Pages
```

## 更新
推送到 main → Actions 自动编译发布 → 板子每 10 分钟检查一次新版本并自动升级。
Partition Scheme：**Minimal SPIFFS (1.9MB APP with OTA)**。
