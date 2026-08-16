# Proteus OOP Simulator — Integrated Project

نسخهٔ یکپارچهٔ پروژهٔ شبیه‌ساز Proteus که رابط گرافیکی بخش‌های ۱ تا ۴ را به
Backend بخش‌های ۵، ۶، ۷، ۹، ۱۰ و ۱۱ وصل می‌کند. پروژه فقط یک `main.cpp` دارد
و کدها به Targetهای مستقل CMake تقسیم شده‌اند تا کار تیمی و Merge در Git کمترین
تداخل را داشته باشد.

## ساختار پروژه

| مسیر | مسئولیت |
|---|---|
| `src/main.cpp` | تنها Entry Point پروژه؛ بخش‌ها با کامنت جدا شده‌اند |
| `src/ui` | منو، Canvas، کتابخانهٔ قطعات و ویرایش گرافیکی — بخش‌های ۱ تا ۴ |
| `src/integration` | تبدیل رویدادهای UI به عملیات Backend و همگام‌سازی دو لایه |
| `src/core` | `Component`، `Pin` و `Circuit` |
| `src/wiring` | Wire، Junction و Net — بخش‌های سیم‌کشی |
| `src/components` | قطعات آنالوگ، دیجیتال، تعاملی و پیشرفته |
| `src/mcu` | MCU، حافظه، رجیستر، Firmware loader و Decoder — بخش ۷ |
| `src/measurement` | Voltmeter، Ammeter، Probe و Oscilloscope |
| `src/file` | Save/Load، Recent Projects و Undo/Redo |
| `src/drc` | بررسی اتصال کوتاه، ورودی شناور و Ground |
| `tests` | تست‌های Backend و تست اتصال Frontend/Backend |

سه Target اصلی CMake عبارت‌اند از:

- `proteus_backend`: تمام کدهای Backend، بدون وابستگی گرافیکی.
- `proteus_integration`: Adapter و مدل قطعات UI؛ قابل تست بدون SDL.
- `ProteusOopSimulator`: برنامهٔ گرافیکی نهایی.

این تفکیک باعث می‌شود تغییرات Frontend، Backend و Integration در فایل‌های جدا
انجام شوند. فایل‌های خروجی کاربر، Build و تنظیمات IDE نیز در `.gitignore` هستند.

## قابلیت‌های متصل‌شده

- ساخت، حذف، جابه‌جایی، Rotate، Mirror و ویرایش قطعات از UI و ثبت مستقیم در `Circuit`
- کتابخانهٔ کامل قطعات پایه، دیجیتال، ADC/DAC، MCU، حافظه، LCD، Keypad و ابزار اندازه‌گیری
- ساخت Wire با کلیک روی دو Pin، انتخاب و حذف Wire و افزودن Junction
- رسم سیم‌ها با رنگ وضعیت منطقی `LOW`، `HIGH` و `UNDEFINED`
- Save/Load و Recent Project از طریق `FileManager`
- Undo/Redo مبتنی بر Snapshot بعد از هر تغییر معتبر کاربر
- DRC قبل از شروع شبیه‌سازی
- Run/Pause/Stop و اجرای گام‌های Backend در حلقهٔ اصلی UI

## کنترل‌های مهم

- قرار دادن قطعه: قطعه را از **Active Components** انتخاب و روی Canvas کلیک کنید.
- سیم‌کشی: روی Pin اول و سپس Pin دوم کلیک کنید.
- انتخاب Wire: روی سیم کلیک کنید و `Delete` بزنید.
- Junction: نشانگر موس را روی تقاطع دو سیم ببرید و `J` بزنید.
- `Ctrl+S`: ذخیره در `proteus_project.txt`.
- `Ctrl+Z` / `Ctrl+Y`: Undo / Redo.
- `R`، `H`، `V`: Rotate و Mirror قطعات انتخاب‌شده.
- دکمه‌های `DRC`، `RUN/PAUSE` و `STOP`: کنترل بررسی و شبیه‌سازی.

## وارد کردن و Build در CLion روی Windows

### ۱. تنظیم Compiler

در CLion به مسیر زیر بروید:

`File > Settings > Build, Execution, Deployment > Toolchains`

یک Toolchain از نوع **MinGW** بسازید و در `Toolset` گزینهٔ **Bundled MinGW**
یا مسیر یک MinGW سالم را انتخاب کنید. صبر کنید تا CMake، C Compiler، C++ Compiler،
Debugger و Build Tool همگی شناسایی شوند. اگر کنار `g++.exe` خطا می‌بینید، پروژه هنوز
قابل Build نیست و باید همین Toolchain را اصلاح کنید.

### ۲. نصب SDL2 و SDL2_gfx با vcpkg

در PowerShell یا CMD:

```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg.exe install sdl2:x64-mingw-static sdl2-gfx:x64-mingw-static
```

اگر از Toolchain نوع Visual Studio/MSVC استفاده می‌کنید، به‌جای Triplet بالا از
`x64-windows` استفاده کنید. Compiler و Triplet باید با هم سازگار باشند.

### ۳. باز کردن پروژه

ZIP را Extract کنید. در CLion گزینهٔ `File > Open` را بزنید و دقیقاً پوشه‌ای را
انتخاب کنید که فایل `CMakeLists.txt` در ریشهٔ آن قرار دارد:

```text
ProteusOopIntegrated/
├── CMakeLists.txt
├── README.md
├── src/
└── tests/
```

پوشهٔ `src` یا خود فایل `main.cpp` را جداگانه باز نکنید.

### ۴. معرفی vcpkg به CMake

به مسیر زیر بروید:

`File > Settings > Build, Execution, Deployment > CMake`

در Profile مربوط به Debug، Toolchain مرحلهٔ ۱ را انتخاب و در **CMake options** بنویسید:

```text
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-static
```

سپس `Reload CMake Project` را اجرا کنید. اگر مسیر vcpkg شما متفاوت است، همان مسیر
واقعی را جایگزین کنید.

### ۵. Build و Run

از لیست Run Configuration بالای CLion، Target زیر را انتخاب کنید:

```text
ProteusOopSimulator
```

سپس `Build > Build Project` و بعد Run را بزنید. برای تست‌ها نیز Targetهای زیر در
همان لیست موجودند:

```text
run_tests
run_integration_tests
```

## Build از Terminal

برای MinGW و vcpkg:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static
cmake --build build
ctest --test-dir build --output-on-failure
```

اگر فقط Backend را می‌خواهید و SDL نصب نیست:

```powershell
cmake -S . -B build-backend -DPROTEUS_BUILD_GUI=OFF
cmake --build build-backend
ctest --test-dir build-backend --output-on-failure
```

راهنمای رسمی مرتبط:

- [باز کردن پروژهٔ CMake در CLion](https://www.jetbrains.com/help/clion/clion-quick-start-guide.html)
- [تنظیم MinGW در CLion](https://www.jetbrains.com/help/clion/quick-tutorial-on-configuring-clion-on-windows.html)
- [اتصال vcpkg به CMake](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration)
