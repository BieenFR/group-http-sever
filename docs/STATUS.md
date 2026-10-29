# Trạng thái bản Biên

- make check: PASS, C11 với -Wall -Wextra -Wpedantic -Werror.
- make test-bien: 12/12 PASS trên Linux của môi trường làm việc.
- IPv4/IPv6 và wildcard: đã thử thật bằng TCP socket.
- Debug tuần tự, timeout request, SIGTERM, daemon nhiều client, đường dẫn tuyệt đối, log destination lỗi, cổng bị chiếm: đã thử.
- Một test dùng binary sws thật: xác nhận listener chạy, nhưng chưa có response HTTP từ stub.
- Các test còn lại link src/server.c + config.c thật với module doubles được đặt riêng trong tests/fixtures; không phải bằng chứng parser/response/logger/CGI thật chạy được.
- make test kiểm tra HTTP đầy đủ vẫn chưa đạt vì module của các bạn còn TODO.
- NetBSD/OmniOS chưa được kiểm tra; không đánh dấu tương thích hoàn tất.
- Phần Biên được bàn giao trên feature/bien-server; không merge main.
