PShape chairModel;
PImage txtrUpper;
void loadChairModel() {
  txtrUpper = loadImage("thumbnail.png");
  chairModel = loadShape("Chairlift_1379.obj");  // Or "chair.stl"
  chairModel.setTexture(txtrUpper);
  chairModel.scale(5);  // Adjust scale if needed
  
  //chairModel.disableStyle();  // This allows fill() to work
}

void drawSkiLiftCarrier() {
  //shape(chairModel);
  
  pushMatrix();
  noStroke();
  fill(0, 100, 255);     // Set desired color here (blue example)
  shape(chairModel);
  popMatrix();
}


//void drawAngleText(String name, float r, float p, float y, int i) {
  /*
  fill(0);
  textAlign(LEFT);
  text(name + " R:" + nf(r, 1, 2) + " P:" + nf(p, 1, 2) + " Y:" + nf(y, 1, 2),
       -width / 2 + 20, height / 2 - 30 - i * 30);
  */
  /*
  float spacing = 400;
  int totalChairs = chairAngles.size();
  float startX = -((totalChairs - 1) * spacing) / 2.0;
  float xPos = startX + i * spacing;  // X position of the chair

  fill(0);
  textAlign(CENTER, CENTER);
  textSize(18);

  // Draw chair name and angle data beside the chair
  text(name + " R:" + nf(r, 1, 2) + " P:" + nf(p, 1, 2) + " Y:" + nf(y, 1, 2),
       xPos, 150);  // 150 is the height offset; adjust as needed
}
*/

void drawAngleText(String name, float r, float p, float y, int i) {
  float spacing = 400;
  int totalChairs = chairAngles.size();
  float startX = -((totalChairs - 1) * spacing) / 2.0;
  float xPos = startX + i * spacing;  // X position of the chair

  fill(0);
  textAlign(CENTER, CENTER);
  textSize(28);

  // Display the chair name and angles, positioned near each chair
  text(name + " R:" + nf(r, 1, 2) + " P:" + nf(p, 1, 2) + " Y:" + nf(y, 1, 2),
       xPos, 150);  // 150 is the height offset; adjust as needed
}


void drawAxisHelper(float len) {
  strokeWeight(3);

  // X-axis (Red)
  stroke(255, 0, 0);
  line(0, 0, 0, len, 0, 0);

  // Y-axis (Green)
  stroke(0, 255, 0);
  line(0, 0, 0, 0, len, 0);

  // Z-axis (Blue)
  stroke(0, 0, 255);
  line(0, 0, 0, 0, 0, len);

  strokeWeight(1);
}
void drawLiveIndicator() {
  int interval = 500;  // Flash every 500 ms
  boolean visible = (millis() / interval) % 2 == 0;

  if (visible) {
    fill(255, 0, 0);
    textSize(32);
    textAlign(RIGHT, TOP);
    text("● Live", width - 20, 20);
  }
}
