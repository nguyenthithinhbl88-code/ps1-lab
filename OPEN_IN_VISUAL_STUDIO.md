Bản 3 có thêm nút **GPU demo** và khung VRAM. Xem **GPU_UPDATE.md** cho trạng thái hiện tại; phần hướng dẫn mở Visual Studio dưới đây vẫn áp dụng.

# Mở dự án có UI trong Visual Studio 2022

1. Giải nén ZIP.
2. Trong Visual Studio Installer, bật **Desktop development with C++** và **C++ CMake tools for Windows**.
3. Mở Visual Studio → **File → Open → Folder**, chọn thư mục `ps1-lab` có `CMakeLists.txt`.
4. Chờ CMake configure xong; chọn cấu hình x64 Debug hoặc x64 Release.
5. Ở Startup Item, chọn **ps1_ui.exe**, nhấn **F5**.

Nếu muốn tạo solution `.sln`, mở Developer PowerShell ở thư mục dự án:

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
.\build-vs\Release\ps1_ui.exe
```

Mở `build-vs/ps1_lab.sln`, đặt `ps1_ui` làm Startup Project nếu dùng solution.

## Nút điều khiển

- **Chay demo**: thực thi 4 lệnh MIPS tự viết; RAM tại 0x100 phải bằng 12.
- **Nap BIOS**: chọn BIOS 512 KiB bạn tự trích xuất; đặt PC ở reset vector BFC00000. Nạp thành công không có nghĩa là boot được BIOS.
- **Reset CPU**: đặt lại thanh ghi/PC; giữ RAM và BIOS.
- **Step**: thực thi một lệnh và xem thanh ghi.
- **Run/Pause**: chạy theo từng lô 100 lệnh qua Windows timer / tạm dừng.

UI dùng Win32 thuần C++17, không cần Qt/SDL hoặc NuGet. Bảng chữ là debugger, chưa phải màn hình GPU PS1. Chưa mở ISO/BIN/CUE hoặc chơi game. Lỗi opcode/MMIO chưa hỗ trợ sẽ dừng chạy và hiện thông báo.

## Kiểm thử

```powershell
ctest --test-dir build-vs -C Release --output-on-failure
```

Lõi và demo được biên dịch/kiểm thử bằng GCC trên Linux trong quá trình tạo dự án. UI Win32 cần biên dịch và kiểm tra trên Windows; môi trường tạo dự án không có Windows SDK hoặc Visual Studio, nên chưa xác nhận chạy UI trên Windows.
