#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
constexpr char kApSsid[] = "AS608-Setup";
constexpr char kApPassword[] = "AS608setup!26";
constexpr uint32_t kConnectTimeoutMs = 15000;
constexpr uint32_t kRebootDelayMs = 1000;
WebServer server(80);
DNSServer dns;
Preferences preferences;
String ssid;
String password;
bool apActive = false;
bool connecting = false;
uint32_t connectStarted = 0;
bool rebootPending = false;
uint32_t rebootRequestedAt = 0;

const char kPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>AS608 Setup</title>
<style>
body{font:16px system-ui,sans-serif;max-width:420px;margin:40px auto;padding:0 18px;line-height:1.5}
input,button{box-sizing:border-box;width:100%;padding:10px;margin:5px 0 14px;font:inherit}
button{cursor:pointer}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#eee;padding:12px}
</style>
<h2>AS608 Device Setup</h2>
<p id="status">Loading...</p>
<form id="wifi"><label>Wi-Fi SSID<input id="ssid" required maxlength="32"></label>
<label>Wi-Fi password<input id="password" type="password" minlength="8" maxlength="63" placeholder="Leave empty to keep current password"></label>
<button>Save &amp; Connect</button></form>
<button id="refresh" type="button">Refresh status</button>
<button id="reboot" type="button">Reboot device</button>
<button id="clear" type="button">Forget Wi-Fi</button>
<pre id="message"></pre>
<script>
const $=id=>document.getElementById(id);
async function api(path,method='GET',body){
 const r=await fetch(path,{method,headers:{'Content-Type':'application/json'},body:body?JSON.stringify(body):undefined});
 const data=await r.json();if(!r.ok)throw Error(data.error||'Request failed');return data;
}
async function refresh(){try{
 const d=await api('/api/v1/wifi');$('ssid').value=d.ssid||'';
 $('status').textContent='Mode: '+d.mode+' | Connected: '+d.connected+' | IP: '+d.ip;
}catch(e){$('message').textContent=e.message}}
$('wifi').onsubmit=async e=>{e.preventDefault();try{
 $('message').textContent=JSON.stringify(await api('/api/v1/wifi','PUT',{ssid:$('ssid').value,password:$('password').value}),null,2);
 $('password').value='';setTimeout(refresh,2000);
}catch(err){$('message').textContent=err.message}};
$('refresh').onclick=refresh;
$('reboot').onclick=async()=>{if(confirm('Reboot device?'))try{$('message').textContent=JSON.stringify(await api('/api/v1/device/reboot','POST'))}catch(e){$('message').textContent=e.message}};
$('clear').onclick=async()=>{if(confirm('Forget saved Wi-Fi credentials?'))try{$('message').textContent=JSON.stringify(await api('/api/v1/wifi','DELETE'))}catch(e){$('message').textContent=e.message}};
refresh();
</script></html>)HTML";

void startAp() {
  if (apActive) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kApSsid, kApPassword);
  dns.start(53, "*", WiFi.softAPIP());
  apActive = true;
  Serial.printf("Setup AP: %s at %s\n", kApSsid, WiFi.softAPIP().toString().c_str());
}
void connectWifi() {
  if (ssid.isEmpty()) { startAp(); return; }
  WiFi.mode(apActive ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  connectStarted = millis();
  connecting = true;
}
void sendJson(int code, const String& json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", json);
}
String jsonEscape(const String& s) {
  String out;
  for (size_t i=0;i<s.length();++i) {
    char c=s[i];
    if(c=='"' || c=='\\') out+='\\';
    if(c=='\n' || c=='\r') continue;
    out+=c;
  }
  return out;
}
String wifiStatus() {
  bool connected=WiFi.status()==WL_CONNECTED;
  String mode=apActive ? (connected ? "AP+STA" : "AP") : "STA";
  String ip=connected ? WiFi.localIP().toString() : (apActive ? WiFi.softAPIP().toString() : "");
  return "{\"ssid\":\""+jsonEscape(ssid)+"\",\"configured\":"+(ssid.isEmpty()?"false":"true")+
         ",\"connected\":"+(connected?"true":"false")+",\"mode\":\""+mode+"\",\"ip\":\""+ip+"\"}";
}
void setupRoutes() {
  server.on("/", HTTP_GET, [](){ server.send_P(200,"text/html; charset=utf-8",kPage); });
  server.on("/api/v1/health", HTTP_GET, [](){ sendJson(200,"{\"ok\":true}"); });
  server.on("/api/v1/device", HTTP_GET, [](){
    sendJson(200,"{\"uptime_ms\":"+String(millis())+",\"free_heap\":"+String(ESP.getFreeHeap())+"}");
  });
  server.on("/api/v1/wifi", HTTP_GET, [](){ sendJson(200,wifiStatus()); });
  server.on("/api/v1/wifi", HTTP_PUT, [](){
    String body=server.arg("plain");
    // Small JSON body parsed with ArduinoJson is preferable to ad-hoc parsing.
    // This minimal endpoint uses the bundled ArduinoJson dependency.
    extern void handleWifiUpdate(const String&);
    handleWifiUpdate(body);
  });
  server.on("/api/v1/wifi", HTTP_DELETE, [](){
    preferences.remove("ssid"); preferences.remove("password");
    ssid=""; password=""; connecting=false;
    WiFi.disconnect(); startAp();
    sendJson(200,"{\"ok\":true,\"message\":\"Wi-Fi credentials cleared\"}");
  });
  server.on("/api/v1/device/reboot", HTTP_POST, [](){
    sendJson(202,"{\"ok\":true,\"message\":\"Reboot scheduled\"}");
    rebootRequestedAt=millis(); rebootPending=true;
  });
  server.onNotFound([](){
    if(apActive && server.method()==HTTP_GET) {
      server.sendHeader("Location","http://192.168.4.1/",true);
      server.send(302,"text/plain","");
    } else sendJson(404,"{\"error\":\"Not found\"}");
  });
  server.begin();
}
}
#include <ArduinoJson.h>
void handleWifiUpdate(const String& body) {
  JsonDocument doc;
  if(deserializeJson(doc,body)) { sendJson(400,"{\"error\":\"Invalid JSON\"}"); return; }
  String newSsid=doc["ssid"] | "";
  String newPassword=doc["password"] | "";
  if(newSsid.isEmpty() || newSsid.length()>32) { sendJson(400,"{\"error\":\"Invalid SSID\"}"); return; }
  if(newPassword.isEmpty() && newSsid==ssid) newPassword=password;
  if(newPassword.length()<8 || newPassword.length()>63) { sendJson(400,"{\"error\":\"Password must be 8-63 characters\"}"); return; }
  ssid=newSsid; password=newPassword;
  preferences.putString("ssid",ssid); preferences.putString("password",password);
  startAp(); // Preserve recovery access during the transition.
  connectWifi();
  sendJson(202,"{\"ok\":true,\"message\":\"Connecting; refresh status shortly\"}");
}
void setup() {
  Serial.begin(115200);
  preferences.begin("device",false);
  ssid=preferences.getString("ssid","");
  password=preferences.getString("password","");
  if(ssid.isEmpty()) startAp(); else connectWifi();
  setupRoutes();
}
void loop() {
  server.handleClient();
  if(apActive) dns.processNextRequest();
  if(connecting) {
    if(WiFi.status()==WL_CONNECTED) {
      connecting=false;
      Serial.printf("Connected: %s\n",WiFi.localIP().toString().c_str());
      // Keep AP available until a future explicit handoff policy is added.
    } else if(millis()-connectStarted>=kConnectTimeoutMs) {
      connecting=false; startAp();
      Serial.println("STA timeout; recovery AP active");
    }
  }
  if(rebootPending && millis()-rebootRequestedAt>=kRebootDelayMs) ESP.restart();
}
