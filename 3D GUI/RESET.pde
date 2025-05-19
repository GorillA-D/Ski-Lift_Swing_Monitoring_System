/*
void drawReset(){
  // Draw the RESET button
  fill(200, 50, 50);
  rect(buttonX, buttonY, buttonW, buttonH, 10);
  fill(255);
  textAlign(CENTER, CENTER);
  text("RESET", buttonX + buttonW / 2, buttonY + buttonH / 2);

}
void mousePressed() {
  if (mouseX > buttonX && mouseX < buttonX + buttonW &&
      mouseY > buttonY && mouseY < buttonY + buttonH) {
    sendResetCommand();
  }
}
void sendResetCommand() {
  try {
    byte[] buf = "RESET".getBytes();
    DatagramPacket packet = new DatagramPacket(buf, buf.length, espIP, espPort);
    udpSocket.send(packet);
    println("Sent RESET command to ESP32");
  } catch (Exception e) {
    e.printStackTrace();
  }
}
*/
