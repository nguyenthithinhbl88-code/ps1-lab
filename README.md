# PS1 Lab — bản 16

Dự án giả lập PS1 C++17/Win32 cho Visual Studio 2022. Bản15 đã được người dùng
xác nhận hiện màn logo Sony Computer Entertainment bằng BIOS thật.
Bản16 thêm CD ISO/BIN/CUE và game homebrew Catch Stars có điều khiển.
**Chưa có game thương mại nào được xác nhận chạy. GTE/MDEC và audio còn thiếu.**

## Mở trong Visual Studio
Giải nén vào thư mục mới. Cài workload Desktop development with C++ và
C++ CMake tools for Windows -> File / Open / Folder -> chọn ps1-lab ->
x64 Release -> Startup Item ps1_ui.exe -> F5.
Tiêu đề phải là **PS1 Lab v16 - BIN CUE and CD Boot**.

Developer PowerShell:
```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Release --output-on-failure
.\build-vs\Release\ps1_ui.exe
```

## Chơi thử game CD đi kèm, không cần BIOS
1. Mount CD -> chọn **examples/catch_stars.cue**. Giữ .cue và .bin cùng thư mục.
2. Bấm **Boot EXE CD -> Run**.
3. Dùng **trái/phải** điều khiển thanh xanh, hứng vật vàng rơi xuống. Thanh trên
   trái là điểm, thanh đỏ trên phải là lượt còn lại; trượt3 lần sẽ reset điểm/lượt.
4. Bấm **Phong hinh** để xem lớn, Esc hoặc Pause để dừng.

Game là chương trình MIPS gốc của dự án, lấy EXE qua SYSTEM.CNF trong ISO9660,
sau đó tự gửi lệnh CD, đọc sector qua DMA3, vẽ bằng GP0 và đọc digital pad qua SIO0.
Không có byte BIOS/game/asset Sony. Đọc **GAME_GUIDE_VI.md**.

## Thử đĩa của bạn qua BIOS
**Nap BIOS -> Mount CD -> chọn .cue -> Boot BIOS CD -> Run.**
Nap BIOS cần file512KiB và tạo máy mới nên phải thực hiện trước Mount CD.
Boot BIOS CD giữ BIOS/đĩa, reset RAM/CPU/GPU/drive rồi chạy từ reset vector.
Nếu bộ đĩa nhiều BIN, chọn CUE để giữ track/pregap đúng; không đổi đuôi file thành ISO.
Region CD: Auto theo prefix BOOT hoặc chọn USA/Europe/Japan/Unlicensed cho GetID
emulated. GetID không phải xác thực wobble từ đĩa gốc.

Boot EXE CD nạp trực tiếp theo SYSTEM.CNF hoặc PSX.EXE;1, **không khởi tạo BIOS
kernel/services**; đường này dành cho homebrew tự khởi tạo, không thay thế boot retail.
Khi dừng/đen: Pause -> Save log; log có track layout, drive command/mode/error,
FIFO, GP0, video và input. Không cần gửi BIOS.

## Thay đổi chính
- ISO2048, BIN raw Mode1/Mode2 2352; CUE BINARY nhiều file/track, INDEX00/01,
  PREGAP; Mode2/2336 và cooked track2048.
- Sector data2048/raw2340, TOC/location/seek/ReadN/ReadS, IRQ2 và command queue.
- DMA3 chờ dữ liệu và truyền nhiều sector theo từng phần; không crash vì chưa có DRQ.
- ISO9660 đọc thư mục con, SYSTEM.CNF BOOT, path không phân biệt hoa/thường.
- Digital pad bàn phím; GPU flat/Gouraud line/polyline; giữ texture/sprite của bản14.
- CPU đọc/ghi word RAM và fetch BIOS qua đường nhanh; giữ MMIO và alignment checks.
- UI Video Hz(model)/CPU MIPS đo theo đồng hồ thật, cập nhật mỗi giây khi Run.
  Video Hz là tốc độ field của model hiện tại, không phải FPS game hoặc FPS paint.

Đọc **DISC_UPDATE.md** để xem phạm vi hỗ trợ, test và tài liệu tham khảo.
Timing CPU/video/CD vẫn gần đúng; audio track chỉ có metadata, chưa phát CDDA/XA;
chưa có memory card/save state, CHD/PBP/ECM/subchannel hay tương thích retail đầy đủ.

## Điều khiển pad
Mũi tên=D-pad, Z=Cross, X=Circle, A=Square, S=Triangle,
Q/W=L1/R1, E/R=L2/R2, Enter=Start, Shift=Select.
Chỉ đọc phím khi cửa sổ emulator ở foreground. Mất focus sẽ nhả nút.

## Demo và kiểm thử
GPU demo / Polygon / Texture demo dùng chương trình MIPS tự viết.
Open EXE nạp PS-X EXE, không phải EXE Windows.
examples/homebrew.iso và dma_polygon.exe là fixture cũ; boot_test_rom.bin là
ROM chẩn đoán tự viết, không phải BIOS Sony.

15 bộ kiểm thử lõi PASS trên GCC C++17 với warnings-as-errors và ASan/UBSan.
Host Linux chưa build/chạy UI Windows. Cần xác nhận Visual Studio trên máy bạn.
Các file *_UPDATE cũ ghi trạng thái lịch sử; README/DISC_UPDATE là phạm vi bản16.

Linux CLI:
```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/ps1_lab --disc examples/catch_stars.cue --steps 2000000
./build/ps1_lab --bios bios.bin --disc your_disc.cue --steps 20000000
```

Tái tạo game/đĩa gốc: `python tools/make_cd_game.py`.
BIN demo có vùng EDC/ECC zero để test emulator, không dùng làm đĩa burn/kiểm chuẩn ECC.
MIT cho mã dự án tự viết; không sao chép implementation của giả lập khác.
