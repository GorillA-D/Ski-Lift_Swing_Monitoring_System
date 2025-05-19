void updateEmergencyStatus(String name, float r, float p, float y) {
  boolean isEmergency = abs(r) > EMERGENCY_THRESHOLD || abs(p) > EMERGENCY_THRESHOLD || abs(y) > EMERGENCY_THRESHOLD;

  if (isEmergency) {
    chairEmergency.put(name, true);
    chairLastEmergencyTime.put(name, millis());

    if (millis() - lastBeepTime > beepCooldown) {
      beep.play();
      lastBeepTime = millis();
    }
  } else {
    if (chairEmergency.getOrDefault(name, false) &&
        millis() - chairLastEmergencyTime.getOrDefault(name, 0) > 1000) {
      chairEmergency.put(name, false);
    }
  }
}
/*
void drawEmergencyIndicator(String name, int i) {
  if (chairEmergency.getOrDefault(name, false)) {
    fill(255, 0, 0);
  } else {
    fill(0, 255, 0);
  }
  ellipse(-width / 2 + 10, height / 2 - 35 - i * 30, 15, 15);
}
*/
void drawEmergencyIndicator(String name, int i) {
  float spacing = 400;
  int totalChairs = chairAngles.size();
  float startX = -((totalChairs - 1) * spacing) / 2.0;
  float xPos = startX + i * spacing;  // X position of the chair

  if (chairEmergency.getOrDefault(name, false)) {
    fill(255, 0, 0);  // Red for emergency
  } else {
    fill(0, 255, 0);  // Green if no emergency
  }
  // Attach the indicator near the chair
  ellipse(xPos, 100, 30, 30);  // Adjust the Y position (100) for better placement
}
/*
void drawEmergencyMessage() {
  if (chairEmergency.containsValue(true)) {
    fill(255, 0, 0);
    text("EMERGENCY:", -width / 2 + 20, -height / 2 + 40);
    for (String name : chairEmergency.keySet()) {
      if (chairEmergency.get(name)) {
        text(name + " triggered emergency!", -width / 2 + 20, -height / 2 + 70);
        break;
      }
    }
  }
}
*/
void drawEmergencyMessage() {
  if (chairEmergency.containsValue(true)) {
    fill(255, 0, 0);
    textAlign(LEFT, CENTER);
    textSize(28);
    text("EMERGENCY:", -width / 2 + 20, -height / 2 + 40);

    int offsetY = 70;  // Starting point for displaying chair names with emergencies
    for (String name : chairEmergency.keySet()) {
      if (chairEmergency.get(name)) {
        text(name + " triggered emergency!", -width / 2 + 20, -height / 2 + offsetY);
        offsetY += 30;  // Add some space between messages
      }
    }
  }
}
