# Đối chiếu yêu cầu và kiểm thử
Nguồn manual đã đối chiếu: https://stevens.netmeister.org/631/sws.1.pdf
Trang đề: https://stevens.netmeister.org/631/f26-group-project.html
Đã cập nhật theo nội dung đầy đủ người dùng cung cấp. Chọn C/socket để triển khai; đoạn đề cung cấp không quy định riêng ngôn ngữ. Nền tảng bắt buộc: NetBSD 11.0, linux.cs631.dotwtf.wtf, omnios.cs631.dotwtf.wtf. Chưa được truy cập các máy chấm.
Trang hw và checklist chính thức chưa truy xuất được; phải mở và đối chiếu trước khi nộp.
RFC1945: https://www.rfc-editor.org/rfc/rfc1945
RFC3875: https://www.rfc-editor.org/rfc/rfc3875

| Hạng mục theo manual | Chủ trì | Nghiệm thu cần bổ sung/chạy |
|---|---|---|
| -h, -p, document root | Biên | help; port mặc định 8080; port sai; root sai |
| -i và mặc định IPv4/IPv6 | Biên | bind từng loại địa chỉ, không mặc định chỉ loopback |
| -d / daemon | Biên | foreground tuần tự; thường chạy nền và xử lý client |
| HTTP/1.1 request | Bảo + Việt An | nhận HTTP/1.1, trả HTTP/1.0, đóng kết nối |
| GET / HEAD | Bảo + Việt An | request chia gói; body GET đúng byte; HEAD không body |
| If-Modified-Since | Bảo + Việt An | cũ/mới/ngày sai; 304 không body |
| Headers và MIME magic | Việt An | Date/Server/Last-Modified/Type/Length; ảnh nhị phân |
| Directory indexing | Việt An | có/không index; thứ tự tên; bỏ dotfiles; escape |
| ~user | Việt An | user tồn tại/không tồn tại; không thoát sws root |
| -c /cgi-bin + RFC3875 | Lê An | env/query/header, CGI lỗi, HEAD, chỉ bật khi cho phép |
| -l / debug stdout | Lê An | append, thời gian GMT, IP và request chính xác |
| Exit code | Biên | thành công 0, lỗi >0 |
| An toàn tài nguyên | Cả nhóm | traversal/symlink; giới hạn; ngắt kết nối; timeout |

Không bắt buộc percent-encoded URI hoặc kiểm tra reserved character theo phần Additional considerations.

make check chỉ kiểm tra build/help/asset. make test hiện có các ca GET, HEAD, ảnh nhị phân, 404; chưa thay thế ma trận này.
Không đánh dấu hoàn thành khi chỉ make check thành công.
