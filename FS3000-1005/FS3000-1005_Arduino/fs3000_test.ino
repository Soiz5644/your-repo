#include <fs3000.h>
// Connect Vin to 3-5VDC
// Connect GND to ground
// Connect SCL to I2C clock pin (A5 on UNO)
// Connect SDA to I2C data pin (A4 on UNO)

FS3000 fs;

void setup() {
    Serial.begin(115200);
    if (fs.begin() != true)
    {
        Serial.print("FS3000 initialize error.\r\n");
        while (1)
            ;
    }
    else
        Serial.print("FS3000 initialize register finished.\r\n");

}

void loop() {
	float speed;
		speed = fs.ReadData();
        Serial.print(speed);
		Serial.print(" m/s\r\n");
	delay(100);
}
