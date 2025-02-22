#include <wiringPi.h>
#include <iostream>
using namespace std;

const int Motor_Pin = 4; // Choose the appropriate pin for PWM

int main(int argc, char *argv[]) {
    // Initialize WiringPi
    if (wiringPiSetup() == -1) {
        cout << "WiringPi setup failed!" << endl;
        return -1;
    }

    // Set the pin mode to PWM
    pinMode(Motor_Pin, PWM_OUTPUT);

    // Set the PWM range to 255 (0-255 range)
    pwmSetRange(255);
    pwmSetClock(192);  // Optional: Control the PWM frequency (depends on your needs)

    cout << "Motor is connected and will be running!!! " << Motor_Pin << endl;

    while (true) {
        // Change the PWM duty cycle, for example, setting it to 50% (128 out of 255)
        //pwmWrite(Motor_Pin, 128);  // Set motor speed to 50% duty cycle
        //cout << "Motor Pin PWM Value: 128 (50% duty cycle)" << endl;

        //delay(1000);  // Wait for 1 second

        // Set the motor to maximum speed
        pwmWrite(Motor_Pin, 255);  // Set to 100% duty cycle (maximum speed)
        cout << "Motor Pin PWM Value: 255 (100% duty cycle)" << endl;

        delay(1000);  // Wait for 1 second

        // Set the motor to minimum speed (off)
        pwmWrite(Motor_Pin, 0);  // Set to 0% duty cycle (motor off)
        cout << "Motor Pin PWM Value: 0 (0% duty cycle, motor off)" << endl;

        delay(1000);  // Wait for 1 second
    }

    return 0;
}
