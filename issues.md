# QNX Planes Application - Issues & Improvements

## Completed

1. ✅ **Server coordinate parameters** - Servers can now start with `-id`, `-x`, and `-y` options
   - Example: `./server/out -id 101 -x 25 -y 30`
   - If coordinates are not specified or invalid, random ones are used

2. ✅ **Map scaling** - The visualization now dynamically scales to the widget size
   - Removed hardcoded VIEW_WIDTH and VIEW_HEIGHT
   - Uses actual Photon widget dimensions via PtWidgetDim()
   - Proper aspect ratio scaling

3. ✅ **Click to select plane** - Added plane selection via mouse click on the top view
   - Left-click on a plane to select it
   - Updates the selected plane in the list and coordinate fields
   - Hit detection with 15-pixel radius

4. ✅ **Example startup script** - Created `run_example.sh`
   - Starts 3 planes with different coordinates
   - Automatically launches dispatcher
   - Proper cleanup on exit

5. ⚠️ **Race condition fixes** - Improved file locking
   - Added flockfile/funlockfile to prevent registry file corruption
   - Better validation of registry entries
   - Debug logging for connection issues

## Known Issues

1. **Plane 0,0 initialization** - Planes briefly appear at (0,0) before first state update
   - This is expected behavior; first update from server sets actual position
   - Not causing crashes with the improved file locking

2. **Map flickering** - Ignored as requested

## Next Steps (Optional)

- Add persistence for window size/position
- Implement better collision visualization (lines between warning pairs)
- Add altitude range filtering
- Keyboard shortcuts for plane navigation