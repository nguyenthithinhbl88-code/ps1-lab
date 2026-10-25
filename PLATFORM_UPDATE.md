# Bản 11 — IRQ CPU, DMA4, GPU readback/copy và SIO

## Mục tiêu
Mở rộng đường phần cứng thật đang còn thiếu ở BIOS. Không bỏ lệnh, không giả menu/logo, không nhập mã nguồn từ giả lập khác. Chưa có ROM Sony trong môi trường phát triển nên chưa kiểm chứng boot BIOS/game end-to-end.

## Mới
1. I_STAT/I_MASK -> COP0 Cause bit10 -> CPU interrupt exception khi SR cho phép. Software interrupt bits8/9 cũng được kiểm tra. Vector theo BEV, EPC/BD và mode stack; pending load được commit trước khi lấy IRQ. UI/log có bộ đếm interrupt exception.
2. DMA4 RAM -> SPU RAM và chiều ngược lại, kiểm tra transfer mode SPUCNT, wrap 512 KiB, completion DICR. Không trộn âm thanh/ADPCM/reverb.
3. GP0(C0) VRAM -> CPU/GPUREAD; DMA2 đọc GPU -> RAM, có underrun guard; GP0(80) VRAM copy, mask bits; GP1(10..1F) query draw area/offset/texture window; GP0(1F) GPU IRQ và GP1(02) acknowledge.
4. SIO0/SIO1 register interface tối thiểu: data/status/mode/control/baud, reset/acknowledge; SIO mô phỏng thiết bị chưa kết nối, trả FF cho synchronous RX, không ACK từ tay cầm/memory card, chưa có input.
5. Latch GPU/CD/SIO IRQ theo cạnh nguồn, tránh đặt lại I_STAT mỗi tick khi nguồn vẫn giữ mức cao. Timer IRQ pulse và DMA IRQ giữ đường riêng.

## Các thanh ghi đáng chú ý
| Địa chỉ | Tên | Trạng thái hiện tại |
|---|---|---|
| 1F801070 / 1074 | I_STAT / I_MASK | Nối vào COP0/CPU; I_STAT acknowledge bằng AND |
| COP0 r12/r13/r14 | SR / Cause / EPC | Interrupt masking/vector/mode stack cơ bản |
| 1F8010C0/C4/C8 | DMA4 MADR/BCR/CHCR | SPU RAM transfer hai chiều |
| 1F8010F4 | DICR | Completion flags/W1C/IRQ3 từ bản10 |
| 1F801DA6 | SPU transfer address | Đơn vị8 byte, cursor nội bộ |
| 1F801DAA / 1DAE | SPUCNT / SPUSTAT | DMA mode check; status đơn giản |
| 1F801810 / 1814 | GPUREAD/GP0 / GPUSTAT/GP1 | Thêm readback/query/GPU IRQ |
| 1F801040 / 1044 | SIO0 DATA / STATUS | Disconnected-device subset |
| 1F801048/104A/104E | SIO0 MODE/CTRL/BAUD | Latch/reset/ack |
| 1F801050..105F | SIO1 | Register/disconnected model gần đúng |

## Cách thử
Giải nén vào thư mục mới -> Visual Studio -> x64 Release -> ps1_ui.exe -> Nap BIOS -> Run. Với ưu tiên hiện tại, test BIOS riêng trước, chưa mount CD. Pause -> Save log khi dừng/chờ. Gửi log mới sẽ biết IRQ có chạy hay BIOS chặn ở phần cứng nào tiếp theo. Khởi động lại từ reset, không tiếp tục trạng thái lỗi bản10.

Homebrew ISO/EXE cũ vẫn dùng được. Boot homebrew từ ISO vẫn là direct loader, không tương đương CD boot BIOS.

## Giới hạn
- IRQ lấy tại ranh giới instruction; chưa cycle-accurate, chưa mô phỏng đầy đủ trường hợp COP2/IRQ latency. Cause line cập nhật khi vào step. Test pending load/EPC/BD, chưa phải hardware conformance suite.
- Không có video timing/scanout/Hblank/Vblank. Không tự phát Vblank giả. Timer0/1 external-clock/sync-enabled vẫn chờ như bản8; vì vậy BIOS có thể chờ.
- DMA đồng bộ, không scheduling/priority/DREQ chính xác. SPU DMA kiểm tra mode nhưng chưa FIFO pacing/IRQ address; chưa tạo âm thanh.
- GPU readback word có số pixel lẻ: halfword thừa là0; copy overlap dùng thứ tự tuần tự, chưa đảm bảo khớp hardware. Read/query làm thay đổi state; debugger nên dùng diagnostic chứ không đọc GPUREAD tùy tiện.
- GP1 display control mới chỉ lưu, viewer vẫn 320x240 tại VRAM(0,0); chưa có texture/gouraud/transparency/GTE hoàn chỉnh.
- SIO chuyển tức thì, chưa baud timing/8-byte FIFO/protocol đầy đủ, trạng thái SIO1 còn dùng chung approximation với SIO0. Không có tay cầm, memory card hoặc lưu game.
- CD-ROM vẫn chỉ ISO2048, chưa BIN/CUE/CHD/XA/CD-DA/retail license/TOC đầy đủ.
- Chưa xác nhận game thương mại chạy, chưa thể hứa logo/menu BIOS.

## Tham khảo thực tế đã đọc
- PCSX-Redux src/core/psxhw.cc: cách phân nhánh MMIO/SIO/DMA, I_STAT acknowledge và DMA callbacks. https://github.com/grumpycoders/pcsx-redux/blob/main/src/core/psxhw.cc
- DuckStation src/core/cpu_core.h: tách PC/NPC, COP0, pending ticks/load delay/interrupt interface. https://github.com/stenzek/duckstation/blob/master/src/core/cpu_core.h
- Tài liệu interrupt từ tác giả PSX-SPX: https://problemkaputt.de/psxspx-interrupts.htm
- SIO register reference: https://psx-spx.consoledev.net/serialinterfacessio/
- DMA: https://psx-spx.consoledev.net/dmachannels/
- GPU: https://psx-spx.consoledev.net/graphicsprocessingunitgpu/

Tham khảo kiến trúc/định dạng, không sao chép/ghép mã các dự án. Mã mới viết riêng, không phải lõi DuckStation/PCSX-Redux và không đạt mức tương thích của họ.

## Xác minh
11 test executables PASS với GCC C++17 -Wall -Wextra -Werror, AddressSanitizer/UBSan (tắt LeakSanitizer do sandbox). platform_tests mới kiểm tra IRQ hardware/software mask, BEV/vector, EPC/BD, pending load commit, DMA4 vòng đọc/ghi + wrap + DICR, GPU odd readback/copy/DMA/query/IRQ edge và SIO reset/ack/FF. Sau thay đổi xử lý cạnh IRQ, chạy lại platform_tests/CD/DICR đều PASS.

UI Win32 mới chưa build/run bằng MSVC/Windows SDK ở môi trường tạo mã; CMake chưa chạy tại đây vì thiếu CMake. Các test được biên dịch trực tiếp bằng GCC. Các bản UI trước đã được người dùng xác minh trên Windows.
