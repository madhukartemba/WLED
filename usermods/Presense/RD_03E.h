#include <HardwareSerial.h>

class RD_O3E
{
private:
    float distance = 0;
    int status = 0;
    uint8_t RX_BUF[64] = {0};
    uint8_t RX_count = 0;
    uint8_t RX_temp;

    void setDistance(float d)
    {
        distance = d;
    }

    void setStatus(int s)
    {
        status = s;
    }

public:
    void setup(int rx, int tx)
    {
        Serial1.begin(256000, SERIAL_8N1, rx, tx);
    }

    void run()
    {
        if (Serial1.available())
        {
            RX_temp = Serial1.read();     // Read a byte from Serial1
            RX_BUF[RX_count++] = RX_temp; // Store it in the buffer

            // Check if we have a valid packet (minimum 5 bytes needed)
            if (RX_count >= 6)
            { // Ensure we have enough bytes to check for a complete frame
                // Check for valid frame: AA + distance (2 bytes) + gesture (1 byte) + 55
                // for (int i=0; i<8; i++) {
                //   Serial.print(RX_BUF[i], HEX);
                //   Serial.print(" ");
                // }
                // Serial.println();
                if (RX_BUF[0] == 0xAA && RX_BUF[1] == 0xAA && RX_BUF[5] == 0x55 && RX_BUF[6] == 0x55)
                {
                    // Extract distance and gesture
                    uint16_t range = (RX_BUF[4] << 8) | RX_BUF[3]; // Combine distance bytes in little-end format

                    // Convert distance to meters
                    float distanceInMeters = (float)range / 100; // Convert cm to meters
                    setDistance(distanceInMeters);

                    // Output distance in meters and gesture for debugging
                    // Serial.printf("Status: %d Distance: %.2f m\n", RX_BUF[2], distanceInMeters);
                    int status = RX_BUF[2];
                    setStatus(status);

                    // Reset buffer and count after processing a packet
                    memset(RX_BUF, 0x00, sizeof(RX_BUF));
                    RX_count = 0;
                }
                else
                {
                    // If we receive an unexpected byte, we can reset the buffer
                    if (RX_count >= sizeof(RX_BUF))
                    {
                        memset(RX_BUF, 0x00, sizeof(RX_BUF));
                        RX_count = 0;
                    }
                }
            }
        }
    }

    float getDistance()
    {
        return distance;
    }

    float getStatus()
    {
        return status;
    }

    bool isHumanPresent()
    {
        return status >= 1;
    }
};