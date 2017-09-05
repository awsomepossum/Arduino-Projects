//Add the following libraries to the respective folder for you operating system. 
//See http://arduino.cc/en/Guide/Environment

#include <FastSPI_LED2.h>     // FastSPI Library from http://code.google.com/p/fastspi/.
#include <EEPROM.h>           // Include the EEPROM library to enable the storing and retrevel of settings.
#include "Sodaq_DS3231.h"     // Contains the DateTime object definitions
#include <PololuLedStrip.h>   // Contains the methods and definitions that allow us to change the colors of the led strip
//#include <Wire.h>             //

#define INPUT_PIN 12
#define BUTTON_PIN 2

#define LED_COUNT 60
#define LED_OFFSET 32

#define NORMAL_CLOCK 1
#define MILLISECOND_CLOCK 2
#define SMOOTH_SECOND_CLOCK 3
#define BREATHING_CLOCK 4
#define SIMPLE_PENDULUM 5
#define RUNNING_RAINBOW 6
#define RAINBOW 7

#define NUM_CLOCK_MODES 6
#define START_CLOCK_MODE 3




// Create an ledStrip object and specify the pin it will use.
PololuLedStrip<12> ledStrip;

// Create a buffer for holding the colors (3 bytes per color).
rgb_color colors[LED_COUNT];

// Colors for hours, minutes, and seconds hands colors
rgb_color secondsColor = (rgb_color){255,0,0};
rgb_color minutesColor = (rgb_color){0,255,0};
rgb_color hoursColor = (rgb_color){0,0,255};
rgb_color millisecondsColor = (rgb_color){50,50,50};

char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

//stores old time to compare with new time
uint32_t old_ts;
uint32_t ts;

// which clock face to display
int clockMode = START_CLOCK_MODE;

// to see if a button is clicked
boolean lastButton = LOW;
boolean currentButton = LOW;

// calculated in methods
int cyclesPerSec = 0;
float cyclesPerSecFloat = 0;
long newSecTime = 0;
DateTime old;
boolean swingBack = true;




void setup () {
    pinMode(BUTTON_PIN, INPUT);
    rtc.begin();
    old = rtc.now();
    //Serial.begin(57600);
}




void loop () {
    //check what clock to display
    clockMode = getClockMode(clockMode);
  
    //get the current date-time
    DateTime now = rtc.now();
    ts = now.getEpoch();

    /*
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
    */

    // for millisecond functionality to work properly
    if (old_ts == 0 || old_ts != ts) {
        old_ts = ts;
        cyclesPerSec = millis() - newSecTime;
        cyclesPerSecFloat = (float) cyclesPerSec;
        newSecTime = millis();
    } 
  
    //reset all LED's to display nothing
    for(int i=0; i<LED_COUNT; i++) {
        colors[i] = (rgb_color){0,0,0};
    }
  
    //run the selected clock's code
    runSelectedClock(now, clockMode);

    // Write the colors to the LED strip.
    ledStrip.write(colors, LED_COUNT);
}




int getClockMode(int & clockMode) {
    currentButton = debounce(lastButton);

    if(lastButton == LOW && currentButton == HIGH){
        clockMode++;
        if(clockMode > NUM_CLOCK_MODES) {
            clockMode = 1;
        }
        //Serial.println(clockMode);
    }

    lastButton = currentButton;
    return clockMode;
}




boolean debounce( boolean & last ){
    boolean current = digitalRead(BUTTON_PIN);
    if( last != current){
        delay(5);
        current = digitalRead(BUTTON_PIN);
    }
    return current;
}




void runSelectedClock(DateTime & now, int & clockMode) {
  
    switch(clockMode) {
        case NORMAL_CLOCK:
          normalClock(now);
          break;
          
        case MILLISECOND_CLOCK:
          millisecondClock(now);
          break;
          
        case SMOOTH_SECOND_CLOCK:
          smoothSecondClock(now);
          break;
    
        case BREATHING_CLOCK:
          breathingClock(now);
          break;

        case SIMPLE_PENDULUM:
          simplePendulum(now);
          break;

        case RUNNING_RAINBOW:
          runRunningRainbow();
          break;
    
        case RAINBOW:
          //runRainbow();
          break;
          
        default:
          break;
    }

}




void normalClock(DateTime & now) {

    //for secondsColor
    colors[(now.second()+LED_OFFSET)%LED_COUNT] = secondsColor;

    //for hoursColor
    colors[(((now.hour()%12)*5)+LED_OFFSET)%LED_COUNT] = hoursColor;
    colors[(((now.hour()%12)*5)+59+LED_OFFSET)%LED_COUNT] = hoursColor;
    colors[(((now.hour()%12)*5)+1+LED_OFFSET)%LED_COUNT] = hoursColor;

    //for minutesColor
    colors[(now.minute()+LED_OFFSET)%LED_COUNT] = minutesColor;
}




void millisecondClock(DateTime & now) {
  
    // This divides by 733, but should be 1000 and not sure why???
    int subSeconds = ((millis() - newSecTime)*LED_COUNT)/cyclesPerSec;
  
    //for millisecondsColor
    colors[(subSeconds+LED_OFFSET)%LED_COUNT] = millisecondsColor;

    // call normalClock for hours, minutes, and seconds
    normalClock(now);
}




void smoothSecondClock(DateTime & now) {
  
    float fracOfSec;
    int subSeconds;
    int secondBrightness;
    int secondBrightness2;
  
    // This divides by 733, but should be 1000 and not sure why???
    subSeconds = (((millis() - newSecTime)*LED_COUNT)/cyclesPerSec)%LED_COUNT;
    fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;

    // Set the brightness of the adjacent seconds to create a smooth second transition
    if (subSeconds < cyclesPerSec) {
        secondBrightness = 50.0*(1.0+sin((3.14*fracOfSec)-1.57));
    }
    if (subSeconds < cyclesPerSec) {
        secondBrightness2 = 50.0*(1.0+sin((3.14*fracOfSec)+1.57));
    }

    // for second hand
    colors[(now.second()+LED_OFFSET)%LED_COUNT] = (rgb_color){secondBrightness,0,0};
    colors[(now.second()+59+LED_OFFSET)%LED_COUNT] = (rgb_color){secondBrightness2,0,0};
  
    // for hour hand
    colors[(((now.hour()%12)*5)+LED_OFFSET)%LED_COUNT] = hoursColor;
    colors[(((now.hour()%12)*5)+59+LED_OFFSET)%LED_COUNT] = hoursColor;
    colors[(((now.hour()%12)*5)+1+LED_OFFSET)%LED_COUNT] = hoursColor;
  
    // for minute hand
    colors[(now.minute()+LED_OFFSET)%LED_COUNT] = minutesColor;
}




void breathingClock(DateTime & now) {
    int breathBrightness = 15.0*(1.0+sin((3.14*millis()/2000.0)-1.57));
  
    // set every 5 led's to be breathing. change which led's every hour so its not the same ones
    for (int i = 0; i < LED_COUNT; i++) {

      int fiveMins = (i+(now.hour()%12)) % 5;
      if (fiveMins == 0) {
          colors[i] = (rgb_color){breathBrightness+5, breathBrightness+5, breathBrightness+5};
      }
      else {
          colors[i] = (rgb_color){0,0,0};
      }

    }

    // call normalClock for hours, minutes, and seconds
    normalClock(now);
}




void simplePendulum(DateTime now) {

    // This divides by 733, but should be 1000 and not sure why???
    int subSeconds = (((millis() - newSecTime)*LED_COUNT)/cyclesPerSec)/6;
    float fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;
    int pendulumPos = 0;

    if (now.second()!=old.second()) {
        old = now;
        
        if(swingBack) {
            swingBack = false;
        }
        else {
            swingBack = true;
        }
    }

    if(swingBack) {
        pendulumPos = 27.0 + 3.4*(1.0+sin((3.14*fracOfSec)-1.57));
    }
    else {
        pendulumPos = 27.0 + 3.4*(1.0+sin((3.14*fracOfSec)+1.57));
    }

    //for pendulum
    colors[(pendulumPos+LED_OFFSET)%LED_COUNT] = millisecondsColor;

    // call normalClock for hours, minutes, and seconds
    normalClock(now);
}




int state = 0;
int r = 255;
int g = 0;
int b = 0;
int pos = 0;
void runRunningRainbow() {

    int subSeconds = (((millis() - newSecTime)*LED_COUNT)/cyclesPerSec)%LED_COUNT;
  
    if(state == 0) {
        g++;
        if(g == 255) {
            state = 1;
        }
    }
    
    if(state == 1) {
        r--;
        if(r == 0) {
            state = 2;
        }
    }
    
    if(state == 2) {
        b++;
        if(b == 255) {
            state = 3;
        }
    }
    
    if(state == 3) {
        g--;
        if(g == 0) {
            state = 4;
        }
    }
    
    if(state == 4) {
        r++;
        if(r == 255) {
            state = 5;
        }
    }
    
    if(state == 5) {
        b--;
        if(b == 0) {
            state = 0;
        }
    }
    
    for(int i=0; i<LED_COUNT; i++) {
        if(i%5 == 0) {
            colors[(pos+i)%LED_COUNT] = (rgb_color){r,g,b};
        }
    }

    pos++;
    
    delay(50);
}










