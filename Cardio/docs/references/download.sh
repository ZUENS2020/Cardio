#!/usr/bin/env bash
# 重建 Cardio 参考资料库（这些 PDF 不入 Git，见 REFERENCES.md）。
# 用法：在本目录运行  bash download.sh
set -u
cd "$(dirname "$0")"
mkdir -p datasheets schematics
UA="Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 Chrome/124 Safari/537.36"

dl() { # url outfile
  curl -fsSL --max-time 60 -A "$UA" -e "https://www.google.com/" "$1" -o "$2" 2>/dev/null
  if [ -f "$2" ] && head -c4 "$2" | grep -q "%PDF"; then
    printf "  ✓ %-46s %s\n" "$2" "$(du -h "$2" | cut -f1)"
  else printf "  ✗ %-46s (失败，按 REFERENCES.md 链接手动下)\n" "$2"; rm -f "$2"; fi
}

M5="https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com"

echo "== 数据手册 =="
dl "https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf"                  datasheets/ESP32-S3_datasheet.pdf
dl "https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf" datasheets/ESP32-S3_TRM.pdf
dl "https://www.ti.com/lit/ds/symlink/tca8418.pdf"                                                          datasheets/TCA8418_keyboard_controller.pdf
dl "http://www.everest-semi.com/pdf/ES8311%20PB.pdf"                                                        datasheets/ES8311_codec.pdf
dl "https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/imx-processors/52419/1/WM8960.pdf"          datasheets/WM8960_codec.pdf

echo "== 原理图 =="
dl "$M5/1178/Sch_M5CardputerAdv_v1.0_2025_06_20_17_19_58.pdf" schematics/Sch_M5CardputerAdv_v1.0.pdf
dl "$M5/1150/Sch_StampS3_v0.3.3.pdf"                          schematics/Sch_StampS3_v0.3.3.pdf
dl "$M5/1178/K132-Adv-cardputer-ADV.pdf"                      schematics/M5_CardputerAdv_K132_overview.pdf
# WM8960 Audio Board 小板原理图（Waveshare wiki File: 页内的 PDF）
dl "https://www.waveshare.com/w/upload/b/b6/WM8960_Audio_Board_Schematic..pdf" schematics/WM8960_Audio_Board_Schematic.pdf

echo
echo "完成。被站点拦截的（ES8311 全本 / NS4150B）请按 REFERENCES.md 链接手动下载。"
