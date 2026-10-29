# Nhóm HTTP server — bản ghép đầy đủ để review

Nhánh feature/le-an-web-test kế thừa code Biên → Bảo → Việt An, rồi thêm CGI/logger của Lê An. main chưa thay đổi.
HTTP server hiện phục vụ được trang nhóm và ảnh SVG qua socket thật.

```bash
make test-full
make test-bien
./sws -d -i 127.0.0.1 -p 8081 -c "$PWD/cgi-bin" "$PWD/www"
```
Mở **http://localhost:8081**. CGI demo: http://localhost:8081/cgi-bin/hello.sh.
Bấm Ctrl+C để dừng. Chạy nền (không -d):
```bash
./sws -i 127.0.0.1 -p 8081 -l "$PWD/requests.log" -c "$PWD/cgi-bin" "$PWD/www"
```
Server detach, stdout/stderr về /dev/null; log request append vào file. Dùng PID xác định từ ss/lsof trên máy thật để gửi SIGTERM khi cần dừng.
Ảnh hiện là ảnh SVG giữ chỗ, chưa phải ảnh thật của thành viên. Thay theo www/images/README.md.

## Kiểm thử
make test-full chạy parser, logger và giao thức HTTP thật (GET/HEAD/1.1, MIME, 304, directory, traversal, CGI/env/redirect/binary/timeout/giới hạn output). make test-bien kiểm tra socket/daemon, với driver đôi thử nghiệm được ghi rõ.
Dùng CGI từ thư mục mà nhóm tin cậy. CGI có timeout 5 giây, output tối đa 4MiB; request tổng tối đa 15 giây.

## Phân công và giới hạn
- Biên: config/socket/daemon, ghép module, dừng process.
- Bảo: parse request/HTTP-date.
- Việt An: files/headers/body/MIME/listing/~user.
- Lê An: CGI/logger/trang nhóm/test/report.
Các tên thể hiện phần được giao, không giả định người đó đã tự review hoặc hiểu hết code. Ghi đóng góp thực tế sau khi mỗi người kiểm tra.
Chưa xác minh trên NetBSD/OmniOS, chưa kiểm tra ~user thành công với home thật, chưa coi là đủ điều kiện nộp.

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
