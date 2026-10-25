# Bản 13 — khung hình và vùng hiển thị BIOS

Ảnh người dùng có viewport cố định 640x480 trên cửa sổ 1920x1080,
chỉ đọc góc VRAM 320x240. Một phần đa giác đỏ/cam bị cắt ở cạnh phải/dưới.
Ảnh cũng báo Unsupported GP0 word 0x2c808080: việc đổi khung xem không
bổ sung lệnh textured quad này và không bảo đảm có logo hoàn chỉnh.

## Thay đổi
- Khung hình dùng toàn bộ vùng còn lại bên phải debugger khi resize/maximize.
- Nút Phong hinh ẩn debugger để khung chiếm toàn bộ vùng dưới toolbar;
  Hien debug đưa bố cục trở lại. Nút toolbar tự xuống dòng theo chiều rộng.
- Fit 4:3, căn giữa, viền đen, pixel scaling; không kéo méo hoặc cắt ảnh.
- display.hpp giải mã GP1(05) địa chỉ đầu framebuffer, GP1(06) độ rộng
  theo dotclock, GP1(07) số dòng và GP1(08) mode/interlace/depth.
- Hiển thị toàn bộ nguồn 640x480 khi mode/range BIOS chọn độ phân giải đó.
- Hỗ trợ chọn trang VRAM, RGB15/RGB24, wrap hàng và display-disable.
- Khi chưa nhận GP1 display configuration, demo cũ vẫn xem 320x240 từ (0,0).
- Debugger hiện tọa độ và kích thước nguồn; tiêu đề cửa sổ ghi v13.

## Kiểm thử và giới hạn
11 bộ kiểm thử lõi đều PASS với GCC C++17 -Wall -Wextra -Werror,
AddressSanitizer/UndefinedBehaviorSanitizer (tắt leak detection).
GPU tests thêm 640x480, đổi trang, mode width, giới hạn allocation,
RGB24 qua ranh giới halfword/hàng, blanking và hình học fit 4:3.
Chưa build/chạy Win32 UI ở máy phát triển Linux; cần kiểm tra Visual Studio.

Đây là framebuffer presentation gần đúng, chưa có VBlank/HBlank,
field weave/timing thực, overscan/CRT borders hoặc sửa VSync timeout.
Chưa có thử nghiệm BIOS Sony trên máy phát triển; không kèm BIOS/game.
Ảnh minh họa tạo riêng chỉ mô tả bố cục, không phải kết quả chạy BIOS.

## Cách dùng
Giải nén vào thư mục mới -> mở ps1-lab -> build ps1_ui -> Nap BIOS -> Run.
Maximize cửa sổ, bấm Phong hinh để xem lớn; Hien debug để quay lại.
Lệnh 0x2C vẫn sẽ làm CPU dừng tại lỗi hiện tại. Save log để kiểm tra bước GPU tiếp.

## Tham khảo
PSX-SPX: GPU Display Control Commands (GP1):
https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
