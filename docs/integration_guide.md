# راهنمای یکپارچه‌سازی UI و شبیه‌سازی با Backend

این سند برای ملیکا و هانا نوشته شده تا UI و موتور شبیه‌سازی با کمترین
خطا به Backend وصل شوند. همه متدهای زیر همین حالا پیاده‌سازی و تست شده‌اند.

## ۰) سه قانون کلی

۱. Circuit مالک قطعات است: قطعه را همیشه با `new` بسازید و به
`addComponent` بدهید؛ خودتان `delete` نکنید.
۲. بعد از هر تغییر مدار، `history.push(fm.serialize(circuit))` را صدا بزنید
تا Undo/Redo کار کند.
۳. قبل از Run حتماً `drc.validate(circuit)` را چک کنید؛ اگر false بود،
`drc.log.messages` را نمایش دهید و شبیه‌سازی را شروع نکنید.

## ۱) رسم قطعات

- موقعیت: `comp->position`
- چرخش و آینه: `rotation`, `mirroredH`, `mirroredV`
- موقعیت جهانی پین برای رسم: `pin.worldPos()`
- کادر انتخاب: `comp->getBoundingBox()`

## ۲) موس و کلیک

- پین نزدیک موس (شروع/پایان سیم): `circuit.findPinAt(mouse, 5)`
  و برای هایلایت: `pin.checkMouseOver(mouse)` که `isHighlighted` را ست می‌کند.
- سیم نزدیک موس (انتخاب/حذف): `circuit.findWireAt(mouse, 3)`
- رسم سیم: polyline از روی `wire.points` (همیشه زوایای ۹۰ درجه).
- نقطه اتصال: دایره توپر در `junction.position`.

## ۳) قرار دادن و جابجایی

- قرار دادن: `circuit.addComponent(new Resistor("R1", x, y, 100));`
- چسباندن به شبکه: `comp->snapToGrid(10);`
- جابجایی: `circuit.moveComponent(comp, dx, dy);`
  (سیم‌های متصل خودش دوباره مسیر‌یابی می‌کند.)

## ۴) سیم‌کشی

- ساخت سیم بین دو پین: `circuit.addWire(pinA, pinB);`
- حذف یک سیم: `circuit.removeWire(w);`
- حذف کل شبکه متصل: `circuit.removeNetOf(w);`
- ساخت Junction روی تقاطع دو سیم: `circuit.addJunctionAt(pos);`

## ۵) حلقه شبیه‌سازی (هانا)

در هر گام به این ترتیب:

1. برای هر قطعه: `comp->step(dt, simTime);`
2. سپس: `circuit.propagateVoltages();`
   (ولتاژ خروجی‌ها را روی کل Net و همه پین‌هایش پخش می‌کند)
3. رنگ سیم از روی: `net->logicState()` (LOW / HIGH / UNDEFINED)

- پین‌های منبع و خروجی گیت‌ها `isOutput = true` دارند؛ روی آن‌ها ولتاژ ننویسید.
- ورودی گیت‌ها فقط `pin.voltage` را می‌خوانند.

## ۶) اندازه‌گیری

- پروب ولتاژ: `probe.read(circuit, mousePos)` و `probe.isFloating(...)`
- ولت‌متر/آمپرمتر خودشان قطعه هستند؛ اضافه کنید و `reading` را بخوانید.
- اسیلوسکوپ: `scope.attachChannel(0, netId)` و در حالت Run
  `scope.update(circuit, simTime)`؛ نمودار از `scope.channels[i].history`.

## ۷) Save/Load

- بار اول: `fm.saveAs(circuit, path)`؛ دفعات بعد: `fm.save(circuit)`
- باز کردن: `fm.load(circuit, path)` (مدار فعلی پاک و از نو ساخته می‌شود)
- پروژه‌های اخیر: `fm.loadRecentList()` سپس لیست `fm.recentProjects`

## ۸) Undo/Redo

- Undo: `if (history.canUndo()) fm.deserialize(circuit, history.undo());`
- Redo: `if (history.canRedo()) fm.deserialize(circuit, history.redo());`

## ۹) نمایش گزارش DRC

    DRC drc;
    bool ok = drc.validate(circuit);
    for (LogMessage& m : drc.log.messages) { ... }

- `m.isError == true` → خط قرمز؛ وگرنه سبز.