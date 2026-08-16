# Manual Test Checklist

Run these tests after the project builds in CLion.

## Section 1 and 2 checks

1. Create A4 and A3 projects and confirm the selected dimensions appear in
   the status bar.
2. Create a custom canvas. Values below 100 mm, above 2000 mm, or values such
   as `500abc` must be rejected.
3. Resize the window and confirm buttons and components still respond at the
   visible mouse position.
4. Zoom with the wheel. The world point under the cursor must remain under the
   cursor.
5. Pan with the middle mouse button and with `Space` + left drag.
6. Confirm the status bar updates X/Y and zoom in real time.

## Section 3 checks

1. Select each category and confirm unrelated components disappear.
2. Search by component name (`resistor`) and category (`digital`).
3. Search for an unknown word and confirm `NO COMPONENT FOUND` appears.
4. Select a result and confirm its schematic preview and category appear.
5. Add a component to the active list. Adding it again must not create a
   duplicate.
6. Remove an active component with its red `X` button.
7. Double-click a search result and confirm it is added and armed for placement.

## Section 4 checks

1. Place several components and confirm their labels increment (`R1`, `R2`).
2. Click a component, then drag it. Its center must snap to the 20-pixel grid.
3. Drag a selection rectangle across several components. Components that
   intersect the rectangle must be highlighted.
4. Extend or toggle the selection using `Shift` or `Ctrl`.
5. Rotate a component four times. The fourth rotation must return it and its
   pins to the original position.
6. Mirror horizontally and vertically and confirm the pin coordinates in the
   properties panel change consistently.
7. Double-click a resistor, battery, LED, and switch. Confirm the second field
   is named `RESISTANCE`, `VOLTAGE`, `COLOR`, and `STATE`, respectively.
8. Edit the label/value, save, and confirm both the canvas and properties panel
   update.
9. Delete components with the keyboard and with the right-click menu.
10. Press `Esc` while placing a component and confirm placement is cancelled.

