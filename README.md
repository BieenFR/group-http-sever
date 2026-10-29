# Phần Việt An — tài nguyên và HTTP response

Nhánh feature/viet-an-response kế thừa server Biên và parser Bảo.
Đã phục vụ HTML/CSS/ảnh, GET/HEAD/304/errors, directory index, đường dẫn ~user, bảo vệ root/symlink và MIME theo magic patterns qua file(1). CGI/logger còn stub ở nhánh này.

```bash
make test-response
make test
./sws -d -i 127.0.0.1 -p 8081 www
```
Mở http://localhost:8081. Trang nhóm dùng ảnh SVG giữ chỗ, chưa có ảnh thật. Terminal có thể báo logger chưa triển khai cho tới khi ghép phần Lê An.

## Cài trên Ubuntu
```bash
sudo apt update
sudo apt install build-essential python3 file git
make
```

Repo: https://github.com/BieenFR/group-http-sever (Private).
Đọc docs/TASKS.md, docs/API.md và docs/GIT_GUIDE.md trước khi sửa.
NetBSD 11.0 và OmniOS chưa được chạy thử; xem docs/PORTABILITY.md.
Các phần này được chuẩn bị với trợ giúp AI theo yêu cầu, không phải bằng chứng mỗi thành viên đã tự viết hoặc review code. Nhóm phải hiểu, kiểm thử, ghi nhận đóng góp thật trong COLLABORATION.md.
