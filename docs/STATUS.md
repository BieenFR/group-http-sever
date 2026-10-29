# Trạng thái bản ghép

Các module đã triển khai, có kiểm thử server thật. Main chưa merge.
- Build C11 -Wall -Wextra -Wpedantic -Werror trên Linux.
- Parser: 15 case với socketpair và request.c thật.
- Logger: 5 nhóm kiểm tra, concurrent append thực tế bằng fork.
- HTTP: tests/test_team_http.py (server thật), thêm smoke test tests/http_acceptance.py.
- Socket/daemon: 12 case tests/test_bien.py; doubles được ghi rõ cho phần ghép module.
- Chưa thử NetBSD/OmniOS; không tuyên bố đã đạt toàn bộ đề.
- Ảnh thành viên còn là SVG giữ chỗ. ~user không tồn tại có kiểm tra; ~user home thật chưa kiểm tra.
- Git history giữ nguyên các commit trước; code được chuẩn bị theo yêu cầu, không dựng commit giả cho thành viên.

Kết quả kiểm tra trước khi push: 31 HTTP integration test, 15 parser case, 5 nhóm logger check và 12 socket/process test đạt trên Linux.
