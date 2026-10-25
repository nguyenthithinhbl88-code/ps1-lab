# Bản 3 — GPU/VRAM demo

## Mở trong Visual Studio
1. Giải nén vào thư mục mới, mở thư mục ps1-lab.
2. Chọn ps1_ui.exe, build và F5.
3. Bấm **GPU demo**. Khung bên phải sẽ hiển thị nền tối, ba cột màu đỏ/xanh lá/xanh dương và vạch trắng phía dưới.

Đây là output thực từ chương trình MIPS: CPU thực thi LUI/ORI/SW để gửi gói lệnh đến GP0. UI đọc kết quả từ VRAM. Demo không sử dụng BIOS hoặc game.

## Phần mới
- GPU VRAM 1 MiB, 1024x512 pixel 16-bit.
- MMIO word GP0 ở 1F801810, GP1/GPUSTAT ở 1F801814, dùng được qua các alias bus đang hỗ trợ.
- GP0(00/01): NOP / cache-clear no-op do chưa có texture cache.
- GP0(02): fill VRAM, RGB24 -> RGB15, căn ngang 16 pixel, wrap tọa độ, chiều cao mask 9 bit.
- GP0(A0): CPU upload VRAM, pixel đóng gói 2 halfword/word, xử lý số pixel lẻ và wrap.
- GP1(00/01/02/03/04): reset control, reset parser, acknowledge IRQ flag, display-disable bit, DMA-direction bits. Không có DMA engine hay IRQ source.
- UI Win32 dùng GDI hiển thị vùng VRAM (0,0)-(319,239), phóng 2 lần. Khung debugger vẫn giữ nguyên.

GPUSTAT hiện là giá trị sẵn sàng đơn giản cộng vài control bit; chưa đúng mọi trạng thái GPU PS1. Bit tắt hiển thị không làm tắt viewer vì viewer là debugger VRAM. GPUREAD chưa có, lệnh không hỗ trợ sẽ dừng bằng thông báo. Reset GPU giữ nguyên VRAM.

## Kiểm thử
Ba chương trình core_tests, cpu_extended_tests (source cpu_extended.cpp) và gpu_tests đều PASS bằng GCC C++17 với -Wall -Wextra -Werror, AddressSanitizer và UndefinedBehaviorSanitizer (tắt leak detection do sandbox). GPU test kiểm tra packet chưa đủ, fill alignment/rounding/wrap, zero-height, odd upload, wrap hai chiều, reset parser, GPUSTAT subset và luồng CPU -> bus -> GPU.

UI mới chưa được build bằng Visual Studio/Windows SDK ở môi trường tạo mã. Khung có chiều rộng cố định; trên màn hình hẹp có thể bị cắt phần bên phải. Bản này chưa có polygon/texture, GTE, display timing/scanout, DMA, CD-ROM hoặc SPU, vẫn chưa boot BIOS/chơi game.

## Tài liệu tham khảo
- https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
- https://psx-spx.consoledev.net/ps1/gpu/memory-transfer-commands/
- Cấu trúc CPU tham khảo trước đó: https://github.com/stenzek/duckstation/blob/master/src/core/cpu_core.h

Không chép mã DuckStation vào dự án. Mã GPU mới được viết cho bộ lệnh tối thiểu nêu trên; chưa được đối chiếu conformance với phần cứng.
