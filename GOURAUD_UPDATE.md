# Bản 12 — gói GPU Gouraud trong log BIOS

Log người dùng báo `Unsupported GP0 word 0x380000b2`, POST=7,
PC=800541e4 sau store tại 800541e0. Đây là điểm emulator dừng,
không phải kết luận BIOS hỏng. Log cũng chứa nhiều `VSync: timeout`.

## Thay đổi
- GP0 30h: opaque untextured Gouraud triangle, 6 word.
- GP0 38h: opaque untextured Gouraud quad, 8 word.
- Mỗi đỉnh gồm RGB24 rồi XY; màu đầu nằm trong command word.
- Packet buffer tăng từ 5 lên 8 word; payload không bị giải mã thành command.
- Nội suy RGB bằng trọng số barycentric số nguyên trước khi giảm xuống RGB15.
- Khi đảo winding, đổi cả tọa độ và màu tương ứng.
- Tái dùng drawing area, signed drawing offset, mask check/force và top-left coverage.
- Giữ cách chia quad hiện có: (0,1,2) rồi (1,2,3), đúng thứ tự đỉnh PS1.
- Flat polygon giữ đường ghi màu hằng để tránh phép chia trên mỗi pixel.

## Kiểm thử
`polygon_tests` kiểm tra RGB tại điểm có trọng số biết trước, đảo winding,
quad màu hằng phủ đủ hình chữ nhật, màu riêng của đỉnh thứ tư,
packet chưa đủ word không vẽ, command kế tiếp không lệch,
GP1 reset packet, mask, clipping và DMA2 linked list chứa chính opcode
0x380000b2. Các bộ kiểm thử CPU/bus/thiết bị cũ được chạy lại.
Không có BIOS Sony đi kèm hoặc được chạy trên máy phát triển.

## Giới hạn
Chỉ thêm 30h và 38h; textured/semi-transparent polygons vẫn chưa hỗ trợ.
Nội suy là xấp xỉ chức năng, chưa khớp fixed-point/rounding của GPU thật;
chưa có dithering. Quad được vẽ khi nhận đủ packet.
Chưa thêm VBlank/HBlank, đồng hồ video hoặc scanout theo thanh ghi GP1.
Vì vậy `VSync: timeout` trong log chưa được giải quyết; bản này chưa chứng minh
logo/menu BIOS hoặc game thương mại hoạt động. UI vẫn xem vùng VRAM cố định.

## Tham khảo
PSX-SPX, GPU Render Polygon Commands:
https://psx-spx.consoledev.net/dev/ps1/gpu/render-polygon-commands/
(bitfield, thứ tự color/vertex, cách chia quad). Mã được viết riêng cho dự án.

## Test trên Windows
Giải nén bản mới vào thư mục mới, mở ps1-lab bằng Visual Studio, build ps1_ui.
Nap BIOS -> chọn BIOS 512 KiB -> Run. Chưa cần Mount ISO.
Nếu dừng: Save log và gửi log mới cùng ảnh. Không dùng tiếp trạng thái RAM cũ.
