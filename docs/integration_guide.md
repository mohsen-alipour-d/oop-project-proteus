# راهنمای یکپارچه‌سازی رابط کاربری و موتور شبیه‌سازی با Backend

این سند برای هر کسی است که می‌خواهد رابط کاربری، کتابخانهٔ قطعات یا موتور
شبیه‌سازی را به Backend وصل کند. نسخهٔ یکپارچه با ۷۱ تست Backend و ۳۳ تست
Integration بررسی شده است.

پیش از هر چیز، آخرین نسخهٔ `main` را بگیرید و یک بار تست‌ها را اجرا کنید تا از سلامت محیط مطمئن شوید:

    git clone https://github.com/mohsen-alipour-d/oop-project-proteus.git
    cd oop-project-proteus
    cmake -S . -B build
    cmake --build build
    ./build/run_tests

## ۰) سه قانون طلایی

۱. **مالکیت حافظه با Circuit است.** هر قطعه را با `new` بسازید و به `addComponent` بدهید؛ هرگز خودتان `delete` نکنید و هرگز قطعهٔ روی پشته (stack) را به مدار ندهید. Circuit هنگام `clear` و در destructor، همهٔ قطعات، سیم‌ها، گره‌ها و شبکه‌ها را آزاد می‌کند.

۲. **بعد از هر تغییر کاربری، یک snapshot بگیرید:** `history.push(fm.serialize(circuit));` تا Undo/Redo کار کند. منظور از تغییر کاربری، قرار دادن، جابجایی، چرخش، سیم‌کشی، حذف و تغییر مقدار قطعه است؛ نه هر گام شبیه‌سازی.

۳. **پیش از Run، همیشه DRC:** `drc.validate(circuit)`؛ اگر `false` برگشت، پیام‌های `drc.log.messages` را نمایش دهید و شبیه‌سازی را شروع نکنید.

## ۱) مدل ذهنی: کلاس‌ها و مسئولیت‌شان

- `Circuit`: ظرف اصلی پروژه؛ فهرست قطعات، سیم‌ها، گره‌ها و شبکه‌ها را نگه می‌دارد و مالک همهٔ آن‌هاست.
- `Component`: کلاس پایهٔ همهٔ قطعات؛ موقعیت، چرخش، آینه و پین‌ها را نگه می‌دارد.
- `Pin`: یک پایهٔ قطعه؛ موقعیت محلی، ولتاژ، جریان، وضعیت اتصال (`connected`) و جهت (`isOutput`) را نگه می‌دارد.
- `Wire`: یک سیم؛ مسیر شکستهٔ ۹۰ درجه‌اش در `points` است و دو سرش به دو `Pin` وصل است.
- `Junction`: نقطهٔ اتصال الکتریکی دو یا چند سیم؛ فقط وقتی کاربر روی تقاطع کلیک کند ساخته می‌شود.
- `Net`: یک شبکهٔ الکتریکی؛ گروهی از پین‌ها که از طریق سیم و گره به هم رسیده‌اند و ولتاژ مشترک دارند.

نکتهٔ کلیدی: **دو سیمی که فقط از روی هم عبور کنند، متصل نیستند.** اتصال الکتریکی تنها با `Junction` برقرار می‌شود.

## ۲) رسم قطعات

- موقعیت قطعه: `comp->position`
- چرخش (مضربی از ۹۰ درجه): `comp->rotation`؛ آینه: `comp->mirroredH` و `comp->mirroredV`
- موقعیت جهانی هر پین برای رسم: `pin.worldPos()` (چرخش و آینه را خودش اعمال می‌کند)
- کادر دور قطعه برای انتخاب و تشخیص برخورد: `comp->getBoundingBox()`

پیشنهاد: بدنهٔ قطعه را در `position` بکشید و پین‌ها را در `worldPos` هر پین.

## ۳) موس و کلیک

- پین نزدیک موس (برای شروع یا پایان سیم): `circuit.findPinAt(mouse, 5)`
- هایلایت پین هنگام عبور موس: `pin.checkMouseOver(mouse)` که فیلد `isHighlighted` را ست می‌کند؛ پین هایلایت‌شده را پررنگ‌تر بکشید.
- سیم نزدیک موس (برای انتخاب یا حذف): `circuit.findWireAt(mouse, 3)`
- اولویت برخورد: اول `findPinAt`، بعد `findWireAt`؛ چون پین‌ها کوچک‌اند و باید بر سیم مقدم باشند.
- رسم سیم: یک polyline از روی `wire.points`؛ این نقاط همیشه زوایای ۹۰ درجه دارند.
- رسم گره: یک دایرهٔ توپر کوچک در `junction.position`.

## ۴) قرار دادن، چسباندن به شبکه و جابجایی

- قرار دادن قطعه: `circuit.addComponent(new Resistor("R1", x, y, 100));`
- نام‌ها باید **یکتا و بدون فاصله** باشند (مثل `R1`، `V1`، `U3`)؛ چون سریال‌سازی با فاصله توکن‌بندی می‌کند.
- چسباندن به شبکهٔ طراحی: `comp->snapToGrid(10);` — پیشنهاد می‌شود بعد از قرار دادن و بعد از جابجایی صدا زده شود تا سیم‌ها تمیز بمانند.
- جابجایی: `circuit.moveComponent(comp, dx, dy);` — سیم‌های متصل خودشان دوباره مسیر‌یابی می‌شوند؛ نیازی به `refreshWires` دستی نیست.
- انتخاب چندتایی با کادر: `circuit.getComponentsInRect(rect)`

## ۵) سیم‌کشی

- ساخت سیم بین دو پین: `circuit.addWire(pinA, pinB);`
- حذف یک سیم: `circuit.removeWire(w);`
- حذف کل شبکهٔ متصل (همهٔ سیم‌های هم‌Net): `circuit.removeNetOf(w);`
- ساخت گره روی تقاطع: `circuit.addJunctionAt(pos);` — اگر کمتر از دو سیم از آن نقطه بگذرد، `nullptr` برمی‌گرداند و چیزی ساخته نمی‌شود.
- بعد از هر کدام، snapshot برای Undo/Redo فراموش نشود.

## ۶) حلقه و کنترل شبیه‌سازی (بخش ۸)

UI مستقیماً زمان را دستکاری نمی‌کند و فقط این API را از `BackendAdapter` صدا می‌زند:

- `startSimulation()` برای Run یا ادامه از Pause
- `pauseSimulation()` برای فریز زمان و رویدادها
- `updateSimulation(dt)` فقط در حالت Running
- `stepSimulation()` برای یک گام ثابت ۱ میلی‌ثانیه و باقی‌ماندن در Pause
- `stopSimulation()` برای صفرکردن زمان، پاک‌کردن رویدادها و بازگشت به Snapshot ویرایش

Adapter در هر به‌روزرسانی به همین ترتیب عمل می‌کند:

1. برای هر قطعه: `comp->step(dt, simTime);`
2. `circuit.propagateVoltages()` ولتاژ خروجی را روی کل Net پخش می‌کند.
3. چند پاس settle با `dt=0` اجرا می‌شود تا زنجیره‌های ترکیبی بدون جلو رفتن مجدد ساعت فوراً در UI دیده شوند.
4. اسیلوسکوپ با زمان جدید نمونه می‌گیرد.

برای UI از `wireViews()` استفاده کنید؛ علاوه بر ولتاژ و `driven`، وضعیت یکی از
`LOW`، `HIGH`، `UNDEFINED` یا `FLOATING` است. رنگ‌های بخش ۸ به‌ترتیب آبی، قرمز،
زرد و خاکستری هستند. در Stop باید رنگ پیش‌فرض ویرایش نمایش داده شود.

قواعد مهم:

- پین‌های منبع و خروجی گیت‌ها `isOutput = true` دارند؛ **هرگز روی این پین‌ها ولتاژ ننویسید**؛ مقدارشان فقط از `step` خود قطعه می‌آید.
- ورودی گیت‌ها فقط `pin.voltage` را می‌خوانند؛ چیزی در آن‌ها ننویسید.
- اگر دو خروجی متفاوت به یک Net برسند، `propagateVoltages` ولتاژ آن Net را `UNDEFINED` می‌کند؛ این همان اتصال کوتاه منطقی است و DRC هم گزارشش می‌کند.
- قطعات تأخیردار فقط در پاس اول `dt` واقعی می‌گیرند؛ پاس‌های settle زمان را جلو نمی‌برند.
- تعامل زنده با `toggleSwitch`، `setPushButtonPressed` و
  `adjustInteractiveValue` در Run و Pause مجاز است و پس از هر تغییر مدار settle می‌شود.

## ۷) اندازه‌گیری

- پروب ولتاژ (بدون افزودن قطعه): `probe.read(circuit, mousePos)` و برای تشخیص شناور بودن: `probe.isFloating(circuit, mousePos)`.
- ولت‌متر و آمپرمتر خودشان قطعه‌اند؛ مثل بقیه قطعات اضافه و سیم‌کشی‌شان کنید و `reading` را روی نمایشگرشان بکشید. اگر `hasError` درست بود، به جای عدد، `ERR` نمایش دهید.
- اسیلوسکوپ: با `scope.attachChannel(0, netId)` هر کانال را به یک Net وصل کنید؛ در حالت Run هر tick صدا بزنید `scope.update(circuit, simTime)`؛ نمودار هر کانال از `scope.channels[i].history` می‌آید. `timeDiv` و `voltDiv` برای هر کانال جداست. در Pause چیزی ثبت نمی‌شود؛ در Stop تاریخچه پاک می‌شود.

## ۸) Save/Load و پروژه‌های اخیر

- اولین ذخیره: `fm.saveAs(circuit, path)` — مسیر و نام را از کاربر بگیرید.
- ذخیره‌های بعدی: `fm.save(circuit)` — روی همان مسیر قبلی.
- باز کردن: `fm.load(circuit, path)` — مدار فعلی کاملاً پاک و از نو ساخته می‌شود.
- فهرست پروژه‌های اخیر: `fm.loadRecentList()` و سپس `fm.recentProjects` (حداکثر ۵ مورد) برای منوی آغازین.

## ۹) Undo/Redo

- Undo: `if (history.canUndo()) fm.deserialize(circuit, history.undo());`
- Redo: `if (history.canRedo()) fm.deserialize(circuit, history.redo());`
- **هشدار مهم:** بعد از هر `deserialize` (یعنی Undo، Redo یا Load)، همهٔ اشاره‌گرهایی که به قطعه، سیم یا پین داشتید **باطل** می‌شوند؛ چون مدار از نو ساخته شده است. هر آنچه UI نگه داشته (انتخاب، سیمِ در حال رسم، پروب و...) باید دوباره از Circuit پرسیده شود.

## ۱۰) نمایش گزارش DRC

    DRC drc;
    bool ok = drc.validate(circuit);
    for (LogMessage& m : drc.log.messages) { ... }

- `m.isError == true` → خط قرمز (خطا)؛ وگرنه سبز (عادی).
- اگر `ok == false`، دکمهٔ Run را غیرفعال نگه دارید.

## ۱۱) دام‌های رایج (حتماً بخوانید)

1. قطعهٔ stack + `addComponent` = crash؛ همیشه `new`.
2. نگه‌داشتن اشاره‌گر بعد از Undo/Redo/Load = اشاره‌گر آویزان؛ همیشه دوباره بپرسید.
3. نام با فاصله یا تکراری = خرابی Save/Load.
4. نوشتن ولتاژ روی پین `isOutput` = خرابی شبیه‌سازی.
5. فراموش کردن `propagateVoltages` = ورودی گیت‌ها هرگز خروجی قبلی را نمی‌بینند.
6. فرض اتصال دو سیم متقاطع بدون Junction = خطای الکتریکی؛ بدون گره، متصل نیستند.

## ۱۲) مرور سریع پرکاربردترین متدها

| کار | متد |
|---|---|
| پین نزدیک موس | `circuit.findPinAt(mouse, 5)` |
| سیم نزدیک موس | `circuit.findWireAt(mouse, 3)` |
| هایلایت پین | `pin.checkMouseOver(mouse)` |
| کادر قطعه | `comp->getBoundingBox()` |
| چسباندن به شبکه | `comp->snapToGrid(10)` |
| جابجایی قطعه | `circuit.moveComponent(comp, dx, dy)` |
| انتخاب چندتایی | `circuit.getComponentsInRect(rect)` |
| ساخت سیم | `circuit.addWire(a, b)` |
| حذف سیم / شبکه | `circuit.removeWire(w)` / `circuit.removeNetOf(w)` |
| گره تقاطع | `circuit.addJunctionAt(pos)` |
| Run / Pause | `startSimulation()` / `pauseSimulation()` |
| گام شبیه‌سازی | `stepSimulation()`؛ گام ثابت ۱ ms و بازگشت به Pause |
| Stop | `stopSimulation()`؛ Reset زمان/رویدادها و Snapshot |
| رنگ سیم | `net->logicState()` |
| پروب | `probe.read(...)` / `probe.isFloating(...)` |
| اعتبارسنجی | `drc.validate(circuit)` |

اگر متدی خواستید که اینجا نیست، پیش از تغییر Backend با محسن هماهنگ کنید؛ به‌احتمال زیاد همین حالا راه ساده‌تری برایش وجود دارد.
