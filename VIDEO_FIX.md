# Bản 15 — sửa hồi quy chờ GPU 480i

## Vấn đề của bản 14
Bản 14 bắt đầu phản ánh GP1(08h) vào GPUSTAT, bao gồm vertical resolution
và interlace. Tuy nhiên bit31 even/odd vẫn luôn bằng0. Một chương trình
chọn 480i rồi chờ bit31 đổi sẽ kẹt. Bản 13 chưa báo mode này đầy đủ nên
có thể đi theo nhánh khác; đây là thiếu sót trong thay đổi trạng thái bản14.

Hai ảnh người dùng ghi PC80059D58 và PC80059D68/Next80059D54,
GPUSTAT-like register values14CE220A, mask80000000 và số lệnh vẫn tăng.
Kết hợp code thiếu clock, đây là dấu hiệu mạnh của vòng chờ parity.
Không có trace BIOS mới hoặc BIOS Sony ở host nên chưa xác nhận nội dung
chính xác các lệnh tại những PC đó. Test dưới đây dùng chương trình tự tạo.

## Thay đổi
- Clock video chạy theo bước lõi CPU; đọc GPUSTAT không làm đổi thời gian.
- GPUSTAT.31 thay đổi theo scanline ở chế độ240 dòng, theo field ở480i,
  luôn0 trong vùng VBlank. GPUSTAT.13 cập nhật field, luôn1 khi không interlace.
- Clock vẫn chạy khi display-disable được bật: BIOS có thể chờ field trước
  khi gửi GP1 display-enable. Không ép bật màn hình, không bỏ qua lệnh BIOS.
- Bộ đếm dùng11/7 video clock cho mỗi core tick, 3413clock/263line NTSC,
  3406clock/314line PAL. Đây là gần đúng, không phải máy PS1 chuẩn chu kỳ.
- Entry VBlank phát IRQ0 qua I_STAT/I_MASK và cơ chế ngắt CPU đã có.
  Acknowledge IRQ trong cùng VBlank không tự phát lại cho tới entry kế.
- UI/log có Video frames, line, field, VBlank và số entry để chẩn đoán.
- Giữ texture/CLUT/blending/sprite của bản14 và bố cục fit4:3.

## Kiểm thử
13 bộ kiểm thử lõi PASS với GCC C++17 -Wall -Wextra -Werror,
AddressSanitizer/UndefinedBehaviorSanitizer (leak detection tắt).

`video_tests` thêm:
1. 480i parity giữ nguyên trong field nhưng đổi ở field kế, kể cả khi màn hình
   bị disable; bit31 bằng0 khi VBlank.
2. Progressive parity đổi mỗi scanline, field bit13 giữ1; display range thay
   đổi blanking; PAL không wrap sớm tại line263.
3. IRQ0 latch/ack/re-entry và CPU thực sự vào vector ngắt.
4. MIPS tự tạo poll GPUSTAT bit31 -> gửi display-enable -> ghi marker thành công.
   Test bắt buộc hoàn thành dưới1.300.000 lệnh. Trạng thái framebuffer sau đó
   có thể được đọc qua display_pixel. Không dùng byte BIOS Sony hoặc ảnh/logo sẵn.
5. Đọc GPUSTAT lặp lại không tự tick; reset khôi phục clock và GPUSTAT.

12 bộ kiểm thử cũ được chạy lại. UI Win32 chưa build/chạy ở host Linux;
cần kiểm tra bằng Visual Studio trên Windows.

## Giới hạn
Một bước lệnh vẫn được tính như một clock CPU, không tính wait states,
cache, pipeline hoặc thời gian DMA/render. PAL/NTSC interlace dùng số dòng
nguyên cố định, chưa mô hình nửa scanline/oscillator theo console region.
Chưa đồng bộ Timer0 dotclock/Timer1 HBlank/sync, chưa weave field scanout thật.
Vì vậy chưa cam kết xóa mọi VSync timeout hoặc boot menu BIOS hoàn chỉnh.
Chưa xác nhận chữ SONY/game thương mại trên bản15. GTE/MDEC/audio vẫn thiếu.

## Chạy lại
Giải nén vào thư mục mới -> mở ps1-lab -> build ps1_ui.
Tiêu đề phải là **PS1 Lab v15 - Video Timing Fix**.
Nap BIOS -> Run, không dùng Reset CPU để tiếp tục RAM từ bản14.
Quan sát Video frames/VBlank tăng. Nếu màn hình vẫn đen, Pause -> Save log;
log mới có phase video và GP0 để chẩn đoán bước tiếp. Không cần gửi BIOS.

## Nguồn tham khảo
PSX-SPX: GPU Status Register và GPU Timings:
https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
Đã đối chiếu mô tả trạng thái CRTC/field và reset trong DuckStation:
https://github.com/stenzek/duckstation/blob/master/src/core/gpu.cpp
Mã timing ở dự án được viết riêng, không sao chép implementation.

Các tài liệu UPDATE cũ ghi trạng thái lịch sử; các dòng nói chưa có VBlank
được thay bằng mô hình gần đúng nêu ở bản15.
