#pragma once
#include <Arduino.h>
static const char kSetupPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>AS608 Setup</title>
<style>
body{font:16px system-ui,sans-serif;max-width:420px;margin:40px auto;padding:0 18px;line-height:1.5}
input,button{box-sizing:border-box;width:100%;padding:10px;margin:5px 0 14px;font:inherit}
button{cursor:pointer}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#eee;padding:12px}
</style>
<h2>AS608 Device Setup</h2>
<p id="status">Loading...</p>
<form id="wifi">
<label>Wi-Fi SSID<input id="ssid" required maxlength="32"></label>
<label>Wi-Fi password<input id="password" type="password" required minlength="8" maxlength="63"></label>
<button>Save &amp; Connect</button>
</form>
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
