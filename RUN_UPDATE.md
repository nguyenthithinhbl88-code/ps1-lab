# Bản 8.1 — sửa Run quá chậm

Run cũ chỉ chạy tối đa 100 lệnh mỗi Windows timer callback (~16 ms), thành nút nghẽn nhân tạo. Bản mới chạy tối đa 100.000 lệnh/callback, kiểm tra ngân sách thời gian sau mỗi 128 lệnh và nhường UI sau khoảng 8 ms. Bảng trạng thái/VRAM chỉ refresh khoảng 10 lần/giây khi Run, giảm chi phí vẽ. Pause vẫn xử lý trên UI thread; một lệnh GPU/DMA dài có thể làm phản hồi vượt ngân sách 8 ms. Không có worker thread, không đổi timing CPU/timer mô phỏng và không bỏ vòng lặp BIOS.

Instructions trong UI đổi sang số thập phân để dễ đọc; PC/register/POST vẫn hex. Step và Run 10k giữ số lệnh yêu cầu. Tốc độ thực phụ thuộc máy/build; không khẳng định đạt full-speed PS1. Chọn x64 Release để đánh giá tốc độ.

Ảnh người dùng cho thấy R4/R5 tăng cùng độ lệch và R6 giảm giữa hai lần chụp, gợi ý một vòng xử lý dữ liệu đang tiến triển; chưa có trace nên chưa xác định được routine. POST=7 và khung đen không đủ để kết luận treo/boot logo thành công.

Cách dùng: giải nén thư mục mới, mở Visual Studio, build ps1_ui.exe (x64 Release), nạp lại BIOS -> Run. Khi gặp lỗi hoặc nghi lặp chờ, Pause -> Save log. Nếu đã có log của phiên cũ, vẫn gửi được; không cần đợi lâu để tạo log.

Chỉ sửa vòng chạy/UI: lõi CPU/GPU/SPU/timer và 8 bộ test của v8 giữ nguyên. Đã kiểm tra diff mã UI, chưa build/chạy Win32 bằng MSVC vì môi trường không có Windows SDK. Cần người dùng kiểm tra chạy trên Windows.
