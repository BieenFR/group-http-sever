# Trạng thái nhánh Bảo

Config/socket/daemon kế thừa commit Biên. Request parser đã viết; response/CGI/logger còn stub ở nhánh này.
Chạy make check, make test-request và make test-bien. Xem docs/members/BAO.md.
Chưa xác minh NetBSD/OmniOS; chưa merge main.

Kết quả kiểm tra trước khi push: 15 parser case và 12 socket/process test đạt trên Linux.
