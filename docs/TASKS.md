# Bảng công việc của nhóm
B01–B04 đã triển khai và kiểm tra Linux trong bản Biên, đang chờ review. B05 còn kiểm tra nền tảng tham chiếu; B06 còn phối hợp với CGI. Các phần Bảo/Việt An/Lê An đã được chuẩn bị trong các nhánh tương ứng; checklist chỉ được đánh dấu sau khi thành viên review thật. Giữ checklist chưa đánh dấu cho tới khi review.

## Biên — feature/bien-server
- [ ] B01: config.c: getopt cho -d -h -c -i -l -p, mặc định port 8080; validate port 1..65535 và document root.
- [ ] B02: server.c: socket/bind/listen/accept với getaddrinfo, IPv4/IPv6, SO_REUSEADDR; kiểm tra từng system call.
- [ ] B03: tích hợp parser → response → logger; đóng fd, xử lý client ngắt kết nối, timeout, SIGPIPE, EINTR.
- [ ] B04: -d chạy foreground tuần tự; chế độ thường daemonize và xử lý nhiều client (thống nhất fork); thu hồi tiến trình con.
- [ ] B05: Makefile tương thích make trên NetBSD/Linux/OmniOS, tích hợp nhánh, kiểm tra chạy từ thư mục khác, xử lý SIGTERM.
- [ ] B06: hỗ trợ Lê An phần fork/exec/waitpid và timeout CGI, có commit thực chất và review chéo.
Bằng chứng: -h, cổng sai, cổng đang dùng, IPv4/IPv6, hai client, debug/daemon.
Không giao hết code server cho một người: parser/response/CGI do các bạn tương ứng viết.

## Bảo — feature/bao-request
- [ ] R01: đọc request nhiều lần recv, kết thúc CRLF CRLF, giới hạn 16KiB và request line/URI.
- [ ] R02: tách method/target/version HTTP/1.0 và HTTP/1.1; luôn trả response HTTP/1.0; phân biệt GET, HEAD và phương thức không hỗ trợ; trả lỗi hợp lý.
- [ ] R03: header không phân biệt hoa thường; parse If-Modified-Since theo HTTP-date và quy ước giờ GMT.
- [ ] R05: hỗ trợ Lê An viết kiểm thử giao thức và CGI; có commit thực chất cho từng ca.
- [ ] R04: request thiếu dòng trống, bị chia gói, quá dài, header sai, client đóng giữa chừng; bổ sung unit test parser.
Phối hợp: raw target giữ nguyên cho files.c; không decode URI ở cả hai module.

## Việt An — feature/viet-an-response
- [ ] F01: ánh xạ URI trong document root, bỏ query khỏi đường dẫn file; chặn NUL và đường dẫn thoát root kể cả qua symlink. Theo đề, không bắt buộc hỗ trợ percent-encoding; không tự decode nhiều lần.
- [ ] F02: thư mục có index.html thì phục vụ; nếu không tạo listing sắp theo tên, bỏ dotfiles; escape HTML/URL.
- [ ] F03: ánh xạ ~user sang thư mục sws trong home theo manual; áp dụng boundary riêng cho thư mục được phép.
- [ ] F04: nhận diện Content-Type qua libmagic; thêm -lmagic vào Makefile.
- [ ] H01: status line HTTP/1.0 + CRLF; Date, Server, Last-Modified, Content-Type, Content-Length đúng.
- [ ] H02: GET gửi đủ file nhị phân theo byte; HEAD chỉ header; xử lý partial send/read và file fd.
- [ ] H03: If-Modified-Since và 304 theo RFC1945; các trang lỗi 400/403/404/500/501.
- [ ] H04: dispatch /cgi-bin sang cgi_respond nếu bật -c; đóng kết nối do Biên phụ trách.
Bằng chứng: HTML, ảnh thật, HEAD, file thiếu, listing, traversal, ~user, 304.

## Lê An — feature/le-an-web-test
- [ ] W01: hoàn thiện www/index.html, CSS, ảnh thật và giới thiệu của cả 4 bạn; sửa alt và đường dẫn ảnh.
- [ ] L01: logger.c: append định dạng IP timestamp-GMT "request-line" status Content-Length.
- [ ] L02: mặc định không log; -l ghi file; -d ưu tiên stdout; sanitize CR/LF và tránh log xen dòng khi nhiều process.
- [ ] C01: CGI theo RFC3875, chỉ bật bằng -c; xác thực executable nằm trong CGI root, không gọi shell từ URI.
- [ ] C02: fork/exec + pipe, environment CGI (REQUEST_METHOD, QUERY_STRING, SCRIPT_NAME, SERVER_PROTOCOL...), xử lý header CGI và HEAD.
- [ ] C03: timeout, waitpid, tiến trình lỗi, output quá lớn và CGI thiếu header.
- [ ] T01: mở rộng tests theo REQUIREMENTS.md; lưu lệnh/kết quả thật, không dùng server thay thế để tuyên bố sws chạy được.
- [ ] T02: README, ảnh chụp trình duyệt/curl phục vụ báo cáo, hướng dẫn demo và tổng hợp báo cáo kiểm thử.
Phối hợp: phần CGI có độ khó cao; Biên hỗ trợ tiến trình, Việt An hỗ trợ header.

## Mốc làm chung
1. Thống nhất include/http.h; build được trước khi chia nhánh.
2. GET trang nhóm + CSS + ảnh, HEAD, 404 trên socket thật; make test phải pass.
3. Hoàn thiện mọi tùy chọn, headers, conditional GET, listing, ~user, CGI, log.
4. Review chéo: Biên ↔ Bảo; Việt An ↔ Lê An; chạy lại toàn bộ trên Ubuntu.
