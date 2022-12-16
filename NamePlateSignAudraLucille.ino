/*
 * 
 * Created by ChrisK
 * 
 * This is the code for the name plate lights I built for Audra and Lucille
 * 
 * The Micro Controller Module used here is a Teensey 4
 * https://www.pjrc.com/store/teensy40.html
 * 
 */

#include <OctoWS2811.h>
#include <FastLED.h>
#include <Arduino.h>
#include <Bounce.h>

template <EOrder RGB_ORDER = GRB,
              uint8_t CHIP = WS2811_800kHz>
class CTeensy4Controller : public CPixelLEDController<RGB_ORDER, 8, 0xFF>
{
    OctoWS2811 *pocto;

public:
    CTeensy4Controller(OctoWS2811 *_pocto)
        : pocto(_pocto){};

    virtual void init() {}
    virtual void showPixels(PixelController<RGB_ORDER, 8, 0xFF> &pixels)
    {

        uint32_t i = 0;
        while (pixels.has(1))
        {
            uint8_t r = pixels.loadAndScale0();
            uint8_t g = pixels.loadAndScale1();
            uint8_t b = pixels.loadAndScale2();
            //pocto->setPixel(i++, r, g, b);
            pocto->setPixel(i++, g, r, b);
            pixels.stepDithering();
            pixels.advanceData();
        }

        pocto->show();
    }
};

const int numPins = 7;
byte pinList[] = {2,7,8,9,10,11,12,14};

const int ledsPerStrip = 15;

const int audraLedsPerStrip[] = {15, 11, 15, 7, 14};
const int lucilleLedsPerStrip[] = {10, 11, 11, 6, 7, 7, 13};

CRGB rgbarray[numPins * ledsPerStrip];

const int colorButtonPin = 3;
const int animationModeButtonPin = 4;
const int brightnessButtonPin = 5;
const int speedButtonPin = 6;


const int ANI_NAME_CHANGE_FADE = 0;
const int ANI_NAME_CHANGE_RAINBOW_FADE = 1;
const int ANI_NAME_CHANGE_RED_FADE = 2;
const int ANI_NAME_CHANGE_GREEN_FADE = 3;
const int ANI_NAME_CHANGE_BLUE_FADE = 4;
const int ANI_COLOR_WIPE = 5;
const int ANI_LETTER_CHANGE = 6; 
const int ANI_LETTER_CHANGE_RANDOM = 7; 


int animationModes[] = { ANI_NAME_CHANGE_FADE, 
                         ANI_NAME_CHANGE_RAINBOW_FADE,
                         ANI_COLOR_WIPE, 
                         ANI_LETTER_CHANGE, 
                         ANI_LETTER_CHANGE_RANDOM };

int currentAnimationModeIndex = 0;

//int currentAnimationMode = ANI_NAME_CHANGE_FADE;

const int CHANNEL_RED = 0;
const int CHANNEL_GREEN = 1;
const int CHANNEL_BLUE = 2;

const int INDICATOR_STYLE_EVERY_PIXEL = 0;
const int INDICATOR_STYLE_SKIP_PIXELS = 1;

const int NAME_SELECTION_PIN_0 = 22;

// We use Bounce to handle bouncing signals from button - teensey is too fast!
Bounce colorButton = Bounce(colorButtonPin, 10);
Bounce animationModeButton = Bounce(animationModeButtonPin, 10);
Bounce brightnessButton = Bounce(brightnessButtonPin, 10 );
Bounce speedButton = Bounce( speedButtonPin, 10 );

// These buffers need to be large enough for all the pixels.
// The total number of pixels is "ledsPerStrip * numPins".
// Each pixel needs 3 bytes, so multiply by 3.  An "int" is
// 4 bytes, so divide by 4.  The array is created using "int"
// so the compiler will align it to 32 bit memory.
DMAMEM int displayMemory[ledsPerStrip * numPins * 3 / 4];
int drawingMemory[ledsPerStrip * numPins * 3 / 4];
OctoWS2811 octo(ledsPerStrip, displayMemory, drawingMemory, WS2811_RGB | WS2811_800kHz, numPins, pinList);

int startTime;
int advanceColorWipeCurrentPixel = 0;
int primaryColorIndex = 0;
CRGB primaryColor;
unsigned int brightnessIndex = 0;


int letterChangeCurrentLetter = 0;
int letterChangeRainbowCurrentColor = 0;

CRGB colorOptions[] = { CRGB::Red, CRGB::Yellow, CRGB::Orange, CRGB::Green, CRGB::Blue, CRGB::Indigo, CRGB::HotPink, CRGB::LightGoldenrodYellow, CRGB::LightSalmon, CRGB::LightGreen, CRGB::DeepSkyBlue, CRGB::MediumPurple }; 

int brightnessOptions[] = { 10, 20,  50, 100 };

int currentSpeedIndex = 2;

int speedOptions[] = {  500, 200, 50, 25, 10, 5, 1, };

int numColorOptions = sizeof(colorOptions) / sizeof( colorOptions[0] );
int numSpeedOptions = sizeof(speedOptions) / sizeof (speedOptions[0] );

CTeensy4Controller<RGB, WS2811_800kHz> *pcontroller;

uint8_t gHue = 0;


int currentName = -1;
const int NAME_AUDRA = 1;
const int NAME_LUCILLE = 0;

void setup() {
  
  pinMode( NAME_SELECTION_PIN_0, INPUT_PULLUP ); 
  delay(1000);
  currentName = digitalRead( NAME_SELECTION_PIN_0 );

  
  octo.begin();
  pcontroller = new CTeensy4Controller<RGB, WS2811_800kHz>(&octo);
  FastLED.setBrightness( brightnessOptions[brightnessIndex] );
  FastLED.addLeds(pcontroller, rgbarray, numPins * ledsPerStrip);
  FastLED.addLeds(pcontroller, rgbarray, numPins * ledsPerStrip);
  
  pinMode(colorButtonPin, INPUT);
  pinMode(speedButtonPin, INPUT );
  pinMode(animationModeButtonPin, INPUT);
  pinMode(brightnessButtonPin, INPUT );
  
  startTime = millis();
  Serial.begin(9600);
  randomSeed(analogRead(0));


  initNameChangeFade();

  if( currentName == NAME_AUDRA ) {
    Serial.println( "This is Audra's name sign" );    
  } else if (currentName == NAME_LUCILLE ) {
    Serial.println( "This is Lucille's name sign");
  }

}

void loop() {
  int currentTime = millis();
  int elapsedTime = currentTime - startTime;
  
  checkColorButton();
  checkAnimationModeButton();
  checkBrightnessButton();
  checkSpeedButton();
  
  if( elapsedTime > speedOptions[currentSpeedIndex] ) {
      switch( animationModes[currentAnimationModeIndex] ) {
        case ANI_COLOR_WIPE:
          advanceColorWipe( primaryColor );
          break;
        case ANI_LETTER_CHANGE:
          advanceLetterChange( primaryColor );
          break;
        case ANI_LETTER_CHANGE_RANDOM:
          advanceLetterChangeRandom();
          break;
        case ANI_NAME_CHANGE_FADE:
          advanceNameChangeFade();
          break;
        case ANI_NAME_CHANGE_RED_FADE:
          advanceNameChangeFadeSingleChannel(CHANNEL_RED);
          break;
        case ANI_NAME_CHANGE_GREEN_FADE:
          advanceNameChangeFadeSingleChannel(CHANNEL_GREEN);
          break;
        case ANI_NAME_CHANGE_BLUE_FADE:
          advanceNameChangeFadeSingleChannel(CHANNEL_BLUE);
          break;
        case ANI_NAME_CHANGE_RAINBOW_FADE:
          advanceNameChangeFadeRainbow();
      } 
      
     startTime = millis();   
  }
}

void checkBrightnessButton() {
  //return;
  brightnessButton.update();

  if( brightnessButton.fallingEdge() ) {
    Serial.println(  "brightness button change ");
    Serial.println( "size of brightnessOptions:" + (String) (sizeof(brightnessOptions) / sizeof( brightnessOptions[0]) ) );
    if( brightnessIndex < sizeof(brightnessOptions) / sizeof( brightnessOptions[0])   - 1 ) {
      brightnessIndex++;
    } else {
      brightnessIndex = 0;
    }
    Serial.println("new brightness index:" + (String) brightnessIndex );
    FastLED.setBrightness( brightnessOptions[brightnessIndex] );
  } 
}


/**
 * Shows an indicator on the display by shortly lighting up this number of pixels
 */
void showIndicator(int indicator, int indicatorStyle) {
    //first clear all LEDs
    fill_solid( rgbarray, ledsPerStrip * numPins, CRGB::Black );

    
    //now turn on the number of strips requested by the value of indicator variable
    for( int i = 0 ; i < indicator +1; i++ ) {
      int currentLed = 0;
      
      if( indicatorStyle == INDICATOR_STYLE_SKIP_PIXELS ) {
          //special cases hack
          if( currentName == NAME_AUDRA && i >= 7 ) {
            currentLed = ledsPerStrip + ((i-7)*2);
          } else if ( currentName ==NAME_LUCILLE && i >= 5 ) {
            currentLed = ledsPerStrip + ((i-5)*2);
          } else {
            currentLed = i * 2;
          }
      } else if (indicatorStyle == INDICATOR_STYLE_EVERY_PIXEL ) {
         currentLed = i;
      }
      rgbarray[currentLed] = primaryColor; 

    }
    FastLED.show();
    FastLED.delay(500);

    //clear all LEDs again
    fill_solid( rgbarray, ledsPerStrip * numPins, CRGB::Black );
    

}

void checkAnimationModeButton() {
  //return;
  animationModeButton.update();

  if( animationModeButton.fallingEdge() ) {
    Serial.println(  "animation mode button change ");
    Serial.println( sizeof( animationModes) - 1 );

    //Since this animation mode adjusts brightness, we need to set the brightness back to last setting
    if( animationModes[currentAnimationModeIndex] == ANI_NAME_CHANGE_FADE ) {
      FastLED.setBrightness( brightnessOptions[brightnessIndex] );
    }


    //advance the amnimation mode
    if( sizeof( animationModes)/sizeof(animationModes[0]) +1 < animationModes[currentAnimationModeIndex]  ) {
      currentAnimationModeIndex = 0;
    }
    else {
      currentAnimationModeIndex++;
    }

    showIndicator( currentAnimationModeIndex, INDICATOR_STYLE_SKIP_PIXELS );

    //start each aniamtion with sensible starting points
    switch ( animationModes[currentAnimationModeIndex] ) {
      case ANI_NAME_CHANGE_FADE:
        initNameChangeFade();  
        break;
      case ANI_COLOR_WIPE:
        initColorWipe();
        break;
      case ANI_NAME_CHANGE_RAINBOW_FADE:
        initNameChangeRainbowFade();
        break;
      case ANI_LETTER_CHANGE:
        initLetterChange();
        break;
      case ANI_LETTER_CHANGE_RANDOM:
        initLetterChange();
        break;
    }

int animationModes[] = { ANI_NAME_CHANGE_FADE, 
                         ANI_NAME_CHANGE_RAINBOW_FADE,
                         ANI_COLOR_WIPE, 
                         ANI_LETTER_CHANGE, 
                         ANI_LETTER_CHANGE_RANDOM };

    
    Serial.print("New mode:");
    Serial.println( animationModes[currentAnimationModeIndex] );
  } 
}

void checkSpeedButton() {
  //return;
  speedButton.update();

  if( speedButton.fallingEdge() ) {
    Serial.println(  "speed button change ");
    if( currentSpeedIndex < numSpeedOptions - 1   ) {
            currentSpeedIndex++;
          } else {
            currentSpeedIndex = 0;
          }
          showIndicator( currentSpeedIndex, INDICATOR_STYLE_EVERY_PIXEL );
          Serial.println( "New speed:" + (String) currentSpeedIndex );
  }
}

void checkColorButton() {
    //return;
    colorButton.update();
    //Serial.println( "buttonState:" + (String) currentButtonState );

    if( colorButton.fallingEdge() ) {
          if( primaryColorIndex < numColorOptions - 1   ) {
            Serial.println("++");
            primaryColorIndex++;
          } else {
            Serial.println("zero");
            primaryColorIndex = 0;
          }

          primaryColor = colorOptions[ primaryColorIndex ];
          
          Serial.println( "New color:" + (String) primaryColorIndex );
    } 

}


void initColorWipe() {
  advanceColorWipeCurrentPixel = 0;
  currentSpeedIndex = 3;
}

void advanceColorWipe( CRGB color ) {
    rgbarray[ advanceColorWipeCurrentPixel ] = color;
    FastLED.show();

    if( advanceColorWipeCurrentPixel < ledsPerStrip * numPins ) {
      advanceColorWipeCurrentPixel++;
    } else {
      advanceColorWipeCurrentPixel = 0;
    }
}


void initLetterChange() {
    brightnessIndex = 3;
    currentSpeedIndex = 1;
    primaryColor = colorOptions[ primaryColorIndex ];
}

void advanceLetterChange( CRGB color ) {
    Serial.print("advanceLetterChange:currentLEtter:");
    Serial.println( letterChangeCurrentLetter );
    for( int i = ledsPerStrip * letterChangeCurrentLetter; i < (letterChangeCurrentLetter * ledsPerStrip) + ledsPerStrip ; i ++ ) {
      //Serial.print("i");
      //Serial.println( i );
      rgbarray[ i ] = color;
    }
    FastLED.show();

    if( letterChangeCurrentLetter < numPins ) {
      letterChangeCurrentLetter ++;
    } else {
      letterChangeCurrentLetter = 0;
    }

}

void advanceLetterChangeRandom() {
  Serial.println("advanceLetterChange RANDOM");
  int randomIndex = random(0,numColorOptions);
  advanceLetterChange( colorOptions[ randomIndex ]  );
}


void advanceLetterChangeRainbow() {
  Serial.println("advanceLetterChange RAINBOW");
  advanceLetterChange( colorOptions[ letterChangeRainbowCurrentColor ]  );

  Serial.println( letterChangeRainbowCurrentColor );

  if( (int) letterChangeRainbowCurrentColor <  (int) (sizeof( colorOptions)/sizeof(colorOptions[0]) - 1  ) ) {  
    letterChangeRainbowCurrentColor++; 
  } else {
    letterChangeRainbowCurrentColor = 0;
  }
  Serial.println( letterChangeRainbowCurrentColor );
}



void setAllToPrimaryColor() {
  fill_solid( rgbarray, ledsPerStrip * numPins, primaryColor );     
}


uint8_t fadeBrightness = 5;
bool fadeOut = true; 

void initNameChangeFade() {
  brightnessIndex = 3;
  primaryColorIndex = 5;
  currentSpeedIndex = 3;
  primaryColor = colorOptions[ primaryColorIndex ];
}


void advanceNameChangeFade() {
    
    if( fadeBrightness <= 5 ) {
       fadeOut = false;
    } else if ( fadeBrightness >= brightnessOptions[brightnessIndex] ) {
      fadeOut = true;
    }

    if( fadeOut ) {
      fadeBrightness--;
    } else {
      fadeBrightness++;
    }
   
    //initiallize if no color set
    primaryColor = colorOptions[ primaryColorIndex ];
    setAllToPrimaryColor();

    FastLED.setBrightness( fadeBrightness );     
    FastLED.show();
}


int getRandomColor(uint8_t color) {
  //0 is down, 1 is up
  int upOrDown = random(-1,2);
  uint8_t newColor;

  Serial.print("old color");
  Serial.println( color );



  if( color >= 250 ) {
    newColor = 249;  
  } else if ( color == 0 ) {
    newColor = 6; 
  } else {
    newColor = color + upOrDown*5; 
  }

  Serial.print("new color");
  Serial.println( newColor );
  return newColor;    
}


bool colorDirection = true;

/**
 * Returns a new color between 0-255 in the "direction" specificed by colorDirection
 */
uint8_t seeSawColor(uint8_t color) {
  uint8_t newColor = -1;

  if( color == 0 ) {
    colorDirection = true;
  } else if ( color == 255 ) {
    colorDirection = false;
  }

  if( colorDirection ) {
    newColor = color + 1;
  } else {
    newColor = color - 1;
  }

  return newColor;
}

void advanceNameChangeFadeSingleChannel(int channel) {
    uint8_t currentR =  primaryColor.r;
    uint8_t currentG =  primaryColor.g;
    uint8_t currentB =  primaryColor.b;
        
    for( int i = 0; i < ledsPerStrip * numPins ; i++ ) {
      if( channel == CHANNEL_RED ) {
        rgbarray[i].setRGB( seeSawColor(currentR), currentG, currentB ); 
      } else if (channel == CHANNEL_GREEN ) {
        rgbarray[i].setRGB( currentR, seeSawColor(currentG), currentB );
      } else if ( channel == CHANNEL_BLUE ) {
        rgbarray[i].setRGB( currentR, currentG, seeSawColor(currentB)  );  
      }
    }
    primaryColor = rgbarray[0];

    
    //Serial.println("R:" + (String) rgbarray[0].r + " G:" + (String) rgbarray[0].g + " B:" + (String) rgbarray[0].b ); 

    FastLED.show();
}


void initNameChangeRainbowFade() {
  brightnessIndex = 1;
  primaryColorIndex = 6;
  currentSpeedIndex = 5;
  primaryColor = CRGB( 125, 125, 125 );
}

void advanceNameChangeFadeRainbow() {
  int channel = random(0,3);
  advanceNameChangeFadeSingleChannel( channel );
}
