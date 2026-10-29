# Group HTTP Server — Biên, Bảo, Việt An, Lê An
Bộ khung phân công để nhóm tự lập trình sws HTTP/1.0 bằng C/socket. Phát triển trên Ubuntu, nhưng phải biên dịch/chạy trên NetBSD 11.0, Linux và OmniOS theo đề. Chấp nhận cả request HTTP/1.1.

**Trạng thái: scaffold, CHƯA có HTTP server hoạt động.** main, header, Makefile và trang nhóm đã có; các hàm TODO trả lỗi ENOSYS. Không dùng gói này như bài nộp hoàn chỉnh.

## Bắt đầu
1. Clone repo https://github.com/BieenFR/group-http-sever.git và chọn nhánh tương ứng trong docs/GIT_GUIDE.md.
2. Đọc docs/members/ tương ứng và docs/TASKS.md.
3. Thống nhất docs/API.md cùng include/http.h trước khi code.
4. Làm mốc GET/HEAD/ảnh/404 trước, sau đó đủ ma trận docs/REQUIREMENTS.md.

## Chuẩn bị Ubuntu
```bash
sudo apt update
sudo apt install build-essential git python3 curl libmagic-dev
make
make check
```
`make check` build nghiêm ngặt, kiểm tra -h và liên kết asset; KHÔNG xác nhận server đã chạy.
```bash
make test
```
`make test` sẽ FAIL ở bộ khung hiện tại vì socket/parser/response chưa được viết. Khi hoàn thành mốc 1, bài kiểm tra này phải pass trên binary sws thật.

## Lệnh chạy sau khi triển khai
```bash
./sws -d -i 127.0.0.1 -p 8080 www
```
Mở http://localhost:8080. Terminal khác:
```bash
curl --http1.0 -v http://localhost:8080/
curl --http1.0 -I http://localhost:8080/
curl --http1.0 -v http://localhost:8080/images/bien.svg
curl --http1.0 -v http://localhost:8080/khong-ton-tai
```
Để kiểm tra ảnh JPEG, Lê An thêm ảnh thật và sửa index.html.
Chạy CGI sau khi đã viết C01–C03:
```bash
./sws -d -i 127.0.0.1 -p 8080 -c cgi-bin www
curl --http1.0 -v http://localhost:8080/cgi-bin/hello.sh
```

## Quy trình
Biên vận hành/integration; Bảo parser; Việt An tài nguyên/response; Lê An CGI/log/trang nhóm/test.
Chỉ merge sau review và báo cáo chạy thật. Các phần nâng cao còn thiếu không được bỏ qua nếu đề gốc yêu cầu.

## Tài liệu
- docs/TASKS.md — danh sách công việc đầy đủ theo phần.
- docs/API.md — hàm và quy ước ghép module.
- docs/REQUIREMENTS.md — ma trận yêu cầu và kiểm thử còn cần làm.
- docs/GIT_GUIDE.md — tạo repository, mời nhóm, chia nhánh, PR.
- docs/STATUS.md — kết quả kiểm tra bộ khung.

## Nguồn đối chiếu
Manual: https://stevens.netmeister.org/631/sws.1.pdf
Đề: https://stevens.netmeister.org/631/f26-group-project.html
Đã cập nhật yêu cầu theo đề đầy đủ người dùng cung cấp. Đọc docs/SUBMISSION.md và docs/PORTABILITY.md. Server vẫn chưa được triển khai; bộ này chưa phải bài nộp hoàn chỉnh.
