# Bản 14 — texture, sprite và pipeline màu

Ảnh test BIOS ở bản 13 đã có hình thoi đỏ/cam của Sony Computer Entertainment,
nhưng dừng tại `Unsupported GP0 word 0x2c808080`. Bản này bổ sung khả năng
render texture thay vì đặt sẵn chữ SONY hay sửa ảnh chụp.

## Các phần được thêm

| Phần | Hành vi |
|---|---|
| Polygon GP0 20h–3Fh | Tam giác/tứ giác, flat/Gouraud, texture/raw/modulation và opaque/semi-transparent |
| Packet parser | Tự tính chiều dài theo flags; buffer 12 word, giữ packet qua nhiều DMA node |
| Texture pages | Trang X theo bước 64 halfword, Y theo bước 256 dòng; tọa độ UV affine |
| 4-bit / 8-bit | Lấy index nibble/byte từ VRAM rồi tra CLUT theo tọa độ của packet |
| 15-bit direct | Đọc RGB15, texel 0000h trong suốt; 8000h là màu đen có STP |
| Texture window E2h | Thay bit UV theo mask/offset, wrap trong 256x256 |
| Modulation | Màu 80h trung tính, 40h tối đi, FFh sáng hơn với saturation |
| Semi-transparency | B/2+F/2, B+F, B-F, B+F/4; texture chỉ blend khi texel có STP |
| Dithering | Ma trận 4x4 cho Gouraud/modulated polygons trước khi lượng tử RGB15 |
| Rectangle GP0 60h–7Fh | Biến kích thước, 1x1, 8x8, 16x16; texture, raw, modulation và blending |
| Sprite flip | E1 bit12/13 giảm U/V; clipping vẫn giữ đúng pha UV |
| Mask/clipping | Áp dụng drawing area, signed offset và E6 check/force cho đầu ra mới |
| GPUSTAT | GP1(08h) cập nhật các bit mode/depth/interlace/PAL/hres |
| Diagnostic | Thêm GPUSTAT, E1/E2, display registers, packet count, polygon/sprite counters và 64 word GP0 gần nhất |
| UI | Nút Texture demo, bộ đếm GPU và hai thanh ghi mỗi dòng cho debugger hẹp |

## Demo không cần BIOS
Bấm **Texture demo**. Một chương trình MIPS tự ghi A0 để upload checkerboard
8x8 vào VRAM, gửi 2C để vẽ hai tứ giác (trung tính/tối), gửi sprite 2D,
rectangle màu và rectangle bán trong suốt. Toàn bộ demo đi qua CPU -> MMIO GP0.
Không dùng ảnh/logo tải sẵn. Có thể nạp lại BIOS để trở về máy mới.

## Kiểm thử
12 bộ test đều PASS với GCC C++17 -Wall -Wextra -Werror,
AddressSanitizer/UndefinedBehaviorSanitizer; leak detection bị tắt trên host.

`texture_tests` mới kiểm tra:
- Chính command word `2C808080`, quad neutral và TPage/status.
- Framing của cả 32 opcode polygon và 32 opcode rectangle, command kế tiếp.
- 4-bit nibble, 8-bit byte, CLUT x/y/index0/index255, direct color.
- Texture zero/8000h, raw/modulation/saturation, window XY, page Y/wrap X.
- Affine UV/color interpolation, đảo winding phải giữ cả màu/UV.
- UV flip, clipping/offset, E6 mask và bốn chế độ blend/STP gate/saturation.
- Dither matrix, raw/flat/rectangle bypass, kích thước rectangle và zero width.
- Packet 12 word chia qua hai node DMA2, reset packet dở, mode GPUSTAT.
- Log chứa thanh ghi GPU và demo MIPS texture thực thi đúng pixel/counter.

Các test cũ về CPU, BIOS trace, GPU upload/readback, DMA, IRQ, SPU, timers,
CD-ROM và SIO được chạy lại. Chưa build/chạy UI Win32 tại host Linux.

## Giới hạn còn lại
- Chưa chạy BIOS Sony ở máy phát triển; cần kiểm tra bản mới với BIOS của bạn.
  Chưa xác nhận chữ SONY, menu BIOS hoặc game thương mại trên bản 14.
- Rasterizer vẫn là xấp xỉ chức năng: chưa khớp fixed-point/rounding/timing phần cứng.
- Chưa có texture cache/CLUT cache thật; đọc VRAM ngay khi vẽ. Feedback khi texture
  chồng đích có thể khác phần cứng. Reserved texture depth 3 tạm đọc như direct RGB15.
- Chưa mô phỏng GP1(09h) texture-disable, draw-to-display/interlace field restrictions.
- Line/polyline GP0 40h–5Fh, GTE, MDEC, ADPCM/audio output vẫn chưa hỗ trợ.
- VBlank/HBlank/video clocks chưa có; `VSync: timeout` vẫn có thể lặp.
- CD-ROM vẫn ISO2048 subset. Tiếp tục test BIOS trước khi chuyển sang CD/game.

## Test với Visual Studio
Giải nén vào thư mục mới -> File/Open/Folder -> ps1-lab -> build ps1_ui.
Kiểm tra tiêu đề **PS1 Lab v14 - Textures and Sprites**.
Thử Texture demo, sau đó Nap BIOS -> Run. Không cần Mount ISO ở bước này.
Nếu dừng/chờ: Pause -> Save log; gửi log mới và ảnh. Log giờ giữ thêm luồng GP0
để phân biệt command thật và packet bị lệch, không cần gửi file BIOS.

## Tham khảo
Mã tự viết cho dự án, không sao chép lõi giả lập khác. PSX-SPX:
- https://psx-spx.consoledev.net/dev/ps1/gpu/render-polygon-commands/
- https://psx-spx.consoledev.net/dev/ps1/gpu/render-rectangle-commands/
- https://psx-spx.consoledev.net/dev/ps1/gpu/rendering-attributes/
- https://psx-spx.consoledev.net/dev/ps1/gpu/misc/
- https://psx-spx.consoledev.net/graphicsprocessingunitgpu/

Các tài liệu *_UPDATE trước bản 14 ghi trạng thái lịch sử; giới hạn texture
ở các bản cũ đã được thay đổi bởi tài liệu này.
