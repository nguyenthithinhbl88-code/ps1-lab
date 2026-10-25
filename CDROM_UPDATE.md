# Bản 9 — CD-ROM / ISO / DMA3 và GPU draw state

## Log BIOS thật
Log người dùng dừng ở `8005A4D4: AC780000` (SW t8,0(v1)); v1=1F801810, t8=E1003000. Đây là GP0(E1) draw mode, chưa được bản cũ nhận. Bản mới nhận E1, E2, giữ trạng thái draw mode/texture window; GP1(05..08) giữ cấu hình display. Chưa hiện thực texture/dither/scanout theo các bit đó. Unsupported GP0 tiếp theo ghi nguyên word vào thông báo lỗi, không chỉ ghi chung chung.

## Thử homebrew từ ảnh đĩa
1. Giải nén thư mục mới, mở Visual Studio, chọn x64 Release / ps1_ui.exe.
2. Bấm **Mount ISO**, chọn `examples/homebrew.iso`.
3. Bấm **Boot homebrew**, rồi **Run 10k**.
4. Tam giác cam/hình chữ nhật xanh xuất hiện như EXE mẫu trước.

Đây là EXE tự viết trong đĩa ISO9660 tự tạo; không có game/BIOS Sony. Boot homebrew dùng đọc trực tiếp thư mục ISO9660 và nạp PSX.EXE;1, **bỏ qua boot BIOS và CD controller**. Đây chỉ là đường chạy homebrew tự khởi tạo, không chứng minh game thương mại chạy. Không khởi tạo BIOS services. Mã tạo ISO: `python tools/make_iso.py`, dựa trên EXE mẫu do `tools/make_demo.py` tạo.

## Thử qua BIOS
**Nap BIOS trước -> Mount ISO sau -> Run**. Nap BIOS tạo máy mới nên eject đĩa cũ. Có thể mount ISO2048 tự trích xuất để kiểm tra lệnh CD, nhưng BIOS vẫn có thể dừng ở phần cứng thiếu. Pause -> Save log khi gặp lỗi/chờ. Không tự đọc game SYSTEM.CNF/khởi tạo retail boot.

## CD-ROM đã có
- ISO 2048 byte/sector, file mở dạng stream, kiểm tra CD001 tại LBA16 và giới hạn 450000 sector; không nạp toàn ảnh vào RAM. Chưa nhận BIN2352/CUE/CHD/multitrack.
- Byte MMIO 1F801800..1803: index, parameters, response FIFO, data FIFO, interrupt enable/flag và acknowledge/request.
- Lệnh subset: Nop/Getstat, Setloc BCD, ReadN/ReadS, Stop/Pause/Init, Setmode/Getparam, SeekL, GetID.
- Command ACK/completion/sector event tách qua hàng đợi, chờ acknowledge trước khi tiếp tục, latch IRQ2 vào I_STAT.
- Read sector 2048 có bounds check, data FIFO; manual DMA3 device->RAM, báo underrun khi thiếu dữ liệu thay vì âm thầm ghi zero.
- GetID không giả chữ ký Sony: luôn báo unlicensed/no-disc qua INT5 completion. Unsupported command trả INT5; register bank không hỗ trợ báo lỗi rõ.
- Boot homebrew chỉ tìm root PSX.EXE;1, chưa hỗ trợ subdirectory/SYSTEM.CNF.

## Giới hạn quan trọng
CD event tick theo lệnh CPU, không đúng timing CD 75/150 sector/s. Không IRQ delivery CPU/COP0, DICR, CD-DA/XA, filter, raw sector mode, TOC, license/region handling hoặc audio mixer. ReadS hiện cùng đường ReadN, SeekL là phản hồi chức năng gần đúng, không mô phỏng cơ học. Setmode chỉ hỗ trợ đường dữ liệu2048; raw-mode bit bị từ chối. Các mode khác chỉ lưu, chưa cung cấp semantics CD-XA. Không có IRQ có thể khiến BIOS chờ dù CD events đã có; GetID unlicensed có thể ngăn BIOS boot đĩa. Chưa hỗ trợ texture/GTE/gouraud nên **chưa có game thương mại nào được xác nhận chạy**.

## Kiểm thử
9 bộ kiểm thử PASS trên GCC C++17 -Wall -Wextra -Werror với AddressSanitizer/UBSan (leak detection tắt do môi trường). CD test kiểm tra commands/ACK/completion, BCD lỗi, IRQ latch, đọc PVD sector không rỗng qua DMA3 vào RAM, ISO directory -> load EXE -> CPU/GPU output, GP0 E1003000/E2, GP1 state và đọc ngoài ảnh. Đây là fixture tự tạo, chưa phải test tương thích game/BIOS Sony đầy đủ.

UI mới chưa build/chạy bằng MSVC/Windows SDK trong môi trường tạo mã; CMake chưa chạy ở đây vì thiếu CMake. Lõi và các target test được build trực tiếp bằng GCC. Run nhanh của bản 8.1 được giữ nguyên.

Tham khảo:
https://github.com/psx-spx/psx-spx.github.io/blob/master/docs/cdromdrive.md
https://psx-spx.consoledev.net/dmachannels/
https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
