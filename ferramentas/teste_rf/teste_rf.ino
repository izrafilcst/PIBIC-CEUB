// DIAGNOSTICO de RF puro, sem ESP-NOW: uma placa vira AP, a outra escaneia.
// Mede se as duas se enxergam e com que RSSI.
#include <Arduino.h>
#include <WiFi.h>

#define SSID_TESTE "DINO-TESTE-RF"

void setup() {
  Serial.begin(115200);
  delay(1500);

#ifdef DINO_AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_TESTE, "dino12345", 1);
  Serial.println("[rf] papel=AP ssid=" SSID_TESTE " canal=1");
  Serial.print("[rf] mac_ap="); Serial.println(WiFi.softAPmacAddress());
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("[rf] papel=SCANNER");
#endif
}

void loop() {
#ifdef DINO_AP
  Serial.printf("[rf] AP no ar, clientes=%d\n", WiFi.softAPgetStationNum());
  delay(3000);
#else
  const int n = WiFi.scanNetworks();
  Serial.printf("[rf] varredura: %d redes\n", n);
  bool achou = false;
  for (int i = 0; i < n; i++) {
    const bool alvo = (WiFi.SSID(i) == SSID_TESTE);
    if (alvo) achou = true;
    Serial.printf("[rf]   %-24s rssi=%4d canal=%2d%s\n",
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
                  alvo ? "   <== ALVO" : "");
  }
  Serial.println(achou ? "[rf] ALVO ENCONTRADO" : "[rf] ALVO AUSENTE");
  WiFi.scanDelete();
  delay(2000);
#endif
}
