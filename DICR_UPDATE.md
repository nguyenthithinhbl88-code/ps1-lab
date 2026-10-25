# Bản 10 — BIOS ResetCallback / DICR

## Lỗi được xác nhận
Trong log người dùng, `8005A678: ADC00000` là SW zero,0(t6); t6=1F8010F4. Đây là ghi 0 vào DICR khi TTY in `ResetCallback: _96_remove ..`. PC hiển thị 8005A67C do đã tiến pipeline. Bản mới replay 4 opcode 8005A66C..8005A678 bằng CPU và xác nhận ghi DICR thành công.

## Triển khai
- DICR 1F8010F4 đọc/ghi word, hỗ trợ alias.
- Lưu bits điều khiển 0..6, 15, 16..23; reserved bits đọc 0.
- Bits 24..30 là cờ hoàn tất kênh, write-one-to-clear. Bit31 read-only, tính từ bit15 hoặc master enable và pending flags.
- Hoàn tất DMA hiện hỗ trợ (2/3/6) đặt channel flag khi cả channel enable và master enable được bật.
- Không mất pending flag khi tắt channel enable; explicit W1C mới xóa.
- Cạnh 0->1 của master flag đặt I_STAT IRQ3; phải acknowledge DICR và I_STAT riêng. Việc đọc hoặc giữ master flag=1 không tự phát cạnh mới.
- Diagnostic log thêm DPCR/DICR/I_STAT/I_MASK; sửa nhãn UI để phản ánh CD-ROM subset đã có.

## Giới hạn
Chưa có đường I_STAT/I_MASK -> COP0 -> CPU interrupt exception. Vì vậy IRQ3 đã latch không có nghĩa handler đã chạy. Bits0..6 chỉ lưu; chưa phát interrupt mỗi slice/list node, chỉ completion cuối. Bit15 có thể ghi và tham gia master logic, chưa tự phát hiện đầy đủ bus-error DMA. Transfer vẫn đồng bộ; DMA4/SPU, priority/chopping và timing chưa có. DICR byte/halfword access chưa hỗ trợ. Chưa chứng minh boot BIOS tới logo/menu hay game; không vá BIOS/skip lệnh.

## Thử BIOS trước
Giải nén thư mục mới, mở Visual Studio, build ps1_ui.exe. Nap BIOS đúng file đã test -> Run. Theo yêu cầu người dùng, chưa Mount ISO hoặc Boot homebrew. Nạp từ đầu, không tiếp tục snapshot lỗi cũ. Nếu dừng/chờ, Pause -> Save log và gửi log mới.

10 bộ test PASS với GCC C++17 -Wall -Wextra -Werror + AddressSanitizer/UBSan (leak detection tắt do sandbox). Test mới gồm exact trace replay, enable/master matrix 7 kênh (test riêng logic flags), W1C, pending flag persistence, IRQ3 edge/ack, read-only bit31, actual OTC completion. Không có ROM Sony ở môi trường phát triển nên chưa kiểm thử BIOS thật end-to-end. UI mới chưa build/chạy bằng MSVC/Windows SDK tại đây; CMake chưa chạy do môi trường thiếu CMake.

Tham khảo: https://psx-spx.consoledev.net/dmachannels/ (DICR).
