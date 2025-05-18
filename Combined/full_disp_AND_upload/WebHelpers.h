#ifndef WEB_HELPERS_H
#define WEB_HELPERS_H

#include <Arduino.h>

extern String webpage;

void append_page_header();
void append_page_footer();
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void ReportSDNotPresent();
void ReportFileNotPresent(String target);
void ReportCouldNotCreateFile(String target);
void File_Upload();

#endif
