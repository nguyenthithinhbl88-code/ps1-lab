# Bản 6 — đường khởi động ROM và chẩn đoán BIOS

## Thử với ROM kiểm tra đi kèm
1. Giải nén vào thư mục mới, mở Visual Studio, chạy ps1_ui.exe.
2. **Nap BIOS** -> `examples/boot_test_rom.bin`.
3. Bấm **Run 10k**.
4. Kết quả mong đợi: `RAM[0x100]: 24589` (0x600D), POST hiển thị `42` (hex).
5. TTY hiện `BOOT ROM: CPU -> BIOS ROM -> MMIO -> RAM -> B0 OK`.
6. Bấm **Save log** để lưu trạng thái/thanh ghi/32 lệnh được fetch gần nhất.

ROM thử không vẽ hình; khung VRAM đen là bình thường. Chương trình đi vào vòng lặp sau khi xong. Đây là ROM chẩn đoán tự viết 512 KiB, không phải BIOS Sony, không có menu, không thay thế BIOS để chơi game. Có source tại tools/make_boot_rom.py, tạo lại bằng Python 3.

## Thử BIOS PS1 thật
Dùng BIOS 512 KiB bạn tự trích xuất. Nap BIOS tạo máy mới và bắt đầu ở BFC00000. Nhấn Run 10k theo từng đợt; khi dừng do phần cứng chưa hỗ trợ, Save log ghi thông báo kèm địa chỉ bus và trace để xác định phần còn thiếu. Nếu chương trình lặp mà không lỗi, Pause rồi Save log. Không khẳng định BIOS sẽ tới logo/menu: chưa có file BIOS Sony để kiểm thử ở phiên phát triển này.

## Mới
- Latch đọc/ghi từng byte cho Memory Control 1 ở 1F801000..1F801023, RAM config 1F801060, cache control đúng địa chỉ ảo FFFE0130.
- Base expansion register giữ high byte 1F; đọc header expansion vắng mặt 1F000000..1F0000FF trả FF.
- POST byte 1F802041; latch I_STAT/I_MASK, I_STAT acknowledge bằng AND; chưa có nguồn IRQ hoặc CPU IRQ delivery.
- Theo dõi putchar tại entry A0(3C)/B0(3D) vào TTY. Chỉ quan sát; CPU vẫn thực thi routine trong RAM/BIOS. Không HLE, không tự trả về, không hiện thực toàn bộ BIOS API, printf/puts không bảo đảm có output nếu không đi qua entry được theo dõi. Entry chưa khởi tạo cũng có thể bị ghi log khi phần mềm nhảy vào; TTY không chứng minh call đã hoàn thành.
- Trace vòng 32 lệnh fetch thành công, tổng số lệnh, diagnostic dump gồm thanh ghi/COP0/POST/config và TTY giới hạn 8 KiB.
- Unsupported bus access báo địa chỉ gốc; các MMIO không hỗ trợ vẫn dừng, không trả 0 hàng loạt.

## Giới hạn cần hiểu
Các memory register chỉ là latch, không mô phỏng waitstate/size remapping/cache tags; reset latch là giá trị đơn giản, chưa đối chiếu reset hardware. Cache isolation vẫn đơn giản như bản trước. Không thêm BIOS HLE hay kernel services. Chưa có GPU/GTE đầy đủ, SPU/CD-ROM/SIO/timers, IRQ scheduling, DICR hoặc scanout; chưa thể hứa boot BIOS Sony hay chạy game. CPU cũng chưa hoàn chỉnh.

6 bộ kiểm thử PASS bằng GCC C++17, -Wall -Wextra -Werror, AddressSanitizer/UBSan (leak detection tắt do sandbox). Boot test chạy ROM thật qua interpreter từ reset vector, kiểm tra MMIO/cache/POST, nhảy đến B0 trong RAM và quay về bằng JR; kiểm tra byte lanes, alias, IRQ latch, fault address và giới hạn TTY. UI mới chưa được build MSVC/Windows SDK trong môi trường này; các bản UI trước đã được người dùng thử trên Windows. CMake chưa chạy ở môi trường tạo mã vì không có CMake.

## Tham khảo
https://psx-spx.consoledev.net/memorycontrol/
https://psx-spx.consoledev.net/expansionportpio/
https://psx-spx.consoledev.net/dev/ps1/kernelbios/tty-console-std-io/
