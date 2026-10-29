# Repository chung đã được chọn

Repository: https://github.com/BieenFR/group-http-sever (Private).

Mỗi thành viên clone và chọn nhánh của mình, ví dụ Biên:
```bash
git clone https://github.com/BieenFR/group-http-sever.git
cd group-http-sever
git switch feature/bien-server
```

| Người | Nhánh |
|---|---|
| Biên | feature/bien-server |
| Bảo | feature/bao-request |
| Việt An | feature/viet-an-response |
| Lê An | feature/le-an-web-test |

Chủ repo vào Settings → Collaborators để mời tài khoản của các bạn; tên tài khoản chưa được cung cấp nên chưa gửi lời mời.
Phần hướng dẫn tạo repo bên dưới chỉ để tham khảo nếu tạo dự án khác; không chạy git init hoặc thêm origin lại trong repo đã clone.

# Tạo GitHub và chia nhánh
## Người tạo repository
1. Giải nén project. Tạo repository GitHub tên group-http-server, Private, không chọn README/.gitignore/license tự động (bộ này đã có README).
2. GitHub → Settings → Collaborators → mời tài khoản của 3 bạn còn lại. Không cần gửi mật khẩu/token cho nhau.
3. Mở terminal ngay thư mục đã giải nén, thay TEN_TAI_KHOAN:
```bash
git init -b main
git add .
git commit -m "chore: add HTTP server team scaffold"
git remote add origin https://github.com/TEN_TAI_KHOAN/group-http-server.git
git push -u origin main
```
Đây là các lệnh nhóm tự chạy khi đã kiểm tra bộ khung; chưa có commit/push nào được thực hiện sẵn.
Nếu git thiếu danh tính, cấu hình tên/email của chính mình bằng git config user.name và user.email trước khi commit.
Không chạy git init trong một repository khác đã tồn tại.

## Mỗi thành viên
```bash
git clone https://github.com/TEN_TAI_KHOAN/group-http-server.git
cd group-http-server
git switch -c feature/bien-server
```
Thay nhánh trên bằng nhánh của mình:
- Biên: feature/bien-server
- Bảo: feature/bao-request
- Việt An: feature/viet-an-response
- Lê An: feature/le-an-web-test

Trước khi sửa, cập nhật main khi working tree sạch:
```bash
git fetch origin
git merge origin/main
```
Sau khi làm và kiểm tra, stage đúng file mình sửa, ví dụ Biên:
```bash
make check
git status
git add src/config.c src/server.c
git commit -m "feat: implement server startup and socket loop"
git push -u origin feature/bien-server
```
Mỗi bạn thay tên file và nhánh cho đúng; không commit thông báo đã làm khi code vẫn TODO.
Tạo Pull Request từ nhánh mình vào main; điền template, nhờ một bạn review rồi merge.

## Tránh xung đột
- Không sửa file của bạn khác mà chưa thống nhất; include/http.h cần review chung.
- Trước mỗi PR, merge origin/main vào nhánh cá nhân, xử lý conflict và chạy lại check.
- Không git push --force; không đưa token, log cá nhân hay thư mục build lên Git.
- Nếu Git báo conflict, sửa các vùng <<<<<<< / ======= / >>>>>>> rồi git add file và git commit.
- Không xóa thay đổi chưa commit để cập nhật nhánh; commit/stash công việc trước.

## Tạo công việc trên GitHub
Tạo 4 Issue với nội dung từ docs/members tương ứng, assign đúng người, mỗi issue liên kết TASKS.md.
Có thể tạo labels: server, request, response, web-test, blocked, ready-for-review.
Chưa có repo online, Issue hoặc nhánh remote nào được tạo tự động trong gói này.
