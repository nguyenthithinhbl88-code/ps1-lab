# Bản 4 — polygon màu phẳng

Giải nén vào thư mục mới, mở ps1-lab bằng Visual Studio, chọn ps1_ui.exe và nhấn F5. Bấm **Polygon** trên hàng nút để chạy demo CPU -> GP0. Khung bên phải hiện hai tam giác lồng nhau và một tứ giác.

## Đã thêm
- GP0(20h): tam giác opaque không texture, màu phẳng; gói 4 word.
- GP0(28h): tứ giác opaque không texture, màu phẳng; gói 5 word, tách thành hai tam giác.
- GP0(E3/E4): vùng vẽ với hai góc inclusive, chỉ áp dụng primitive.
- GP0(E5): offset tọa độ có dấu 11 bit.
- GP0(E6): force/check mask bit cho primitive và CPU upload; fill không chịu mask.
- Quy tắc cạnh top-left đơn giản, hỗ trợ hai winding, bỏ hình suy biến và primitive quá lớn.
- Demo mới dùng mã MIPS để gửi mọi gói vẽ; không cần BIOS.

## Kiểm thử
Bốn chương trình core_tests, cpu_extended_tests, gpu_tests, polygon_tests PASS dưới GCC C++17 với -Wall -Wextra -Werror và AddressSanitizer/UndefinedBehaviorSanitizer (leak detection tắt theo giới hạn sandbox). Test polygon kiểm tra winding, phủ kín tứ giác, clip, offset âm, mask, degenerate và output demo CPU.

UI Windows mới vẫn chưa được biên dịch/chạy trong môi trường tạo mã. Kiểm thử không thay thế kiểm chứng phần cứng: rasterizer hiện dùng edge function tại tọa độ nguyên, chưa cam kết khớp scan conversion/fixed-point/subpixel chính xác của PS1. Chưa hỗ trợ gouraud shading, semi-transparency, texture/dithering, GTE, DMA, timing/scanout hoặc game/BIOS boot. Viewer vẫn chỉ là vùng VRAM cố định 320x240, không phải display controller.

Tài liệu tham khảo encoding drawing area/offset/mask:
https://psx-spx.consoledev.net/dev/ps1/gpu/rendering-attributes/
https://psx-spx.consoledev.net/graphicsprocessingunitgpu/

Không nhập mã DuckStation vào bản cập nhật này. Các ghi chú GPU_UPDATE.md và CPU_UPDATE.md mô tả bản trước, một số giới hạn đã thay đổi theo bản 4.
