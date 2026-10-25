# Bản 8 — Timer / Root Counters

## Lỗi từ log người dùng
POST=6, 138233 lệnh. `000015A0: 0C0009E8` là JAL tới 000027A0; `000015A4: A6600000` là SH zero,0(s3), s3=1F801100. SH trong delay slot gây lỗi bus. PC trong log đã chuyển tới đích JAL, không phải địa chỉ lệnh gây lỗi.

## Bổ sung
- COUNT/MODE/TARGET cho Timer 0/1/2 tại 1F801100/110/120.
- Truy cập halfword và word, alias KSEG0/KSEG1; word dùng 16 bit thấp, 16 bit cao trả 0 theo lựa chọn đơn giản của emulator (không phải cam kết hardware).
- Counter chạy với clock hệ thống gần đúng, reset khi ghi MODE, match target, overflow flag, reset-on-target, flags reset khi đọc MODE.
- Timer2 chia 8 và sync-stop mode; IRQ once/repeat/pulse/toggle được latch vào I_STAT bit 4/5/6.
- Pulse chỉ mô hình hóa như một event, không có độ rộng xung chính xác.
- Save log ghi thêm trạng thái ba timer, không làm mất reached flags khi xuất log.
- Bỏ chuyển wchar_t -> char ngầm trong Save log để tránh cảnh báo C4244; log lỗi là ASCII, ký tự ngoài ASCII chuyển thành '?'.

## Độ chính xác
Tick một lần sau mỗi lệnh thực thi thành công, **không chính xác chu kỳ PS1**; chưa mô phỏng khoảng dừng sau ghi/reset counter. Dotclock/Hblank/Vblank chưa có: Timer0/1 ở external clock hoặc sync-enabled sẽ chờ, không tự dùng nhịp CPU thay thế. Chưa có đường IRQ controller -> COP0 -> exception, nên I_STAT có thể latch nhưng chưa ngắt CPU. Chưa có scheduler/video timing, DICR, CD-ROM/GTE hoàn chỉnh. Bản này xử lý MMIO timer đang chặn BIOS, không cam kết boot tới logo/menu hoặc chạy game.

## Thử lại
Giải nén thư mục mới -> Visual Studio -> ps1_ui.exe -> Nap BIOS đúng file đã dùng -> Run 10k hoặc Run. Nạp lại từ đầu, không tiếp tục trạng thái lỗi cũ. Nếu lỗi/vòng lặp mới, Pause -> Save log. Đây là BIOS Sony thật trên máy người dùng; ở môi trường phát triển chỉ có trace, chưa có file ROM để chạy end-to-end.

## Kiểm thử
8 bộ test PASS với GCC C++17 -Wall -Wextra -Werror, AddressSanitizer/UBSan (tắt leak detection). Test mới replay đúng JAL/SH trong log và kiểm tra target/overflow/read-clear, IRQ latch, one-shot/toggle, timer2 divide8/stop, alias, word access và external-clock wait. Diagnostic thay đổi được kiểm tra thêm bằng boot_tests.
UI mới chưa build MSVC/Windows SDK tại môi trường tạo mã; chưa chạy CMake tại đây do thiếu CMake. Các phiên bản UI trước đã chạy trên máy người dùng.

Tham khảo: https://psx-spx.consoledev.net/timers/
