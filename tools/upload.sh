#!/bin/bash
#
# Facilitator batch-flash tool — flash many ESP32 boards in a loop
# (plug board → Enter → flash → swap board → repeat).
#
# ⚠️  Personal setup, macOS only — NOT the standard flashing path:
#   - Edit CMD below to point at YOUR esptool (path is machine-specific).
#   - Requires a MERGED firmware.bin (bootloader+partitions+app) in the
#     current directory — this repo does not ship one.
#   - Serial-port detection uses macOS /dev/cu.* names; won't work on
#     Linux/Windows.
#
# For normal single-board flashing, follow README.md (arduino-cli upload).

CMD="/Users/kamfaichan/Library/Arduino15/packages/esp32/tools/esptool_py/5.3.0/esptool"

while true; do
    echo "========================================"
    echo "【ESP32 批量穩定燒錄】"
    echo "請接上新的 ESP32，然後按 [Enter] 開始燒錄..."
    read -r
    
    echo "🔄 正在重置 Mac USB 序列埠快取..."
    # 強制關閉可能殘留的序列埠連線
    pkill -f esptool 2>/dev/null
    exec 3>&- 2>/dev/null
    
    # 使用 Mac 內建工具踢掉舊的 tty 連線
    for f in /dev/cu.usbserial* /dev/cu.usbmodem*; do
        if [ -e "$f" ]; then
            screen -X -S "$f" quit 2>/dev/null
            stty -f "$f" hupcl 2>/dev/null
        fi
    done
    
    # 稍微等 1 秒讓系統反應
    sleep 1
    
    PORT=$(ls /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/cu.usbmodem* 2>/dev/null | head -n 1)
    
    if [ -z "$PORT" ]; then
        echo "❌ 錯誤：找不到任何 ESP32 開發板！請確認板子已插好。"
        continue
    fi
    
    echo "偵測到開發板：$PORT"
    echo "正在以 115200 穩定速度燒錄中，請稍候..."
    
    "$CMD" --chip esp32 --port "$PORT" --baud 115200 write-flash -z 0x0 firmware.bin
    
    if [ $? -eq 0 ]; then
        echo "✅ 燒錄成功！請拔除開發板，換下一隻。"
    else
        echo "❌ 燒錄失敗！"
    fi
done
