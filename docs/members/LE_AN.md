# Lê An — bàn giao để review

Nhánh feature/le-an-web-test kế thừa Việt An → Bảo → Biên, là bản ghép có thể chạy website.
File: src/cgi.c, src/logger.c, www/, tests/test_logger.*, tests/test_team_http.py, tài liệu.
- CGI chỉ bật với -c; fexecve executable đã mở an toàn, không dựng shell command từ URI.
- CGI environment, PATH_INFO/query, status/content type/location/extension fields, local/external redirect, binary/HEAD.
- Timeout 5s, output tối đa 4MiB, process group, thu hồi child; CGI lỗi/malformed output trả 502/504.
- Logger mặc định tắt, -d stdout ưu tiên -l; append/lock file giữa worker, GMT, escape quote/control.
- Trang nhóm dùng ảnh initials SVG giữ chỗ vì chưa có ảnh thật được cung cấp.
Chạy make test-full và make test-bien. NetBSD/OmniOS và ~user tồn tại chưa kiểm tra.
