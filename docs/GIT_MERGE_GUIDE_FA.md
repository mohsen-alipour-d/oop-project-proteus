# راهنمای کار تیمی بدون Conflict

## قانون فایل‌ها

- فقط `src/main.cpp` Entry Point است؛ هیچ عضو تیم فایل `main` دوم نسازد.
- تغییرات رابط گرافیکی فقط در `src/ui` انجام شوند.
- تغییرات موتور مدار، قطعات و فایل/DRC در پوشهٔ Backend مربوط به خودشان انجام شوند.
- کدی که باید UI و Backend را به هم وصل کند فقط در `src/integration` قرار بگیرد.
- برای افزودن فایل جدید به Build فقط `CMakeLists.txt` تغییر کند؛ نام فایل موجود عوض نشود.

## روال پیشنهادی هر Feature

```bash
git switch main
git pull --ff-only
git switch -c feature/short-feature-name

# edit, build and test
git add <only-the-files-you-changed>
git commit -m "feat: describe the feature"

git fetch origin
git rebase origin/main
git push -u origin feature/short-feature-name
```

بعد از سبز شدن `run_tests` و `run_integration_tests`، Branch از طریق Pull Request
Merge شود. فایل‌های `cmake-build-*`، `.idea`، `recent_projects.txt` و
`proteus_project.txt` نباید Commit شوند؛ این موارد در `.gitignore` قرار دارند.

## هنگام Conflict

قبل از انتخاب `ours` یا `theirs`، هر دو نسخه را بخوانید. در `CMakeLists.txt` معمولاً
باید Sourceهای هر دو Branch در Target درست حفظ شوند. در `src/integration` نیز باید
هر دو رفتار Frontend و Backend باقی بمانند. بعد از Resolve حتماً هر دو تست اجرا شوند.
