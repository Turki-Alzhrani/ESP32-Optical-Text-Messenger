#include <WiFi.h>
#include <WebServer.h>
#include <atomic>

// ESP32 DevKit (classic dual-core ESP32), Arduino-ESP32 core.
const int LED_PIN = 18;
const uint32_t BIT_MS = 40;
const char *AP_NAME = "Optical-TX";
const char *AP_PASSWORD = "Light2026";
WebServer server(80);

struct Message { char text[81]; uint32_t id; };
QueueHandle_t txQueue;
std::atomic<bool> busy(false);
std::atomic<uint32_t> activeId(0), completedId(0);
uint32_t nextId = 0;

const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="ar" dir="rtl"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>إرسال النص بالضوء</title><style>
:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#0a1220;color:#f0f5ff;font-family:system-ui,sans-serif;min-height:100vh;padding:30px 18px;display:grid;place-items:center}
main{width:min(100%,520px)}.badge{display:inline-block;background:#133b39;color:#91edda;border:1px solid #22635b;padding:7px 12px;border-radius:30px;font-size:13px}h1{font-size:31px;margin:20px 0 10px}p{color:#b0bfd3;line-height:1.9}.card{margin-top:25px;background:#131f31;border:1px solid #29394e;border-radius:24px;padding:23px}label{font-weight:650}textarea{width:100%;margin-top:12px;border:1px solid #40536f;border-radius:14px;background:#0b1423;color:white;padding:15px;font:18px system-ui;resize:vertical;min-height:130px}textarea:focus{outline:2px solid #5de2c0}.meta{display:flex;justify-content:space-between;gap:10px;color:#b0bfd3;font-size:13px;margin:12px 0 22px}button{width:100%;border:0;border-radius:13px;padding:15px;background:#7ce8ca;color:#09251d;font-size:17px;font-weight:750;cursor:pointer}button:disabled{opacity:.45;cursor:wait}#status{margin:18px 0 0;padding:13px;border-radius:12px;background:#0b1423;line-height:1.7}small{display:block;margin-top:18px;color:#a3b2c7;line-height:1.9}footer{font-size:12px;direction:ltr;text-align:center;color:#8093ad;margin:24px}progress{width:100%;height:10px;accent-color:#7ce8ca;margin-top:14px}[hidden]{display:none!important}</style></head>
<body><main><span class="badge">ESP32 · Optical Link</span><h1>رسالتك… عبر الضوء</h1><p>اكتب نصًا إنجليزيًا، وأرسله إلى شاشة المستقبل بومضات LED.</p>
<section class="card"><form id="form"><label for="text">النص المراد إرساله</label><textarea id="text" dir="ltr" maxlength="80" placeholder="Hello Turki!" required></textarea><div class="meta"><span id="count">0 / 80 حرفًا</span><span id="estimate">المدة: —</span></div><button id="send" type="submit">إرسال بالضوء</button></form><div id="status" role="status" aria-live="polite">جارٍ الاتصال بالمرسل…</div><progress id="progress" max="100" value="0" hidden></progress><small>إنجليزي وأرقام ورموز فقط، حتى 80 حرفًا. اكتمال الإرسال لا يؤكد الاستلام؛ راجع شاشة OLED.</small></section><footer>25 bit/s · GPIO18 · Local & offline</footer></main>
<script>
const text=document.getElementById('text'),send=document.getElementById('send'),statusBox=document.getElementById('status'),bar=document.getElementById('progress');
let submitting=false,polling=false,lastCompleted=0,awaitedId=0;
function duration(){return 1.2+(text.value.length+1)*.48;}
function update(){document.getElementById('count').textContent=text.value.length+' / 80 حرفًا';document.getElementById('estimate').textContent=text.value.length?'المدة: نحو '+duration().toFixed(1)+' ثانية':'المدة: —';}
text.addEventListener('input',update);
async function poll(){if(polling)return;polling=true;try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();send.disabled=s.busy||submitting;text.disabled=s.busy||submitting;if(s.busy){statusBox.textContent='جارٍ إرسال الرسالة بالضوء…';bar.hidden=false;bar.removeAttribute('value');}else{bar.hidden=true;if(s.completed>0){lastCompleted=s.completed;statusBox.textContent='اكتمل الإرسال. تأكد من ظهور النص على شاشة OLED.';}else if(!submitting){statusBox.textContent='جاهز للإرسال';}}if(awaitedId&&s.completed>=awaitedId)awaitedId=0;}catch(e){send.disabled=true;statusBox.textContent='تعذّر الاتصال. تأكد من الاتصال بشبكة Optical-TX.';}finally{polling=false;}}
document.getElementById('form').addEventListener('submit',async(e)=>{e.preventDefault();const value=text.value;if(!value.trim()){statusBox.textContent='اكتب رسالة أولًا.';return;}if(!/^[\x20-\x7E]{1,80}$/.test(value)){statusBox.textContent='استخدم الإنجليزية والأرقام والرموز في سطر واحد فقط.';return;}submitting=true;send.disabled=true;text.disabled=true;statusBox.textContent='جارٍ تسليم النص للمرسل…';try{const r=await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({text:value})});if(!r.ok){const detail=await r.text();throw Error(detail);}const data=await r.json();awaitedId=data.id;statusBox.textContent='بدأ الإرسال بالضوء…';}catch(err){statusBox.textContent='تعذّر تأكيد الطلب. انتظر حالة المرسل قبل المحاولة مجددًا.';}finally{submitting=false;}});
send.disabled=true;update();poll();setInterval(poll,700);
</script></body></html>)HTML";

void sendByte(uint8_t value) {
  digitalWrite(LED_PIN, LOW);
  delay(BIT_MS);
  for (int i = 0; i < 8; ++i) {
    digitalWrite(LED_PIN, (value >> i) & 1);
    delay(BIT_MS);
  }
  digitalWrite(LED_PIN, HIGH);
  delay(BIT_MS * 3);
}

// Separate task: HTTP requests never run inside optical bit timing.
void transmitTask(void *) {
  Message message;
  for (;;) {
    if (xQueueReceive(txQueue, &message, portMAX_DELAY) == pdTRUE) {
      digitalWrite(LED_PIN, HIGH);
      delay(1200); // Receiver requires 1 s of steady light to arm.
      for (size_t i = 0; message.text[i]; ++i) {
        sendByte((uint8_t)message.text[i]);
      }
      sendByte('\n');
      completedId.store(message.id);
      busy.store(false);
    }
  }
}

void handleSend() {
  String value = server.arg("text");
  if (value.length() == 0 || value.length() > 80) {
    server.send(400, "text/plain", "Use 1 to 80 ASCII characters.");
    return;
  }
  bool nonspace = false;
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t c = (uint8_t)value[i];
    if (c < 32 || c > 126) {
      server.send(400, "text/plain", "Use printable ASCII, one line only.");
      return;
    }
    if (c != ' ') nonspace = true;
  }
  if (!nonspace) { server.send(400, "text/plain", "Message is empty."); return; }
  bool expected = false;
  if (!busy.compare_exchange_strong(expected, true)) {
    server.send(409, "text/plain", "Transmitter is busy.");
    return;
  }
  Message message = {};
  value.toCharArray(message.text, sizeof(message.text));
  message.id = ++nextId;
  activeId.store(message.id);
  if (xQueueSend(txQueue, &message, 0) != pdTRUE) {
    busy.store(false);
    server.send(503, "text/plain", "Queue unavailable.");
    return;
  }
  server.send(202, "application/json", "{\"id\":" + String(message.id) + "}");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  txQueue = xQueueCreate(1, sizeof(Message));
  if (!txQueue || xTaskCreatePinnedToCore(transmitTask, "opticalTX", 4096,
       nullptr, 2, nullptr, 1) != pdPASS) {
    Serial.println("Cannot start transmitter");
    while (true) delay(1000);
  }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_NAME, AP_PASSWORD)) {
    Serial.println("Cannot start access point");
    while (true) delay(1000);
  }
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", PAGE);
  });
  server.on("/send", HTTP_POST, handleSend);
  server.on("/status", HTTP_GET, []() {
    bool running = busy.load();
    uint32_t done = completedId.load();
    String json = "{\"busy\":";
    json += running ? "true" : "false";
    json += ",\"id\":" + String(activeId.load());
    json += ",\"completed\":" + String(done) + "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });
  server.onNotFound([](){ server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.print("Open http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  delay(1);
}
