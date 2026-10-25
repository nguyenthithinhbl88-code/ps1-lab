# Bản 7 — xử lý lỗi BIOS POST 4 / SPU

## Bằng chứng từ log người dùng
Lệnh BFC0687C: A4400186 = SH zero,0x186(v0), v0=1F801C00. Địa chỉ ghi thực là 1F801D86 (âm lượng reverb phải). CPU đã fetch 80929 lệnh; POST=4. PC hiển thị BFC06880 vì pipeline đã tăng trước lỗi bus. Không phải bằng chứng đã hiển thị logo PS1.

## Sửa
- SPU register bank 1F801C00..1F801DFF, truy cập halfword và word; CPU LH/LHU/SH dùng bus read16/write16 thay vì tách byte.
- RAM âm thanh 512 KiB; transfer address nhân 8, con trỏ nội bộ wrap.
- FIFO tối đa 32 halfword; ghi khi dừng được xếp hàng, mode manual-write flush vào RAM. Transfer thực thi ngay ở mode này, không timing/FIFO hardware chính xác.
- SPUCNT ở 1F801DAA, SPUSTAT ở 1F801DAE; status mode bits phản chiếu ngay, chưa có độ trễ. Không giả báo DMA-ready.
- Không cho write vào một số status/current-volume read-only. Các voice/reverb register khác chỉ lưu giá trị, chưa tạo âm thanh.
- UI có bộ đếm SPU writes.

## Thử lại
Giải nén bản mới vào thư mục mới, mở Visual Studio, chạy ps1_ui.exe. **Nap BIOS lại từ đầu**, dùng đúng BIOS đã thử, rồi Run 10k / Run. Không tiếp tục trạng thái cũ: lệnh lỗi trong bản cũ đã tăng PC. Nếu gặp lỗi mới hoặc vòng lặp, Pause -> Save log và gửi log mới.

7 bộ kiểm thử đều PASS bằng GCC C++17 với -Wall -Wextra -Werror, AddressSanitizer/UBSan (leak detection tắt). Test mới replay chính xác 3 opcode tại BFC06874..BFC0687C và xác nhận hoàn thành ghi SPU, ngoài test alias/word/halfword, FIFO, address wrap, mode/status và overflow. Không có BIOS Sony để kiểm thử phần sau của BIOS; chỉ có trace cung cấp. UI mới chưa build/chạy bằng Windows SDK/MSVC trong môi trường tạo mã; CMake vẫn chưa chạy ở môi trường này vì chưa có CMake.

## Giới hạn
Đây là giao tiếp SPU tối thiểu phục vụ khởi tạo, **chưa phải bộ giả lập âm thanh đầy đủ**. Chưa có DMA4, ADPCM, ADSR, 24 voice synthesis, reverb, playback, IRQ hoặc timing. Byte access đến SPU chưa hỗ trợ; FIFO manual-read bị từ chối. Không cam kết BIOS Sony boot tới menu/logo. GPU, GTE, CD-ROM/timer/IRQ vẫn còn thiếu như README.

Tham khảo: https://psx-spx.consoledev.net/psx-spx.pdf (SPU memory access/status/reverb sections).
