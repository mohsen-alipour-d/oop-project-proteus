# راهنمای کار تیمی با Git و GitHub - پروژه Proteus

آدرس مخزن:
https://github.com/mohsen-alipour-d/oop-project-proteus

## 0) مفهوم‌های ساده

- Repository یا مخزن: پوشه پروژه روی GitHub که همه کدها و تاریخچه تغییرات در آن است.
- Clone: دانلود یک کپی کامل از مخزن روی دستگاه خودتان.
- Commit: ثبت یک عکس لحظه‌ای از تغییرات شما با یک پیام توصیفی.
- Push: فرستادن commit های خودتان از دستگاه‌تان به GitHub.
- Pull: گرفتن آخرین تغییرات بقیه اعضا از GitHub به دستگاه خودتان.
- Branch: شاخه کاری مستقل. هر نفر روی شاخه خودش کار می‌کند تا کدها روی هم خراب نشوند.
- main: شاخه اصلی. فقط نسخه تست‌شده و ادغام‌شده پروژه به آن می‌رسد. هیچ‌کس روی main کد نمی‌زند.

## 1) راه‌اندازی اولیه (فقط یک بار)

1) دعوت‌نامه مخزن را قبول کنید (از ایمیل یا لینکی که برایتان فرستاده شده).

2) برای خودتان Personal Access Token بسازید:
    - روی عکس پروفایل GitHub کلیک کنید: Settings
    - پایین منوی چپ: Developer settings
    - Personal access tokens سپس Tokens (classic)
    - Generate new token (classic)
    - در فرم: Note یک نام دلخواه، Expiration گزینه No expiration، و از لیست تیک‌ها فقط repo
    - Generate token را بزنید و توکن را فوراً در جای امن ذخیره کنید؛ فقط یک بار نمایش داده می‌شود.
    - توکن مثل رمز عبور است؛ آن را در چت عمومی به هیچ‌کس ندهید.

3) ترمینال را باز کنید و این دستورات را به ترتیب بزنید:

   git clone https://github.com/mohsen-alipour-d/oop-project-proteus.git
   cd oop-project-proteus
   git config --global push.autoSetupRemote true
   git config --global pull.rebase true

   توضیح: دستور اول مخزن را دانلود می‌کند، دومی وارد پوشه پروژه می‌شود،
   دومی و سومی تنظیمات یک‌بارمصرف برای راحت‌تر شدن push و pull هستند.

4) branch کاری خودتان را بسازید (نام دقیق هر نفر در جدول بخش 6 آمده است):

   git checkout -b feature/menu-canvas-mcu

5) پروژه را در CLion باز کنید:
   File سپس Open سپس انتخاب پوشه oop-project-proteus سپس Trust Project.
   صبر کنید تا CMake کامل لود شود، بعد یک بار با مثلث سبز Run کنید تا مطمئن شوید بیلد سالم است.

## 2) روتین هر جلسه کاری (به همین ترتیب، هر جلسه)

1) بررسی وضعیت و branch:

   git status

2) گرفتن آخرین تغییرات بقیه، قبل از شروع کار:

   git pull

3) کد زدن در CLion و حتماً Run کردن و تست کردن.

4) ثبت و فرستادن تغییرات در پایان کار:

   git add .
   git commit -m "add clock generator with adjustable period"
   git push

قانون طلایی: اول هر جلسه pull، آخر هر جلسه push.
هیچ‌وقت کد فقط روی دستگاه خودتان نماند.

## 3) قوانین commit (مستقیم روی نمره اثر دارد)

- commit ها باید کوچک، منظم و در طول توسعه باشند.
  آپلود یکجای همه فایل‌ها در آخر ترم (Bulk Upload) تخلف محسوب می‌شود و نمره را کم می‌کند.
- پیام commit باید معنادار و انگلیسی باشد و دقیقاً بگوید چه کاری انجام شده.
  درست: add wire routing with 90 degree paths
  غلط: update یا fix یا test یا .
- همه اعضا باید commit های خودشان را داشته باشند؛ فعالیت نداشتن نمره را کم می‌کند.
- هیچ‌کس مستقیم روی main کار و commit نمی‌زند.
- اگر فایل cpp جدیدی اضافه کردید، باید آن را به CMakeLists.txt هم اضافه کنید
  و در پیام commit به آن اشاره کنید.

## 4) قوانین کد (مستقیم روی نمره اثر دارد)

- Clean Code: نام کلاس‌ها، تابع‌ها و متغیرها باید دقیقاً عملکردشان را بازتاب دهند.
  کد نامرتب و نامفهوم، 50 درصد جریمه روی نمره نهایی دارد.
- هوش مصنوعی فقط نقش مشاور دارد. کپی مستقیم کد از AI تقلب محسوب می‌شود
  و کل گروه را جریمه می‌کند.
- هر فایل باید در پوشه درست خودش قرار بگیرد (بخش 5 را ببینید).

## 5) ساختار پروژه و جای فایل‌ها

    src/core                        کلاس‌های پایه مشترک (Component, Pin, Wire و...)
    src/components/sources          منابع تغذیه (GND, DC, Battery, Clock)
    src/components/passive          مقاومت، خازن، سلف
    src/components/interactive      کلید، دکمه، LED، سون‌سگمنت
    src/components/digital          گیت‌های منطقی و فلیپ‌فلاپ‌ها
    src/wiring                      سیستم سیم‌کشی و اتصالات
    src/measurement                 ابزارهای اندازه‌گیری
    src/simulation                  موتور شبیه‌سازی
    src/ui                          گرافیک و رابط کاربری
    docs                            اسناد پروژه از جمله همین راهنما

## 6) تقسیم کار و branch ها

| نفر | بخش‌های مسئول | branch |
| محسن | 5 و 6 و 9 | feature/backend-core |
| ملیکا | 1 و 2 و 7 | feature/menu-canvas-mcu |
| هانا | 3 و 4 و 8 | feature/editor-sim |
| مشترک | 10 و 11 | feature/file-drc |

درباره branch مشترک (feature/file-drc):
نفر اول آن را می‌سازد:

    git checkout -b feature/file-drc
    git push

نفر دوم بعد از git pull این دستور را می‌زند و روی همان می‌رود:

    git checkout feature/file-drc

قانون طلایی کار مشترک: قبل از شروع همیشه pull، بعد از پایان همیشه push.
هیچ‌وقت دو نفر همزمان و آفلاین روی این branch کار طولانی نکنند.

## 7) ادغام در main (merge)

وقتی یک زیربخش کامل شد و Run و تست موفق داشت، در چت گروه هماهنگ کنید و بعد:

    git checkout main
    git pull
    git merge feature/your-branch
    git push
    git checkout feature/your-branch

اگر موقع merge خطای conflict دیدید یعنی یک فایل را دو نفر همزمان تغییر داده‌اید.
فایل را در CLion باز کنید؛ بخش‌های بین علامت‌های <<<< و >>>> دو نسخه هستند؛
نسخه درست را نگه دارید، فایل را ذخیره کنید، سپس git add و git commit بزنید.

## 8) مشکلات رایج

- اگر push از شما Username و Password خواست:
  Username نام کاربری GitHub خودتان است و Password توکن خودتان است (نه رمز اکانت).
  موقع paste کردن توکن هیچ چیزی روی صفحه دیده نمی‌شود؛ این طبیعی است.
- خطای remote origin already exists یعنی remote قبلاً تنظیم شده؛
  دستور git remote add را دوباره نزنید.
- خطای not a git repository یعنی در پوشه اشتباهی هستید؛
  اول cd oop-project-proteus بزنید.
- اگر CLion فایل جدید را نشناخت، CMakeLists.txt را چک کنید و در صورت نیاز
  روی آن کلیک راست کرده و Reload CMake Project بزنید.