#pragma once

// Experimental native Empire Report. Uses existing engine GraphicWin support;
// it does not change turn processing or save formats.
bool empire_report_visible();
void empire_report_show();
void empire_report_hide();
void empire_report_scroll(int rows);
void empire_report_scroll_page(int pages);
void empire_report_scroll_wheel(int delta);
