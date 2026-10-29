# Trạng thái kiểm tra bộ khung

- make check: PASS, biên dịch C11 với -Wall -Wextra -Wpedantic -Werror; help chạy được; asset HTML tồn tại.
- make test: FAIL như dự kiến: config_parse trả lỗi TODO B01; chưa có server lắng nghe.
- Makefile đã chỉnh sang cú pháp POSIX-style, nhưng chưa xác minh trên NetBSD/OmniOS.
- Yêu cầu đã cập nhật từ đề đầy đủ: HTTP/1.1 input, ba hệ, tar/checklist/git log/đóng góp đều.
- Chưa kiểm thử trình duyệt hoặc giao thức HTTP thành công; chưa có tính năng server hoàn chỉnh.
- Repo đích: BieenFR/group-http-sever; bộ khung được chuẩn bị cho một commit chung. Không có Issue/PR hoặc lời mời thành viên được tạo trong bước này.
- tests/http_acceptance.py chạy binary sws thật, không dùng server mô phỏng.
- CI hiện chỉ chạy make check; nhóm bật make test sau mốc 1.
