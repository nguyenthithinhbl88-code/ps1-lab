# Bản 16 — BIN/CUE, controller, direct boot và game homebrew

## Kết quả đã kiểm chứng
15 bộ kiểm thử lõi PASS: GCC C++17 -Wall -Wextra -Werror và ASan/UBSan,
leak detection tắt trên host. Game Catch Stars đã qua đường CUE -> SYSTEM.CNF ->
MIPS CD commands -> DMA3 -> GPU; input trái/phải và logic tăng điểm/respawn được test.
CLI --disc cũng chạy được fixture, đọc2 sector, chuyển512word DMA và hiện frame.
Đây là kiểm thử homebrew của dự án, chưa có kết quả chạy game thương mại.
UI Win32 chưa build/chạy ở host Linux.

## Lớp ảnh đĩa mới
DiscImage giữ stream/file, bảng track và ánh xạ sector, không nạp toàn BIN vào RAM.
Mount qua đối tượng tạm: file hỏng/thiếu không phá đĩa đang mount.

| Input | Hỗ trợ |
|---|---|
| ISO |2048byte/sector, có ISO9660 descriptor|
| BIN đơn |auto sync+mode cho Mode1/2352 hoặc Mode2/2352; thiếu CUE sẽ không có audio TOC|
| CUE |FILE BINARY có tên chứa khoảng trắng, UTF8 path, nhiều file/track01..99|
| Track mode |MODE1/2048, MODE1/2352, MODE2/2048, MODE2/2336, MODE2/2352, AUDIO|
| Index |INDEX00/01, stored pregap và synthetic PREGAP; pregap đầu track1 loại khỏi LBA0|
| Audio |đọc metadata/TOC/raw, không playback hoặc mixing|

Data Mode1 lấy2048byte ở offset16 trong raw2352; Mode2 Form1 ở24.
Form2 có thể đọc raw nhưng bị từ chối khi yêu cầu ISO user-data2048.
Cooked ISO/2336 được dựng header raw; EDC/ECC không được tái tạo/xác thực.
Sector raw data phải có sync/mode khớp track. Giới hạn450000sector,99file/track,
CUE1MiB; kiểm tra alignment, index, số sector và path ISO.
WAV/MP3/ECM/CHD/PBP/CCD/MDS/subchannel, INDEX02+, POSTGAP và mixed stride trong
cùng một FILE chưa hỗ trợ; parser trả lỗi rõ thay vì đọc sai offset.

## CD controller và DMA
Lệnh được thêm/mở rộng: Getstat, Setloc, ReadN/ReadS, MotorOn/Stop/Pause/Init,
Mute/Demute, Setfilter/Getparam/Setmode, GetlocL/GetlocP, SetSession1,
GetTN/GetTD, SeekL/SeekP, Test20h, GetID và ReadTOC.
GetlocP dựng vị trí từ TOC; không phải dữ liệu SubQ chuẩn bảo vệ đĩa.
Test20h trả phiên bản drive mô phỏng cố định.

Mode sector800h và924h (2048/2340byte) đi qua FIFO và DMA3.
DMA bắt đầu trước DRQ sẽ chờ; transfer lớn hơn một sector tiếp tục theo các
lần sector-ready sau, giữ CHCR active và cập nhật BCR remaining gần đúng.
Completion/DICR/IRQ3 chỉ phát khi kết thúc toàn bộ transfer.
CD IRQ2 và response1/2/3/5 giữ cơ chế acknowledge cũ; tối đa4 command có thể
đợi pending response thay vì ném exception host ngay. Volume registers chỉ latch.

GetID tương thích BIOS bằng metadata mô phỏng: Auto suy region từ prefix BOOT
(SCUS/SLUS, SCES/SLES, SCPS/SLPS/SLPM...), hoặc chọn Region CD trong UI.
BIN không có wobble vùng lead-in để xác thực; đây không phải chứng nhận đĩa.
Homebrew/unknown ở Auto vẫn nhận unlicensed INT5, manual override hỗ trợ test BIOS.

Chưa có sector timing75/150Hz thật, seek latency, command busy timing, sector
ring-buffer/overrun chính xác, XA decoding/filtering hoặc CDDA Play. Play trả INT5.
Setfilter latch metadata; không bỏ qua sector dựa theo XA filter. Bit4 Setmode bị từ chối.

## Boot và CPU/GPU/input
- ISO9660 traversal có thư mục con, BOOT trong SYSTEM.CNF, cdrom: prefix,
  slash/backslash, chữ hoa/thường và optional version; fallback PSX.EXE;1.
- Boot EXE CD validate PS-X EXE rồi reset máy với media giữ nguyên và direct load.
  Không khởi tạo kernel TCB/EVENT/STACK của SYSTEM.CNF; dùng stack từ EXE header.
- Boot BIOS CD reset toàn bộ core, giữ BIOS512KiB và media để BIOS đọc CD thật qua MMIO.
- CPU fetch/load word RAM/BIOS và store RAM dùng đường decode nhanh, giữ endian,
  RAM mirror, alignment và MMIO side effects. Chưa benchmark mức tăng tốc trên Windows.
- GPU thêm flat/Gouraud line/polyline40h–5Fh, terminator, clipping/offset/mask,
  dither và blending. Raster Bresenham/interpolation là functional approximation;
  polyline joint pixels/rounding có thể khác GPU thật.
- Digital pad SIO0:01h,42h,5Ah,buttons active-low, snapshot, ACK IRQ7 gần đúng.
  Transfer byte/ACK là immediate, chưa timing baud chính xác, memory card/port2 disconnected.
- UI có Mount CD, Boot EXE CD, Boot BIOS CD, Region CD, Eject CD,
  keyboard pad, Video Hz(model) và CPU MIPS. Input chỉ khi app foreground.

## Kiểm thử mới
`disc_tests`: Mode1/Mode2 offsets, cooked/raw, Mode2/2336, Form2, single/multi-file
TOC, stored/synthetic pregap, first INDEX01 offset, malformed CUE và atomic mount;
SYSTEM.CNF nested path/legacy fallback; CD query/seek/Test/GetID/raw DMA3/IRQ2,
deferred commands, DMA start-before-data và hai sector/completion, digital pad,
MIPS game đọc CD/DMA, input và scoring; boot BIOS giữ BIOS/media và clear RAM.
`line_tests`: flat/Gouraud, endpoint/zero-length, polyline terminators,
clip/offset/mask/blend, reset và oversized reject.
13 bộ test cũ CPU/BIOS trace/GPU/texture/video/DMA/SPU/timers/SIO/CD chạy lại.

## Giới hạn để đọc đúng kết quả
Chưa có GTE, MDEC, audio output, memory cards hoặc save state; retail games thường
cần các phần đó. Không khẳng định mount thành công nghĩa là game sẽ chạy.
VBlank model bản15 vẫn gần đúng, Timer0 dotclock/Timer1 HBlank/sync còn thiếu.
Vì vậy có thể vẫn VSync timeout hoặc dừng ở opcode/MMIO khác.

## Nguồn đã tra cứu
- PSX-SPX CD drive/registers/commands: https://psx-spx.consoledev.net/cdromdrive/
- CD format: https://psx-spx.consoledev.net/dev/ps1/cdr/cdromformat/
- SYSTEM.CNF/PS-X EXE: https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/executables/
- CUE handling (tham khảo cấu trúc, không chép implementation):
  https://github.com/stenzek/duckstation/blob/master/src/util/cd_image_cue.cpp
- Controller exchange: https://psx-spx.consoledev.net/controllersandmemorycards/
- GPU lines: https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
- CUE mode names: https://www.gnu.org/software/ccd2cue/manual/html_node/MODE-_0028Compact-Disc-fields_0029.html
- Win32 input: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate

Mã dự án và game tự viết; không sao chép lõi giả lập, không kèm BIOS/game thương mại.
Các tài liệu UPDATE trước bản16 là trạng thái lịch sử.
