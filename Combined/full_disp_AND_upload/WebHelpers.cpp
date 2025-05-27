#include "WebHelpers.h"
#include <ESP32WebServer.h>

String webpage = ""; //String to save the html cod
extern ESP32WebServer server;

//Upload a file to the SD
void File_Upload()
{
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' type='file' name='fupload' id = 'fupload' value=''>");
  webpage += F("<button class='buttons' type='submit'>Upload File</button><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html",webpage);
}

//Upload a set time to the SD
void Time_Upload()
{
  append_page_header();
  webpage += F("<h3>Enter time (YYYY-MM-DD HH:MM:SS)</h3>");
  webpage += F("<FORM action='/timeUploadProcess' method='post'>");
  webpage += F("<input class='buttons' type='text' name='timeUploadProcess' id = 'timeUploadProcess'><br><br>");
  webpage += F("<button class='buttons' type='submit'>Set Time</button>");
  webpage += F("</form>");
  webpage += F("<a href='/'>[Back]</a><br><br>");

  append_page_footer();
  server.send(200, "text/html",webpage);
}

void append_page_header() {
  webpage  = F("<!DOCTYPE html><html>");
  webpage += F("<head>");
  webpage += F("<title>Audrey's Server</title>"); // NOTE: 1em = 16px
  webpage += F("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
  webpage += F("<style>");  // ⬅️ start of <style>

  webpage += F("body{background-color:#fff0f5;font-family:'Segoe UI','Comic Sans MS',sans-serif;color:#5e3c58;padding:2em;margin:0 auto;max-width:800px;}");

  webpage += F("h1{color:#e36bae;text-align:center;margin-top:0.5em;font-size:1.8em;}");
  webpage += F("h2,h3{color:#8c5b75;text-align:center;font-weight:normal;margin:0.5em 0;}");

  webpage += F("ul{list-style:none;padding:0;margin-bottom:2em;overflow:hidden;background-color:#ffb6c1;border-radius:12px;box-shadow:0 2px 6px rgba(0,0,0,0.1);}");  
  webpage += F("li{float:left;}");  
  webpage += F("li a{display:block;color:white;text-align:center;padding:0.75em 1em;text-decoration:none;font-weight:bold;font-size:1em;}");  
  webpage += F("li a:hover{background-color:#ff8bb3;border-radius:12px;}");

  webpage += F("table{width:100%;border-collapse:collapse;margin-top:1em;background:#fff;border-radius:12px;overflow:hidden;box-shadow:0 2px 6px rgba(0,0,0,0.05);}");  
  webpage += F("th{background-color:#ffe4f0;color:#5e3c58;padding:0.8em;text-align:center;border-bottom:2px solid #f7c2df;font-size:1em;}");  
  webpage += F("td{padding:0.8em;text-align:center;border-bottom:1px solid #f3d3e8;font-size:0.95em;}");  
  webpage += F("tr:nth-child(even){background-color:#fff7fb;}");  
  webpage += F("tr:hover{background-color:#fff0f5;}");

  webpage += F("button,input[type='submit']{background-color:#ff8bb3;color:white;border:none;padding:0.5em 1.2em;border-radius:10px;font-size:0.95em;margin:0.2em;cursor:pointer;transition:background 0.2s;}");  
  webpage += F("button:hover,input[type='submit']:hover{background-color:#f86aa1;}");  

  webpage += F("input[type='file']{margin:1em 0;padding:0.6em;width:100%;border-radius:8px;border:1px solid #ddd;font-size:0.95em;color:#5e3c58;background:#fff8fb;}");

  webpage += F("form{display:inline;}");  

  webpage += F("a{display:inline-block;margin-top:1.5em;text-decoration:none;color:#e36bae;font-size:0.95em;}");  
  webpage += F("a:hover{text-decoration:underline;}");

  webpage += F(".status{text-align:center;margin-top:1em;font-style:italic;color:#8c5b75;}");

  webpage += F("::file-selector-button{background-color:#ffa4d4;border:none;padding:0.4em 0.8em;border-radius:6px;color:#5e3c58;cursor:pointer;font-size:0.9em;}");  
  webpage += F("::file-selector-button:hover{background-color:#ff8bb3;}");

  webpage += F("*,*:before,*:after{box-sizing:border-box;}");  // universal box-sizing

  webpage += F("</style>");
  webpage += F("</head><body><h1>Clock Gallery Directory</h1>");
  webpage += F("<ul>");
  webpage += F("<li><a href='/'>Current Files</a></li>"); //Menu bar with commands
  webpage += F("<li><a href='/upload'>Upload a .bin File</a></li>"); 
  webpage += F("<li><a href='/timeUpload'>Set Clock Time</a></li>"); 
  webpage += F("</ul>");
}
//Saves repeating many lines of code for HTML page footers
void append_page_footer()
{ 
  webpage += F("</body></html>");
}

//SendHTML_Header
void SendHTML_Header()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); //Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}

//SendHTML_Content
void SendHTML_Content()
{
  server.sendContent(webpage);
  webpage = "";
}

//SendHTML_Stop
void SendHTML_Stop()
{
  server.sendContent("");
  server.client().stop(); //Stop is needed because no content length was sent
}

//ReportSDNotPresent
void ReportSDNotPresent()
{
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//ReportFileNotPresent
void ReportFileNotPresent(String target)
{
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//ReportCouldNotCreateFile
void ReportCouldNotCreateFile(String target)
{
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
