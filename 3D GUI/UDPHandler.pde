void initUDP() {
  try {
    udpSocket = new DatagramSocket(12345);
    udpSocket.setSoTimeout(0);
  } catch (Exception e) {
    println("UDP init failed: " + e);
    exit();
  }
}

void startUDPThread() {
  udpThread = new Thread(new Runnable() {
    public void run() {
      while (true) {
        try {
          DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
          udpSocket.receive(packet);
          String data = new String(packet.getData(), 0, packet.getLength());

          if (data.startsWith("ANGLE|")) {
            String[] parts = data.split("\\|");
            if (parts.length >= 5) {
              String name = parts[1].substring(5);
              float r = float(parts[2].substring(2));
              float p = float(parts[3].substring(2));
              float y = float(parts[4].substring(2));
              chairAngles.put(name, new float[] {r, p, y});
            }
          } else if (data.startsWith("EMERGENCY")) {
            String[] parts = data.split("\\|");
            for (String part : parts) {
              if (part.startsWith("NAME:")) {
                String name = part.substring(5);
                chairEmergency.put(name, true);
                chairLastEmergencyTime.put(name, millis());
              }
            }
          }

        } catch (Exception e) {
          // silent error
        }
      }
    }
  });
  udpThread.start();
}
