#include "jura_web.h"
#include "jura_page.h"
#include "esphome/components/logger/logger.h"
#include <cstdio>
#include <cctype>
#include <cstring>

#ifdef USE_ESP8266
#include <Arduino.h>
#endif

namespace esphome::jura_web {
static const char *const TAG = "jura.web";

void JuraWeb::setup() {
  this->jura_->register_jura_web(this);
  auto *base = web_server_base::global_web_server_base;
  if (base == nullptr) {
    ESP_LOGE(TAG,"WebServerBase unavailable; check web_server setup");
    this->mark_failed();
    return;
  }
  // No stock ESPHome web_server is linked in the compact profile. Register
  // before init(), so WebServerBase installs the digest-authenticated handler.
  base->add_handler(this);
  base->init();
#ifdef USE_LOGGER
  if (logger::global_logger != nullptr)
    logger::global_logger->add_log_callback(this,[](void *instance,uint8_t level,const char *tag,
                                                     const char *message,size_t len){
      static_cast<JuraWeb *>(instance)->on_log(level,tag,message,len);
    });
#endif
  ESP_LOGI(TAG,"Jura console available at /jura/");
}

// HTTP callbacks on ESP8266 run in the networking (sys) context. Defer any
// machine work to ESPHome's regular loop instead of calling the Jura UART driver
// directly from handleRequest(). The browser receives 202 once locally queued.
void JuraWeb::loop() {
  const uint8_t operation = this->pending_action_;
  if (operation == 0) return;
  this->pending_action_ = 0;
  if (operation == ACTION_EEPROM || operation == ACTION_RAM) {
    const bool eeprom = operation == ACTION_EEPROM;
    const bool started = this->jura_->start_discovery(
        eeprom ? jura::JuraButtonType::EEPROM_DUMP : jura::JuraButtonType::RAM_DUMP);
    if (!started) {
      this->on_capture_finished(eeprom, false);
      ESP_LOGW(TAG, "%s read rejected by Jura UART scheduler", eeprom ? "EEPROM" : "RAM");
    }
  } else if (operation == ACTION_COMMAND) {
    if (!this->jura_->queue_raw_command(std::string(this->pending_command_)))
      ESP_LOGW(TAG, "Web Jura command rejected by UART scheduler");
  }
}

void JuraWeb::dump_config() { ESP_LOGCONFIG(TAG,"Jura ESP8266 flash-based web console ready"); }

bool JuraWeb::canHandle(AsyncWebServerRequest *req) const {
  const auto &url = req->url();
  return url == "/" || url == "/jura" || url == "/jura/" || url.startsWith("/jura/api/");
}

void JuraWeb::send_error_(AsyncWebServerRequest *req,int code,const char *error) {
  req->send(code,"text/plain",error);
}

// In addition to digest authentication, ensure browser mutations originate from
// this host. The mandatory custom header makes cross-site HTML forms ineligible.
bool JuraWeb::origin_safe_(AsyncWebServerRequest *req) {
  if (!req->hasHeader("X-Jura-Control")) return false;
  auto *origin=req->getHeader("Origin");
  if (origin == nullptr || origin->value().length() == 0) return true;
  auto *host=req->getHeader("Host");
  if (host == nullptr) return false;
  const String o=origin->value();
  return o == String("http://")+host->value() || o == String("https://")+host->value();
}

uint8_t JuraWeb::count_pages_(uint16_t mask) {
  uint8_t n=0;for(;mask;mask&=mask-1)n++;return n;
}

void JuraWeb::handleRequest(AsyncWebServerRequest *req) {
  const auto &path=req->url();
  if(path=="/") {
    if(req->method()!=HTTP_GET) return send_error_(req,405,"GET required");
    req->redirect("/jura/");
    return;
  }
  if(path=="/jura" || path=="/jura/") {
    if(req->method()!=HTTP_GET) return send_error_(req,405,"GET required");
    auto *res=req->beginResponse_P(200,"text/html",JURA_HTML_GZ,JURA_HTML_GZ_LEN);
    res->addHeader("Content-Encoding","gzip");
    res->addHeader("Cache-Control","no-store");
    return req->send(res);
  }
  if(path=="/jura/api/status" && req->method()==HTTP_GET) return this->send_status_(req);
  if(path=="/jura/api/snapshot" && req->method()==HTTP_GET) return this->send_snapshot_(req);
  if(path=="/jura/api/logs" && req->method()==HTTP_GET) return this->send_logs_(req);
  if ((path=="/jura/api/read" || path=="/jura/api/command") && req->method()==HTTP_POST) {
    if(!origin_safe_(req)) return send_error_(req,403,"Same-origin X-Jura-Control header required");
    return this->send_action_(req);
  }
  send_error_(req,404,"Unknown Jura console route");
}

void JuraWeb::send_status_(AsyncWebServerRequest *req) {
  char b[224];
  std::snprintf(b,sizeof(b),
    "{\"version\":\"22.20.14\",\"eeprom_pages\":%u,\"ram_pages\":%u,\"eeprom_done\":%s,\"ram_done\":%s,\"eeprom_ok\":%s,\"ram_ok\":%s,\"queued\":%u,\"free_heap\":%u}",
    count_pages_(this->eeprom_mask_),count_pages_(this->ram_mask_),
    this->eeprom_done_?"true":"false",this->ram_done_?"true":"false",
    this->eeprom_ok_?"true":"false",this->ram_ok_?"true":"false",
    static_cast<unsigned>(this->pending_action_),
#ifdef USE_ESP8266
    ESP.getFreeHeap()
#else
    0U
#endif
  );
  // snprintf always NUL-terminates this nonempty buffer, even on truncation.
  // ESP8266 String supports const char*, NOT a (char*, size_t) constructor.
  req->send(200,"application/json",String(b));
}

void JuraWeb::send_snapshot_(AsyncWebServerRequest *req) {
  const bool ee=req->hasParam("region") && req->getParam("region")->value()=="eeprom";
  const bool ram=req->hasParam("region") && req->getParam("region")->value()=="ram";
  if (!ee && !ram) return send_error_(req,400,"region must be eeprom or ram");
  // One 16-item page per request. A multi-kilobyte AsyncResponseStream built
  // in the ESP8266 networking callback can starve both heap and the HA API.
  if (!req->hasParam("page")) return send_error_(req,400,"page=0..15 is required");
  const String page_str=req->getParam("page")->value();
  if (page_str.length()==0 || page_str.length()>2) return send_error_(req,400,"Invalid page");
  for (unsigned i=0;i<page_str.length();i++) if (page_str[i]<'0'||page_str[i]>'9')
    return send_error_(req,400,"Invalid page");
  const int page=page_str.toInt();
  if (page<0 || page>=16) return send_error_(req,400,"Invalid page");
  if (!((ee?this->eeprom_mask_:this->ram_mask_)&(1U<<page)))
    return send_error_(req,404,"Page not captured");
  char b[96];
  int n=std::snprintf(b,sizeof(b),"%s %s:%02X => %s:",
      ee?"EEPROM":"RAM",ee?"RT":"RR",static_cast<unsigned>(page*16),ee?"rt":"rr");
  if(n<0 || static_cast<size_t>(n)>=sizeof(b))return send_error_(req,500,"Formatting error");
  size_t used=static_cast<size_t>(n);
  for(unsigned i=0;i<16;i++){
    n=ee?std::snprintf(b+used,sizeof(b)-used,"%04X",this->eeprom_[page*16+i]):
         std::snprintf(b+used,sizeof(b)-used,"%02X",this->ram_[page*16+i]);
    if(n<0 || static_cast<size_t>(n)>=sizeof(b)-used)return send_error_(req,500,"Formatting error");
    used+=static_cast<size_t>(n);
  }
  req->send(200,"text/plain",b);
}

void JuraWeb::send_logs_(AsyncWebServerRequest *req) {
  uint32_t since=0;
  if(req->hasParam("since"))since=static_cast<uint32_t>(req->getParam("since")->value().toInt());
  // Max 4 records and one fixed buffer rather than a response-stream String
  // repeatedly reallocating on the ESP8266 networking thread.
  char b[640]{};
  size_t used=0;
  uint8_t sent=0;
  const uint32_t first=this->next_seq_>LOG_CAPACITY ? this->next_seq_-LOG_CAPACITY+1 : 1;
  for(uint32_t seq=first;seq<=this->next_seq_ && sent<4;seq++) {
    const auto &entry=this->lines_[(seq-1)%LOG_CAPACITY];
    if(!entry.seq || entry.seq<=since || entry.seq!=seq)continue;
    const int n=std::snprintf(b+used,sizeof(b)-used,"%lu\t%lu\t%s\n",
        static_cast<unsigned long>(entry.seq),static_cast<unsigned long>(entry.stamp),entry.message);
    if(n<0 || static_cast<size_t>(n)>=sizeof(b)-used)break;
    used+=static_cast<size_t>(n);
    sent++;
  }
  req->send(200,"text/plain",b);
}

void JuraWeb::send_action_(AsyncWebServerRequest *req) {
  const auto &path=req->url();
  if(path=="/jura/api/read") {
    const bool from_query=req->hasParam("region");
    if(!from_query && !req->hasParam("region",true)) return send_error_(req,400,"Missing region");
    const String region=from_query?req->getParam("region")->value():req->getParam("region",true)->value();
    const bool ee=region=="eeprom";
    if(!ee && region!="ram")return send_error_(req,400,"Use eeprom or ram");
    if(this->pending_action_!=0 || (!this->eeprom_done_ && this->active_capture_==ACTION_EEPROM) ||
       (!this->ram_done_ && this->active_capture_==ACTION_RAM))
      return send_error_(req,409,"Read/command already queued or capture running");
    if(ee){this->eeprom_mask_=0;this->eeprom_done_=false;this->eeprom_ok_=false;}
    else{this->ram_mask_=0;this->ram_done_=false;this->ram_ok_=false;}
    this->active_capture_=ee?ACTION_EEPROM:ACTION_RAM;
    this->pending_action_=this->active_capture_;
    req->send(202,"text/plain","Capture scheduled");return;
  }
  const bool from_query=req->hasParam("command");
  if(!from_query && !req->hasParam("command",true))return send_error_(req,400,"Missing command");
  String value=from_query?req->getParam("command")->value():req->getParam("command",true)->value();
  value.trim();value.toUpperCase();
  // WE writes deliberately excluded; all interactive writes will eventually
  // be mediated by compare/read/write/verify against a write-enable switch.
  bool safe = false;
  if (value.length()==5 && value.startsWith("FA:")) safe = true;
  if (value.length()==5 && (value=="AN:01"||value=="AN:02")) safe=true;
  if (value.length()==5 && (value.startsWith("RE:")||value.startsWith("RR:")||value.startsWith("RT:")))safe=true;
  if(value=="IC:"||value=="TY:")safe=true;
  if(safe && value.length()==5 && value[0]!='A') {
    for(unsigned i=3;i<5;i++) if(!isxdigit(static_cast<unsigned char>(value[i])))safe=false;
  }
  if(safe && value.length()==5 && value.startsWith("FA:")) {
    for(unsigned i=3;i<5;i++)if(!isxdigit(static_cast<unsigned char>(value[i])))safe=false;
  }
  if(!safe) return send_error_(req,403,"Only FA:xx, AN:01/02, RE:xx, RR:xx, RT:xx, IC:, TY: allowed. WE: disabled");
  if(this->pending_action_!=0 || (this->active_capture_!=0 &&
     !((this->active_capture_==ACTION_EEPROM?this->eeprom_done_:this->ram_done_))))
    return send_error_(req,409,"Capture/command pending");
  std::snprintf(this->pending_command_,sizeof(this->pending_command_),"%s",value.c_str());
  this->pending_action_=ACTION_COMMAND;
  req->send(202,"text/plain","Command scheduled");
}

void JuraWeb::on_eeprom_page(uint8_t addr,const uint16_t *v) {
  if(addr>0xF0 || (addr&15))return;
  for(unsigned i=0;i<16;i++)this->eeprom_[addr+i]=v[i];
  this->eeprom_mask_|=static_cast<uint16_t>(1U<<(addr>>4));
}
void JuraWeb::on_ram_page(uint8_t addr,const uint8_t *v) {
  if(addr>0xF0 || (addr&15))return;
  for(unsigned i=0;i<16;i++)this->ram_[addr+i]=v[i];
  this->ram_mask_|=static_cast<uint16_t>(1U<<(addr>>4));
}
void JuraWeb::on_capture_finished(bool eeprom,bool success) {
  if(eeprom){this->eeprom_done_=true;this->eeprom_ok_=success;}
  else{this->ram_done_=true;this->ram_ok_=success;}
  this->active_capture_=0;
  ESP_LOGI(TAG,"%s capture finished (%s)",eeprom?"EEPROM":"RAM",success?"complete":"partial/failed");
}
void JuraWeb::on_protocol(bool tx,const char *line) {
  if(line==nullptr)return;
  char b[LOG_WIDTH+1];
  std::snprintf(b,sizeof(b),"%s %.110s",tx?"TX:":"RX:",line);
  this->on_log(0,"uart",b,std::strlen(b));
}
void JuraWeb::on_log(uint8_t level,const char *tag,const char *msg,size_t len) {
  (void)level;
  if(!msg)return;
  // Avoid logging our own telemetry and recursive log generation.
  if(tag && std::strcmp(tag,"jura.web")==0)return;
  auto &e=this->lines_[this->next_seq_%LOG_CAPACITY];
  e.seq=++this->next_seq_;e.stamp=millis();
  size_t out=0;
  if(tag){for(size_t i=0;tag[i] && out<LOG_WIDTH-3 && i<24;i++)e.message[out++]=tag[i];e.message[out++]=' ';}
  for(size_t i=0;i<len && out<LOG_WIDTH;i++) {
    const char ch=msg[i];if(ch=='\n'||ch=='\r')break;
    e.message[out++]=static_cast<unsigned char>(ch)>=32?ch:' ';
  }
  e.message[out]='\0';
}
} // namespace esphome::jura_web
