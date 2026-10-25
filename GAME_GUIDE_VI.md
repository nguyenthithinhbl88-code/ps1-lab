# Catch Stars — game homebrew kiểm tra CD-ROM

Mục tiêu: di chuyển thanh xanh để hứng vật vàng. Mỗi lần hứng được tăng điểm,
biểu diễn bằng thanh tím phía trên trái. Thanh đỏ trên phải biểu diễn3 lượt;
trượt3 lần sẽ bắt đầu lại. Đây là game nhỏ tự viết để kiểm thử lõi PS1, không phải
một game thương mại được chuyển thành demo.

## Chạy
1. Build ps1_ui ở cấu hình **x64 Release**.
2. Mount CD -> examples/catch_stars.cue. catch_stars.bin phải ở cạnh CUE.
3. Boot EXE CD -> Run. Không cần BIOS cho game này.
4. Bấm trái/phải, hoặc bấm vào khung hình trước nếu emulator mất focus.
5. Phong hinh để ẩn debugger; Esc/Pause để dừng, Run để tiếp tục.

Không nhấp đúp catch_stars.exe: đó là PS-X EXE, không phải chương trình Windows.
Bản ISO tương đương cũng có thể mount bằng Mount CD.

## Cách game chạy
- Boot loader đi theo SYSTEM.CNF -> GAME/MAIN.EXE;1.
- MIPS gửi Setloc/ReadN, chờ CD interrupt, bật FIFO request và lập trình DMA3
  để đọc primary volume descriptor từ đĩa vào RAM3000h, rồi Pause drive.
- Vòng game chờ IRQ0 VBlank qua I_STAT; đọc phím bằng chuỗi SIO0 digital pad.
- CPU tính vị trí/va chạm/điểm và gửi GP0 clear/rectangle tới GPU.
- Trạng thái kiểm thử tại RAM2000h: X, frame, score, fallingY, misses (word liên tiếp).

Nguồn toàn bộ nằm ở tools/make_cd_game.py. Chạy Python3 để tái tạo EXE/ISO/BIN/CUE.
Fixture BIN dùng raw Mode2 Form1 có EDC/ECC zero vì lõi chưa kiểm ECC; không cam kết
boot trên PS1 vật lý hoặc dùng để kiểm chuẩn EDC/ECC.

## Tốc độ
Video Hz(model) là số field GPU giả lập tiến được mỗi giây thật.
CPU MIPS là triệu lệnh mỗi giây thật. Cả hai được lấy mẫu mỗi giây khi Run.
Chúng không đếm số ảnh game khác nhau và không chứng minh timing PS1 chính xác.
Cấu hình Debug hoặc F5 kèm debugger có thể chậm; thử Release, Ctrl+F5.
Game không có âm thanh hoặc lưu tiến trình trong bản16.
