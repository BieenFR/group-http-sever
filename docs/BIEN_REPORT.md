# Phần Biên — bàn giao và giới hạn

## Kết quả
Đã viết config.c và server.c, giữ nguyên API trong include/http.h, không làm thay phần parser/response/logger/CGI của các bạn.

- getopt: -d -h -c -i -l -p; port mặc định 8080, chỉ nhận số từ 1 tới 65535.
- Root và CGI root dùng realpath và kiểm tra directory; log tương đối được đưa thành đường dẫn tuyệt đối trước chdir daemon.
- getaddrinfo, IPv4/IPv6 riêng với IPV6_V6ONLY; SO_REUSEADDR, FD_CLOEXEC, listener nonblocking, poll/accept.
- Debug chạy foreground, phục vụ một client mỗi lần. Request logging do logger.c thực hiện, chưa có implementation thật.
- Chế độ thường: double fork, setsid, chdir /, umask 027, stdin/out/err về /dev/null; pipe báo startup cho tiến trình gọi.
- Mỗi client daemon có worker riêng, giới hạn 128 worker; thu hồi bằng waitpid WNOHANG.
- SIGPIPE được bỏ qua; socket recv/send timeout và alarm tổng 15 giây cho một request; SIGINT/SIGTERM dừng listener và worker, SIGKILL sau thời gian chờ nếu cần.
- Lời gọi request_read → response_send → log_request đúng API; fd client do server đóng.

## Tự chạy trên Ubuntu
```bash
sudo apt update
sudo apt install build-essential python3
make test-bien
./sws -d -i 127.0.0.1 -p 8081 www
```
Dùng cổng 8081 để tránh cổng 8080 đang được Docker sử dụng trên máy của bạn. Nếu 8081 cũng bận, chọn cổng khác.

Lệnh cuối mở listener, nhưng curl/trình duyệt chưa có trang để hiển thị vì request/response còn stub. Terminal debug sẽ báo Function not implemented. Đây là phụ thuộc còn thiếu, không phải chứng cứ website đã hoạt động.
Bấm Ctrl+C để dừng. Nên chỉ dùng -d trong lúc các module chưa ghép xong.

## Kết quả kiểm thử
12 test trong tests/test_bien.py: CLI hợp lệ/lỗi; IPv4 + paths; IPv6; wildcard cả hai hệ địa chỉ; occupied port; address lỗi; log destination lỗi; debug tuần tự/timeout; SIGTERM giữa request; client ngắt/rò fd; daemon nhiều kết nối + paths + log plumbing; listener binary thật.

Module doubles chỉ là driver kiểm thử, không parse HTTP và không phục vụ HTML. Test executable được build trong thư mục tạm, không đưa vào sws. Không thay logger thật bằng driver trong production.

## Cần phối hợp tiếp
- Bảo phải xử lý recv EINTR, EOF, timeout, giới hạn request và điền error_status/first_line an toàn.
- Việt An phải xử lý partial send, HEAD, errors; response_send nhận request=NULL khi parser thất bại.
- Lê An phải fflush hoặc close logger trước khi worker _exit; CGI exec phải giữ listener/client fd đúng ownership (client FD_CLOEXEC), stdout qua pipe.
- Worker client chạy trong process group riêng, gồm CGI con nếu CGI không tự tách group. Khi tắt server, group nhận TERM rồi KILL khi cần.
- B06 chưa hoàn thành vì chưa có code CGI của Lê An để tích hợp/review; timeout CGI riêng và reaping phải làm trong cgi.c.
- B05 mới có Makefile cơ bản; cần build/run thật trên NetBSD 11.0 và OmniOS, cấu hình LDLIBS nếu hệ yêu cầu.
- Test log và daemon runtime là test module plumbing. Chưa kiểm thử HTTP hoàn chỉnh hoặc tính đúng của request logs thật.
- Người dùng đã yêu cầu push phần Biên lên feature/bien-server; main không thay đổi.
