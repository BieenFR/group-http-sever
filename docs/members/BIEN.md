# Biên — phần đã triển khai

Nhánh đích: feature/bien-server. Bản bàn giao trên nhánh feature/bien-server để review.

- src/config.c: getopt, validate port/root/CGI, đường dẫn tuyệt đối.
- src/server.c: listener IPv4/IPv6, debug tuần tự, daemon double fork, worker fork, timeout, đóng fd và shutdown.
- Makefile: thêm make test-bien, giữ cú pháp make cơ bản.
- tests/test_bien.py + tests/fixtures/server_modules.c: kiểm tra thật socket/process với module đôi thử nghiệm tách khỏi binary production.

Đọc docs/BIEN_REPORT.md để biết phần đã xác minh và phần còn phụ thuộc các bạn.
