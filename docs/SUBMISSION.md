# Quy định nộp theo đề được cung cấp

- Hạn ghi trong đề: 2026-12-14 15:59:59. Đề không ghi múi giờ; xác nhận với giảng viên trước khi quy đổi sang giờ Việt Nam.
- Định dạng: tar không nén, tên user1-user2-user3-user4.tar, giải nén thành thư mục cùng tên.
- user1... là username thật của thành viên, không tự thay bằng tên gọi Biên/Bảo nếu đó không phải username.
- Gồm source, Makefile, README, checklist và git log.
- README phải nêu ai hoàn thành phần nào, cách phối hợp, đóng góp của từng người.
- Gửi từ email @stevens.edu đến jschauma@stevens.edu, subject [CS631] group project — đây là hướng dẫn của đề nguồn; nếu lớp của nhóm dùng cách nộp khác, xác nhận với giảng viên lớp.
- Không tự gửi email trong bộ khung này.

## Phân công và lịch sử
Mỗi người dùng danh tính Git của chính mình và tạo commit cho công việc thật. Không tạo commit giả hoặc chia lịch sử giả cho đủ người. Không squash toàn bộ mất dấu tác giả khi chuẩn bị git log.
Đóng góp cần tương đương về thực chất, không chỉ số commit/dòng code; hỗ trợ và review được ghi lại. Biên giúp CGI/process; Bảo giúp kiểm thử; Việt An/Lê An review chéo tài nguyên/CGI.
Điền COLLABORATION.md khi làm thật, sau đó đưa tổng kết vào README.

## Đóng gói thủ công sau khi hoàn thiện
Tại repository, khi không còn thay đổi chưa lưu, thay bốn USER bằng username thật:
```sh
python3 tools/package_submission.py USER1 USER2 USER3 USER4
```
Script lấy file đã track từ HEAD, thêm git log và tạo tar trong submission/. Không commit/push tự động. Không đưa thay đổi chưa commit vào bài nộp. Kiểm tra tar trước khi gửi.
