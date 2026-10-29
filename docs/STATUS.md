# Trạng thái nhánh Việt An

Kế thừa Biên + Bảo; HTTP file tĩnh đã hoạt động. CGI/logger còn stub.
make test-response kiểm tra server thật, không dùng doubles.
MIME phụ thuộc file(1) có --mime-type. Chưa kiểm tra NetBSD/OmniOS; chưa merge main.

Kết quả kiểm tra trước khi push: 19 HTTP static test và smoke GET/HEAD/ảnh/404/HTTP1.1 đạt trên Linux.
