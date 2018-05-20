/*
Code by M. Saybolt
//
This is the arduino code used to translate sensor and i/o data from the Crazyflie
to commands for the goose-buster robot hardware. It is inspired by Cherokey-Rover
project, whose code is based off of crazyCar coded by /u/evoggy of the Bitcraze team.

*/

#define PM1_EN  4   // M1 direction, low, high
#define PM1_PWM 5   // M1 PWM, 0-255
#define PM2_PWM 6   // M2 PWM, 0-255
#define PM2_EN  7   // M2 direction, low, high
#define PSERVO_AZIMUTH 8 // TODO FIX
#define PSERVO_ELEVATION 9 //TODO fix/confirm
#define PACCY_RELAY 10 // TODO Fix/confirm

// Init char queue for reading RS232 from Crazyflie
String  charQueue= "";
char c;
int motorSpeed = 0;

void setup()
{
    // Open the serial connection to the CF2
    Serial.begin(19200); 
    Serial.println("GOOSE BUSTER.. Deploy!");

    pinMode(PM1_EN, OUTPUT);
    pinMode(PM1_PWM, OUTPUT);
    pinMode(PM2_EN, OUTPUT);
    pinMode(PM2_PWM, OUTPUT);
    pinMode(PSERVO_AZIMUTH, OUTPUT);
    pinMode(PSERVO_ELEVATION, OUTPUT);
    pinMode(PACCY_RELAY, OUTPUT);

    // PWM of 0 gives 0 speed to Cherokey motors
    motorInit(0);
}

void loop()
{
    while (Serial.available() > 0)
    {
        c = (char)Serial.read();
        if (c == '\n') // The packet coming over the line is finished,
        {
            // pass, insert cases to handle over serial line
            Serial.println("Command: ");
            Serial.println(charQueue);

            Serial.println("Char: ");
            // Declare char array
            // Will be populated with one 'packet' of user input data from the Crazyflie
            /* Could cut out the Arduino with the Sabertooth motor controllers, but still needed for the servos.
               Since Arduino is easier for most to debug, the servo control and accessory is just sent over as raw values for processing here
               Less stuff to fail over serial line, seems more intuitive to me. This arduino code is less complicated than CF code.
               Also this archetecture offers larger selection of motor controllers instead of requiring a Sabertooth, at the added cost
               of requiring an Arduino 'middleman'.

               If the CF code is simplified, could pass raw controller values into here, turn up roll/pitch in app
               and have more granular control over robot instead of assuming constant (non-default for Android) value of
               30. 
            */
            // [leftmotorDir(F,B,S), leftMotorMagnitude(0-9), rightMotorDir, rightMotorMagnitude, rightStickX, rightStickY, accessoryBool, \n]
            // char charArr[5];
            char charArr[8];
            charQueue.toCharArray(charArr, 8);
            Serial.println(charQueue);

            // Acquire speeds from serial packet to set and tell user. Must adjust max pitch/roll correctly for this static mapping to work
            Serial.print("SpeedM1: ");
            int speedM1 = charArr[2];
            // speedM1 = (speedM1 - 48) * 28; // Copied verbatim from evoggy, not sure what this is for exactly
            // See picture of scope screen
            // Need to map these 0-9-Null values to 0-255, unless thats what this does!
            speedM1 = speedM1 * 28; // Scale so 9 is ~255
            Serial.print("SpeedM2: ");
            int speedM2 = charArr[3];
            // speedM2 = (speedM2 - 48) * 28; //dunno why this, but 28 showed up in my version too!
            speedM2 = speedM2 * 28; // Scale so 9 is ~255

            // Print the new speed values for user on Serial monitor
            Serial.print("SpeedM1 adjusted: ");
            Serial.println(speedM1);
            Serial.print("SpeedM2 adjusted: ");
            Serial.println(speedM2);
           
            // Process direction.
            // charArr[0] and [1] hold direction for two motors. For me
            // watching the scope, B was forward and F was backward. I think these are hex, chosen by development order, and coincidentally look like (F)orward and (B)ackward
            // This should be changed in the CF firmware to make it easier to read, unless I'm missing something
            // Might tear all this up and just send raw floats for processing here. Keep strange build errors to a minimum
            // by compartmentalizing code into ecosystems connected by well-established comms (RS232 over CF UART)

            // With the extra bytes now, maybe makes sense to use elifs too, to structure timing better (or even makes a difference?)
            if (charArr[0] == 'S' && charArr[1] == 'S') // Never actually seen this case but copying evoggy
            {
                motorStop(0);
                Serial.print("Stop");
            }
            if (charArr[0] == 'F' && charArr[1] == 'F') 
            {
                // Something in the CF firmware makes FW on the controller (with default drone mapping
                // in cfclient) send Bs, and backward send F.. so easily flipped them here.
                motorReverse(speedM1, speedM2, 0);
                Serial.print("Forward");
            }
            if (charArr[0] == 'B' && charArr[1] == 'B') 
            {
                motorForward(speedM1, speedM2, 0);
                Serial.print("Reverse");
            }
            if (charArr[0] == 'B' && charArr[1] == 'F') 
            {
                motorTurnLeft(speedM1, speedM2, 0);
                Serial.print("TurnLeft");
            }
            if (charArr[0] == 'F' && charArr[1] == 'B') 
            {
                motorTurnRight(speedM1, speedM2, 0);
                Serial.print("TurnRight");
            }
            if (charArr[4] != 0)
            {
                // Right stick X axis position
                // If non-zero, a direction change is desired
                servoControl(PSERVO_AZIMUTH, charArr[4])
            }
            if (charArr[5] != 0)
            {
                // Right stick Y axis position
                servoControl(PSERVO_ELEVATION, charArr[5])
            }
            if (charArr[6] == 1)
            {
                // Fire relay
                digitalWrite(PACCY_RELAY, HIGH)
            }
            if (charArr[6] == 0)
            {
                digitalWrite(PACCY_RELAY, LOW)
            }

            Serial.println(";"); // Notify user of end
            charQueue = ""; // Reset the char queue
        }
        else
        {
            // IF not the end of a packet,
            // Add the chars into a char queue
            charQueue += c;
            Serial.println(c);
        }
    }
}

void servoControl(int servoPin, float userAxisInput)
{
    /*
    Main servo control function.

    This maps user input from an analog stick to movement with a servo.
    Without it, you'd have to hold the stick at a certain percentage to keep
    the servo still, aimed at that position.

    Need to process in own thread? somehow on arduino lol
    */
    int servoMin = 0 // Might want to move limits to an input to limit rotation on per-motor basis.
    int servoMax = 255
    int servoCurrent = 128
    //<pseudocode-ish since no internet>
    if (userAxisInput > 0) {
        // If the axis positive, increase the PWM duty cycle for the servo (turn it further)
        servoCurrent = servoCurrent + int(userAxisInput) // will probably need scaling factor
        // Check and correct if needed to not overrun/invalidate servo input
        if (servoCurrent >= servoMax) {
            servoCurrent = servoMax
        }
    }
    else if (userAxisInput < 0) {
        // If input is negative, decrease PWM duty cycle
        servoCurrent =  servoCurrent - int(userAxisInput) 
        if (servoCurrent <= servoMin) {
            servoCurrent = servoMin
        }
    }
    // else if userAxisInput == 0
        // servoCurrent = 
    // ^^^ not needed?
    analogWrite(servoPin, servoCurrent)
}

void motorInit(int power)
{
    // This function seems pointless/only used once
    analogWrite(PM1_PWM, power);
    analogWrite(PM2_PWM, power);
}

//void motor1update(
void motorStop(int duration)
{
    digitalWrite(PM1_PWM, 0);
    digitalWrite(PM2_PWM, 0);
    digitalWrite(PM1_EN, LOW);
    digitalWrite(PM2_EN, LOW);
    delay(duration);
}

void motorForward(int speedM1, int speedM2, int duration)
{
    digitalWrite(PM1_EN, HIGH);
    digitalWrite(PM2_EN, LOW);
    analogWrite(PM1_PWM, speedM1);
    analogWrite(PM2_PWM, speedM2);
    delay(duration);
}

void motorReverse(int speedM1, int speedM2, int duration)
{
    digitalWrite(PM1_EN, LOW);
    digitalWrite(PM2_EN, HIGH);
    analogWrite(PM1_PWM, speedM1);
    analogWrite(PM2_PWM, speedM2);
    delay(duration);
}

void motorTurnLeft(int speedM1, int speedM2, int duration)
{
    digitalWrite(PM1_EN, HIGH);
    digitalWrite(PM2_EN, HIGH);
    analogWrite(PM1_PWM, speedM1);
    analogWrite(PM2_PWM, speedM2);
    delay(duration);
}


void motorTurnRight(int speedM1, int speedM2, int duration)
{
    digitalWrite(PM1_EN, LOW);
    digitalWrite(PM2_EN, LOW);
    analogWrite(PM1_PWM, speedM1);
    analogWrite(PM2_PWM, speedM2);
    delay(duration);
}
