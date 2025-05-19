import processing.net.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.util.HashMap;
import processing.sound.*;
import java.net.*;

InetAddress espIP;
int espPort;

int buttonX = 20, buttonY = 20, buttonW = 100, buttonH = 40;


DatagramSocket udpSocket;
byte[] buffer = new byte[1024];

Thread udpThread;
SoundFile beep;
boolean beepPlayed = false;
int lastBeepTime = 0;
final int beepCooldown = 250;

final int emergencyTimeout = 10000;
final float EMERGENCY_THRESHOLD = 35.0;

HashMap<String, float[]> chairAngles = new HashMap<String, float[]>();
HashMap<String, Boolean> chairEmergency = new HashMap<String, Boolean>();
HashMap<String, Integer> chairLastEmergencyTime = new HashMap<String, Integer>();

void setup() {
  size(1920, 1080, P3D);
  /*
  try {
  espIP = InetAddress.getByName("172.20.10.13");  // Replace with your actual ESP32 IP
  espPort = 12345;  // The port your ESP32 is listening on
} catch (Exception e) {
  e.printStackTrace();
}
*/
  loadChairModel();  // After size() and before other logic
  textSize(22);
  beep = new SoundFile(this, "alert.wav");
  initUDP();
  startUDPThread();
}

void draw() {
  background(233);
  //drawReset();
  translate(width / 2, height / 2, 0);

  int i = 0;
  for (String name : chairAngles.keySet()) {
    float[] angles = chairAngles.get(name);
    float r = angles[0], p = angles[1], y = angles[2];

    updateEmergencyStatus(name, r, p, y);

    pushMatrix();
    //translate(-200 + i * 400, 200, 0);
    float spacing = 400;
    int totalChairs = chairAngles.size();
    float startX = -((totalChairs - 1) * spacing) / 2.0;
    translate(startX + i * spacing, 0, 0);
    // Flip vertically
    scale(1, -1, 1);  // This flips the model along the Y-axis
    rotateX(radians(-p));
    rotateZ(radians(r));
    rotateY(radians(-y));
    drawSkiLiftCarrier();
    drawAxisHelper(100);  // 100 is axis length
    popMatrix();

    drawAngleText(name, r, p, y, i);
    drawEmergencyIndicator(name, i);
    drawLiveIndicator();

    i++;
  }

  drawEmergencyMessage();
}
