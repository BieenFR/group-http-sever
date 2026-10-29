# Phần Bảo — HTTP request parser

Nhánh feature/bao-request kế thừa code server của Biên (feature/bien-server).
Đã viết request.c: bounded recv, request line, HTTP/1.0/1.1, GET/HEAD/method khác, headers và HTTP-date. Chưa có response/logger/CGI thật trên nhánh này.

```bash
make test-request
make test-bien
```

Chưa thể mở trang web từ nhánh Bảo cho tới khi ghép phần response của Việt An.

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
