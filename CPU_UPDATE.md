# CPU update 2 — 25/10/2026

## Thay đổi
- SRA/SLLV/SRLV/SRAV, SLT/SLTI; ADD/ADDI/SUB có exception tràn số.
- BLTZ/BGEZ/BLTZAL/BGEZAL, BLEZ/BGTZ và delay slot cho nhánh không được lấy.
- HI/LO, MFHI/MFLO/MTHI/MTLO; MULT/MULTU/DIV/DIVU, xử lý chia cho 0 và INT_MIN / -1.
- LB/LBU/LH/LHU/SB/SH; LWL/LWR/SWL/SWR và chuyển tiếp kết quả giữa cặp load unaligned.
- COP0 cơ bản: MFC0, MTC0, RFE; SR, Cause, EPC, BadVAddr, PRId.
- Exception: SYSCALL, BREAK, reserved instruction, overflow và địa chỉ không căn hàng. EPC/BD và BEV vector.
- UI hiển thị HI/LO, SR, Cause, EPC; bỏ macro trùng gây C4005.

## Cách dùng
Thay thư mục mã nguồn bằng bản cập nhật, mở lại bằng Visual Studio và build target ps1_ui. Không chép thư mục build cũ. Chạy test bằng CTest như hướng dẫn trước; có thêm cpu_extended_tests.

## Tham khảo DuckStation
Đã xem cấu trúc trạng thái CPU (COP0, PC/NPC, branch delay, pending loads) trong mã nguồn công khai:
https://github.com/stenzek/duckstation/blob/master/src/core/cpu_core.h
https://github.com/stenzek/duckstation/blob/master/src/core/cpu_core_private.h

Không nhập/copy mã DuckStation vào gói này; đây không phải bản fork hay lõi DuckStation. Không tải được cpu_core.cpp qua nguồn web ở phiên này nên chưa đối chiếu trực tiếp interpreter của DuckStation. Những kiểm thử trong dự án không phải bộ test DuckStation hay kiểm chứng tương đương DuckStation.

## Đã kiểm thử
GCC C++17 với -Wall -Wextra -Werror và AddressSanitizer/UndefinedBehaviorSanitizer: cả core_tests và cpu_extended_tests PASS. LeakSanitizer không chạy trong sandbox nên tắt riêng leak detection. Test bao gồm nhân/chia biên, tràn số, exception trong delay slot nhánh không được lấy, alignment, load byte có dấu, cặp unaligned ở 4 offset, COP0 load delay, RFE, RI và dịch phải số âm.

Chưa biên dịch UI mới bằng MSVC/Windows SDK trong môi trường tạo mã. UI bản trước đã chạy trên máy người dùng; bản cập nhật vẫn cần xác minh trên Windows.

## Chưa hoàn thiện
Chưa boot BIOS hoặc game; chưa có GPU/GTE, SPU, CD-ROM, DMA, timer/IRQ controller. COP0 chỉ một phần: chưa có IRQ delivery, breakpoint phần cứng, đầy đủ masks/privilege/coprocessor exceptions. Bus chưa hỗ trợ MMIO, bus lỗi vẫn dừng bằng host exception. Cache isolation chỉ bỏ store khi IsC bật, chưa mô phỏng cache thật. Chưa chính xác chu kỳ hoặc độ trễ nhân/chia. CPU vẫn cần kiểm thử conformance rộng hơn trước khi gọi là R3000A hoàn chỉnh. Unknown opcode đi vào RI vector, vì vậy Run có thể lặp handler thay vì báo lỗi opcode như bản cũ.
