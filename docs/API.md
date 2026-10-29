# Hợp đồng giữa các module
Nguồn chuẩn: include/http.h. Không tự tạo struct trùng ở file .c.

Luồng tích hợp do Biên phụ trách:
1. Parse tham số, xác thực root, mở socket IPv4/IPv6, khởi tạo chế độ debug/daemon.
2. accept; lưu địa chỉ IP và thời điểm nhận kết nối.
3. request_read; nếu lỗi truyền error_status sang response_send.
4. response_send dispatch tài nguyên thường hoặc cgi_respond.
5. log_request với status và content_length; đóng client fd đúng một lần.

- server_run sở hữu listener/client fd; module response không tự đóng client fd.
- resource_open trả file fd; response_send chịu trách nhiệm đóng file fd.
- Mọi buffer có giới hạn. Không coi một recv là đủ request, một send là đủ response.
- Chấp nhận request HTTP/1.0 và HTTP/1.1, phản hồi luôn HTTP/1.0 và đóng kết nối.
- Chỉ cần URI tương đối theo đề; không bắt buộc percent-encoding hoặc kiểm tra reserved character.
- Các hàm chưa làm trả -1 với errno=ENOSYS. Đây không phải server hoạt động.
- request_read ghi lỗi giao thức vào error_status; truyền request=NULL nếu request không hợp lệ.
- response_result luôn được khởi tạo kể cả khi gửi lỗi; HEAD giữ Content-Length của GET nhưng không gửi body.
- status=0 của resource_open chỉ dành cho thư mục đã được xác thực cần listing.
- Việt An và Lê An phải thống nhất trách nhiệm header CGI: cgi_respond tạo HTTP response hoàn chỉnh từ output CGI.
- Biên quản lý timeout và xử lý SIGPIPE; response và CGI vẫn phải xử lý write lỗi.
- Đề xuất lỗi: 400 sai cú pháp; 403 không được truy cập; 404 không có file; 500 lỗi nội bộ; 501 phương thức chưa hỗ trợ. Đối chiếu RFC trước khi chốt.
