# Hướng dẫn thiết lập & build (tập trung các script)

Mục tiêu: gom các script hiện có (`tools/*.ps1`, `scripts/*.ps1`) thành một hướng dẫn duy nhất để dễ theo dõi.

**Yêu cầu trước:**
- Windows 10/11 (hoặc tương đương)
- `git`, `cmake`, `ninja` (hoặc để Conan cài đặt), `python` (nếu dùng `python -m conan`)
- Quyền chạy PowerShell scripts (ExecutionPolicy)

**1) Build nhanh (hướng dẫn ngắn gọn)**
- Chạy bước cài dependencies bằng Conan (tạo folder `build`) :

PowerShell:

    .\scripts\build.ps1 -SetupConan

- Build project (config Release, generator Ninja):

PowerShell:

    .\scripts\build.ps1 -BuildDir build -Config Release

`scripts/build.ps1` đã tự động xử lý `conan install` nếu cần và sử dụng file toolchain do Conan tạo ra (nếu có). Nếu `conan` không có trong PATH, script sẽ cố gọi `python -m conan`.

**2) Nếu bạn cần MinGW (winlibs) — tự động tải & cài**
- Tự động tải và giải nén winlibs (nếu chưa có):

    .\tools\install_mingw.ps1

Hoặc nếu bạn đã có một winlibs zip, đặt file đó tại `tools\winlibs.zip` rồi chạy script.

**3) Nếu muốn khôi phục MinGW từ hệ (copy portable install vào `tools\mingw-w64`)**
- Dùng:

    .\tools\restore_mingw.ps1

Script sẽ tìm `g++.exe` trên hệ hoặc copy các thư mục `usr`/`mingw64`/`bin` phù hợp sang `tools\mingw-w64`.

**4) Tìm compiler có sẵn trên máy**
- Dùng:

    .\tools\find_compiler.ps1

Script liệt kê các executable compiler (g++, clang++, cl) tìm thấy và các vị trí phổ biến.

**5) Ghi chú & best-practices (ngắn)**
- Tôi đã gom các file tạm, báo cáo, và build artifacts vào `archive/cleanup-backup/` trước khi xóa.
- `.gitignore` đã được cập nhật để tránh commit các file report và thư mục build.

**6) Tích hợp thành một script duy nhất**
- Đã merge logic từ `tools/find_compiler.ps1`, `tools/install_mingw.ps1`, `tools/restore_mingw.ps1` và `tools/setup_conan.ps1` trực tiếp vào `scripts\\build.ps1`.
- Bạn chỉ cần sử dụng `scripts\\build.ps1` cho mọi bước build/setup cơ bản; các script gốc đã được lưu trong `archive/cleanup-backup/`.
 - Đã merge logic từ `tools/find_compiler.ps1`, `tools/install_mingw.ps1`, `tools/restore_mingw.ps1` và `tools/setup_conan.ps1` trực tiếp vào `scripts\\build.ps1`.
 - Bạn chỉ cần sử dụng `scripts\\build.ps1` cho mọi bước build/setup cơ bản; các script gốc đã được lưu trong `archive/cleanup-backup/`.

### 7) Hỗ trợ đa nền tảng và CI
 - Có sẵn GitHub Actions workflow tại `.github/workflows/ci.yml` để chạy build và tests trên `ubuntu-latest`, `macos-latest` và `windows-latest`.
 - Thư mục `conan/profiles/` chứa các profile mẫu: `windows_mingw`, `windows_msvc`, `linux_gcc`, `macos_clang`.
 - Một Dockerfile mẫu `tools/Dockerfile` có sẵn để tạo môi trường build Linux tái tạo được.

Ví dụ chạy cục bộ (Linux/macOS):
```bash
python -m conan install . --profile conan/profiles/linux_gcc --output-folder build --build=missing
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build/tests --output-on-failure
```

# Quickstart — build & test (ngắn gọn)

Prerequisites: `git`, `python3`, `cmake`, `ninja` (Conan will be used).

Windows (PowerShell):
```powershell
git clone <repo>
cd repo
.\scripts\build.ps1 -SetupConan         # installs deps via Conan
.\scripts\build.ps1 -BuildDir build -Config Release -RunTests
```

Linux / macOS (bash):
```bash
git clone <repo>
cd repo
python3 -m pip install --user conan
python3 -m conan install . --output-folder build --build=missing
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build/tests --output-on-failure
```

Notes:
- `scripts/build.ps1` includes helpers to detect/install MinGW and to run tests with required PATH adjustments.
- For CI and reproducible builds see `.github/workflows/ci.yml`, `conan/profiles/` and `tools/Dockerfile`.
- If tests fail with missing DLLs on Windows, run the PowerShell build with `-RunTests` or ensure `tools/mingw-w64/bin` is on PATH.

That's it — the above commands are copy-paste ready.
