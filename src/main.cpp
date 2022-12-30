#include <Arduino.h>
#include <SoftwareSerial.h>

#define SERIAL_BAUD 9600
#define AT_BAUD 38400
#define TIMEOUT_MILLIS 5000

#define _assert(msg,cmd,si,ert)     Serial.print(msg);\
                                    if (at_assert(cmd, si, "OK", ert)) Serial.println("SUCCESS"); \
                                    else Serial.println("FAILED");

SoftwareSerial slave(2, 3);
SoftwareSerial master(4, 5);


String at_cmd(String command, SoftwareSerial& serial_interface)
{
    unsigned int time_limit = millis() + TIMEOUT_MILLIS;

    serial_interface.print(command + "\r\n");

    while (!serial_interface.available())
    {
        if (millis() > time_limit) return "ERROR: Timed out\r\n";
    }

    return serial_interface.readString();
}

bool at_connect(SoftwareSerial& serial_interface)
{
    serial_interface.begin(AT_BAUD);
    bool out = at_cmd("AT", serial_interface).equals("OK\r\n");
    serial_interface.flush();
    serial_interface.end();
    return out;
}

bool at_assert(String command, SoftwareSerial& serial_interface, String reference, bool& error_tracker)
{
    if (!(at_cmd(command, serial_interface).equals(reference + "\r\n")))
    {
        error_tracker = true;
        return false;
    }

    return true;
}

void setup()
{
    String slave_addr;
    String slave_addr_param;

    bool connection_error = false;
    bool setup_error = false;

    Serial.begin(SERIAL_BAUD);

    Serial.println();
    Serial.println("This program will sync 2 HC-05 bluetooth modules.");
    Serial.println("Please connect them as shown below and put them into AT mode:");
    Serial.println();
    Serial.println("Expected pin Configuration:");
    Serial.println("MASTER:  RX: 2  TX: 3");
    Serial.println("SLAVE:   RX: 4  TX: 5");
    Serial.println();

    Serial.println("Checking if slave and master are connected:");

    Serial.print("Slave...  ");
    if (at_connect(slave)) Serial.println("CONNECTED");
    else
    {
        Serial.println("DISCONNECTED");
        connection_error = true;
    }
    Serial.print("Master... ");
    if (at_connect(master)) Serial.println("CONNECTED");
    else
    {
        Serial.println("DISCONNECTED");
        connection_error = true;
    }

    if (connection_error)
    {
        Serial.println("[ERROR]: Bluetooth modules are not correctly connected");
        delay(TIMEOUT_MILLIS);
        exit(1);
    }

    Serial.println();
    Serial.println("Configuring slave module:");
    slave.begin(AT_BAUD);

    _assert("Resetting module...                ", "AT+ORGL", slave, setup_error);
    _assert("Setting role to slave...           ", "AT+ROLE=0", slave, setup_error);
    _assert("Setting pin to 1205...             ", "AT+PSWD=\"1205\"", slave, setup_error);
    _assert("Setting connection mode to bind... ", "AT+CMODE=0", slave, setup_error);

    slave_addr = at_cmd("AT+ADDR?", slave).substring(6, 20);
    slave_addr_param = '"' + slave_addr.substring(0, 4) + ","
                     + slave_addr.substring(5, 7) + ","
                     + slave_addr.substring(8, 14) + '"';

    slave.end();
    //delay(1000);

    Serial.println();
    Serial.println("Configuring master module");
    master.begin(AT_BAUD);

    _assert("Resetting module...                ", "AT+ORGL", master, setup_error);
    _assert("Setting role to master...          ", "AT+ROLE=1", master, setup_error);
    _assert("Setting pin to 1205...             ", "AT+PSWD=\"1205\"", master, setup_error);
    _assert("Setting connection mode to bind... ", "AT+CMODE=0", master, setup_error);
    _assert("Binding to address " + slave_addr + "...", "AT+BIND=" + slave_addr_param, master, setup_error);

    master.end();
    Serial.println();

    if (setup_error) Serial.println("[ERROR]: Failed to setup bluetooth modules. sry");
    else Serial.println("[SUCCESS]: Finished setup!");

    delay(TIMEOUT_MILLIS);
    exit(setup_error);
}

void loop() {}
