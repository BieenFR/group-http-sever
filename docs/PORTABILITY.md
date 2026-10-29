# Tương thích nền tảng

Bắt buộc kiểm tra thật trên NetBSD 11.0, linux.cs631.dotwtf.wtf và omnios.cs631.dotwtf.wtf.
Không tuyên bố pass các nền tảng này chỉ vì chạy được Ubuntu.

- Makefile dùng danh sách object và suffix rule, tránh GNU-only wildcard/pattern rules.
- Dùng POSIX socket/process API; không phụ thuộc epoll, /proc, accept4 hoặc Linux-only extensions.
- Dùng getaddrinfo/getnameinfo, socklen_t, ssize_t, sigaction, waitpid.
- Kiểm tra khả năng hỗ trợ IPv6 và cấu hình IPV6_V6ONLY, tránh hai listener tranh cùng cổng.
- OmniOS có thể cần make LDLIBS="-lsocket -lnsl"; xác minh trên máy thật, không giả định đã thử.
- MIME hiện dùng file(1) có --mime-type (magic patterns), không liên kết libmagic trực tiếp. Nếu hệ OmniOS thiếu flags này, cài bản file/libmagic tương thích và cấu hình macro SWS_FILE_PROGRAM; phải kiểm tra trên máy thật.
- Nếu dùng libmagic, kiểm tra header/library trên từng hệ; cấu hình CPPFLAGS/LDFLAGS/LDLIBS qua make. Không hardcode đường dẫn Ubuntu.
- Không coi timegm/strptime có cùng khả năng hiển thị trên mọi hệ; kiểm tra header và feature macros, bảo toàn GMT.
- Không chdir làm mất document root/log/CGI root tương đối khi daemonize: resolve cấu hình trước.
- Python là công cụ test, không phải dependency để chạy sws.

| Hệ | Build | HTTP tests | Daemon/CGI/IPv6 | Trạng thái |
|---|---|---|---|---|
| Ubuntu môi trường chuẩn bị | make check PASS | FAIL do TODO | Chưa chạy | Chỉ xác minh bộ khung |
| NetBSD 11.0 | Chưa chạy | Chưa chạy | Chưa chạy | TODO |
| Linux host của môn | Chưa chạy | Chưa chạy | Chưa chạy | TODO |
| OmniOS host của môn | Chưa chạy | Chưa chạy | Chưa chạy | TODO |
