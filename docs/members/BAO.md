# Bảo — bàn giao để review

Nhánh feature/bao-request, kế thừa server của Biên.
File: src/request.c; tests/test_request.c + test_request.py. Cập nhật smoke test của Biên để gửi request hoàn chỉnh khi parser đã hoạt động.
15 ca parser trên socketpair với code thật. Không phải parser giả. Nhận request chia gói, header tối đa 16KiB, URI tối đa theo HTTP_MAX_TARGET, kiểm tra control/NUL, duplicate date; hỗ trợ 3 dạng HTTP-date theo RFC1945.
If-Modified-Since không hợp lệ được bỏ qua; phương thức chưa hỗ trợ được chuyển sang response trả 501.
HTTP/1.1 được nhận nhưng response luôn do module Việt An trả HTTP/1.0. CGI và logger không thuộc nhánh này.
