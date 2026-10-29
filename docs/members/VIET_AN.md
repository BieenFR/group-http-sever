# Việt An — bàn giao để review

Nhánh feature/viet-an-response, kế thừa Bảo + Biên để kiểm tra tài nguyên thật.
File: src/files.c, src/response.c, include/response_internal.h. include/http.h cập nhật hợp đồng directory fd để tránh mở lại đường dẫn sau khi kiểm tra.
- GET/HEAD, Date GMT, Server, Last-Modified, Content-Type, Content-Length, status/errors, conditional GET.
- MIME dùng file(1) đọc từ fd đã xác thực, không gọi shell; ảnh giả đuôi .png vẫn được nhận diện text/plain. CSS/JS text được gán đúng web MIME.
- Directory index sort tên, bỏ dotfiles, escape HTML/URL, hỗ trợ tên có dấu cách.
- Canonical root boundary + openat từng component O_NOFOLLOW; symlink chỉ được resolve trong root.
- Percent decode đúng một lần (phần bổ sung ngoài mức bắt buộc đề); chặn NUL/control/traversal.
- ~user tra home bằng getpwnam; nhánh directory trả fd trực tiếp cho response.
- Không phục vụ source CGI qua alias tĩnh khi CGI root nằm dưới document root.
Cần file(1) hỗ trợ --mime-type; OmniOS có thể cần bản file/libmagic trong package bổ sung. Chưa kiểm tra nền tảng này.
Chạy make test-response và make test. CGI/logger sẽ được ghép từ Lê An.
