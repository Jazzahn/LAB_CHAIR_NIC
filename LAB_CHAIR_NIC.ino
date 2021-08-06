#include <SPI.h>

#include <EthernetUdp2.h>
#include <EthernetClient.h>
#include <Dhcp.h>
#include <EthernetServer.h>
#include <Ethernet2.h>
#include <util.h>
#include <Dns.h>
#include <Twitter.h>

#define VIAL_RECEIVE 2
#define HEAD_RECEIVE 3
#define ROCK_RECEIVE 4
#define RESET_PUZZLE 5

bool useDHCP = false;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x2C, 0xF7, 0xF1, 0x08, 0x18, 0x01
};

IPAddress ip(10, 1, 20, 90);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

int headState = 0;
int vialState = 0;
int rockState = 0;

void setup() {
  Serial.begin(9600);

  SPI.begin();

  // start the Ethernet connection and the server:
  Serial.println("Not using DHCP");
  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
  
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  pinMode(ROCK_RECEIVE, INPUT);
  pinMode(VIAL_RECEIVE, INPUT);
  pinMode(HEAD_RECEIVE, INPUT);
  pinMode(RESET_PUZZLE, OUTPUT);
}

void loop() {
  // listen for incoming Ethernet connections:
  listenForEthernetClients();

  // Maintain DHCP lease
  if (useDHCP) {
    Ethernet.maintain();
  }

  if (digitalRead(ROCK_RECEIVE) == HIGH) {
    rockState = 1;
  } else if (digitalRead(ROCK_RECEIVE) == LOW) {
    rockState = 0;
  }

  if (digitalRead(HEAD_RECEIVE) == HIGH) {
    headState = 1;
  } else if (digitalRead(HEAD_RECEIVE) == LOW) {
    headState = 0;
  }

  if (digitalRead(VIAL_RECEIVE) == HIGH) {
    vialState = 1;
  } else if (digitalRead(VIAL_RECEIVE) == LOW) {
    vialState = 0;
  }
}

String statusString(int rock, int head, int vial) {
  String rockString = "N/A";
  String headString = "N/A";
  String vialString = "N/A";
  if (rock == 1) {
    rockString = "ROCK GOOD!";
  }
  if (head == 1) {
    headString = "CANISTER GOOD!";
  }
  if (vial == 1) {
    vialString = "VIAL GOOD!";
  }
  String returnString = rockString + " :: " + headString + " :: " + vialString;
  return returnString;
}

void reset() {
  digitalWrite(RESET_PUZZLE, HIGH);
  rockState = 0;
  headState = 0;
  vialState = 0;
  delay(10000);
  digitalWrite(RESET_PUZZLE, LOW);
}

// Actual request handler
void processRequest(EthernetClient& client, String requestStr) {
  Serial.println(requestStr);

  // Send back different response based on request string
  if (requestStr.startsWith("GET /status")) {
    Serial.println("polled for status!");
    writeClientResponse(client, statusString(rockState, headState, vialState));
  } else if (requestStr.startsWith("GET /reset")) {
    Serial.println("Room reset");
    reset();
    writeClientResponse(client, "ok");
  } else {
    writeClientResponseNotFound(client);
  }
}



/*
 * HTTP helper functions
 */

void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // Grab the first HTTP header (GET /status HTTP/1.1)
    String requestStr;
    boolean firstLine = true;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          processRequest(client, requestStr);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          firstLine = false;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;

          if (firstLine) {
            requestStr.concat(c);
          }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}


void writeClientResponse(EthernetClient& client, String bodyStr) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Access-Control-Allow-Origin: *");  // ERM will not be able to connect without this header!
  client.println();
  client.print(bodyStr);
}


void writeClientResponseNotFound(EthernetClient& client) {
  // send a standard http response header
  client.println("HTTP/1.1 404 Not Found");
  client.println("Access-Control-Allow-Origin: *");  // ERM will not be able to connect without this header!
  client.println();
}

