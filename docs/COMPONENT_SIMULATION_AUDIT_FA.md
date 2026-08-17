# گزارش ممیزی موتور شبیه‌سازی و کامپوننت‌ها

## منشأ خطای FLOATING

در نسخهٔ قبلی، `Circuit::propagateVoltages()` فقط Netی را معتبر می‌دانست که
حداقل یک `Pin::drivesNet()` روی همان Net داشته باشد. این منطق برای مدار دیجیتال
جهت‌دار مناسب بود، اما برای مدار آنالوگ صحیح نبود؛ چون دو طرف Resistor،
Capacitor، Inductor، LED، Switch و ابزار اندازه‌گیری دو Net متفاوت هستند و
ولتاژ سمت دوم باید از حل KCL/KVL به دست آید، نه از یک Output مستقیم.

سه مفهوم اکنون جدا هستند:

1. `Unconnected pin`: پین اصلاً سیم ندارد و DRC شمارهٔ پین و نام قطعه را می‌دهد.
2. `Unreferenced electrical island`: همهٔ پین‌ها سیم دارند، اما آن بخش مدار به
   GND یا یک خروجی دارای مرجع متصل نیست؛ DRC قبل از Run آن را رد می‌کند.
3. `FLOATING`: فقط Net واقعاً حل‌نشده در زمان اجرا؛ نه هر Net فاقد Output مستقیم.

## حل آنالوگ اضافه‌شده

`src/simulation/analog_solver.cpp` از Modified Nodal Analysis ساده‌شده استفاده
می‌کند. منابع ولتاژ، مقاومت‌ها، مدل Backward Euler خازن و سلف، مدل قطعه‌ای LED،
کلید بسته و مقاومت داخلی ابزارها در ماتریس مدار Stamp می‌شوند. خروجی‌های دیجیتال
هم به‌عنوان Driver به همین حل تزریق می‌شوند؛ بنابراین موتور Mixed-Signal باقی
می‌ماند و تست‌های ADC/DAC/MCU قبلی نیز حفظ می‌شوند.

## نتیجهٔ ممیزی ۲۵ تعریف UI

| ID | کامپوننت | مدل شبیه‌سازی/اصلاح |
|---:|---|---|
| 0 | Resistor | Stamp رسانایی، محافظ مقدار صفر، جریان دوطرفه |
| 1 | Capacitor | Backward Euler، حفظ ولتاژ حالت، حذف تقسیم بر `dt=0` |
| 2 | Battery | منبع ولتاژ دوترمیناله؛ Pin 0 منفی و Pin 1 مثبت مطابق UI |
| 3 | Clock | خروجی ولتاژ نسبت به پین GND Reference، نه دو Output مستقل |
| 4 | Switch | مدار باز = قطع؛ مدار بسته = اتصال دوطرفه با مقاومت بسیار کم |
| 5 | AND Gate | ورودی دیجیتال، خروجی Driver و تأخیر انتشار |
| 6 | LED | مدل قطعه‌ای با ولتاژ آستانه و مقاومت روشن |
| 7 | Ground | مرجع صفر ولت و Anchor جزیرهٔ الکتریکی |
| 8 | Inductor | مدل Backward Euler و حفظ جریان حالت |
| 9 | DC Source | منبع دوترمیناله با قطب‌بندی هماهنگ UI |
| 10 | Push Button | خروجی 0/5V و تعامل زنده در Run/Pause |
| 11 | 7 Segment | هشت ورودی دیجیتال و Decode وضعیت Segmentها |
| 12 | OR Gate | ورودی دیجیتال، خروجی Driver و تأخیر انتشار |
| 13 | NOT Gate | ورودی دیجیتال، خروجی Driver و تأخیر انتشار |
| 14 | XOR Gate | ورودی دیجیتال، خروجی Driver و تأخیر انتشار |
| 15 | NAND Gate | ورودی دیجیتال، خروجی Driver و تأخیر انتشار |
| 16 | D Flip-Flop | نمونه‌برداری لبهٔ بالارونده و نگه‌داری حالت |
| 17 | ADC | Vin/Vref آنالوگ، خروجی‌های دیجیتال و Conversion Delay |
| 18 | DAC | ورودی‌های دیجیتال/Vref، خروجی آنالوگ و Conversion Delay |
| 19 | Microcontroller | GPIO دوطرفه، زمان دستور، RAM/Flash و Decoder |
| 20 | External Memory | Address/Control ورودی و Data Bus دوطرفه |
| 21 | LCD 16x2 | Data/RS/RW/E ورودی و تشخیص لبهٔ Enable |
| 22 | Matrix Keypad | اتصال پویای Row/Column و Driver دوطرفه هنگام فشردن کلید |
| 23 | Voltmeter | مقاومت ورودی بالا و اختلاف ولتاژ دو Net |
| 24 | Ammeter | مقاومت سری بسیار کم و جریان شاخه |

## اصلاحات تکمیلی

- `Net::voltageResolved` از وجود مستقیم Output جدا شد.
- وضعیت و جریان قبلی LED/Meterها پس از قطع یا مدار حل‌ناپذیر دیگر در UI باقی نمی‌ماند.
- مقاومت داخلی Battery در حل مدار و جریان ترمینال آن لحاظ می‌شود.
- مقدارهای صفر/منفی R، C و L هنگام ورود از UI Clamp می‌شوند.
- پین‌های منابع دوترمیناله باید هر دو سیم‌کشی شوند.
- اتصال کوتاه دو سر یک Voltage Source به یک Net توسط DRC گزارش می‌شود.
- Ground جدا و بلااستفاده دیگر نمی‌تواند یک مدار بدون Reference را معتبر کند.
- پیام DRC در Status Bar اولین خطای دقیق را نشان می‌دهد و کل خطاها در Console می‌آیند.

## مدارهای Regression

- Battery → LED → Resistor → GND
- Clock → Resistor → Capacitor → GND
- DC Source → Inductor → Resistor → GND
- ADC با Vin/Vref روی Netهای حل‌شدهٔ آنالوگ
- DAC با کد دیجیتال `1010` و بار مقاومتی آنالوگ
- Battery → Ammeter → Resistor → GND همراه Voltmeter
- Push Button → Switch → Resistor → GND
- Run/Pause/Step/Stop و بازگردانی Snapshot
- ADC/DAC، MCU/LCD، MCU/Keypad و MCU/External Memory
- Save/Load تمام ۲۵ نوع کامپوننت

در نسخهٔ فعلی ۱۰۵ بررسی Backend و ۵۰ بررسی Integration بدون شکست اجرا می‌شوند.
این مجموعه علاوه بر مدارهای بالا، رفتار مستقل XOR، NAND، Seven Segment، D-FF،
MCU، External Memory، LCD و Matrix Keypad را نیز پوشش می‌دهد.
