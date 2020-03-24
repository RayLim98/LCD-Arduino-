#include <stdio.h>
#include <avr/io.h>
#include <LiquidCrystal.h>
#include <math.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);            //Select Pins for LCD

int Milliseconds = 0;
int Seconds = 0;
int Minutes = 0;

int Motor_Current_Step = 0;                     //Current Step of the Motor 0-7
int Motor_Direction_Manual = 1;                 //0 = Clockwise, 1 = Anti-Clockwise
int Motor_Manual_Delay = 400;                   //Default Delay - Set to 250 for 1ms Delay on Timer 1 with 64 Prescaler

unsigned int Steps_Left = 0;                    //Steps Remaining for Step Mode
int Start_Toggle = 0;                           //0 = Not Running, 1 = Start

unsigned int Steps_Left_Link = 0;               //Steps Remaining for Link Mode

int Current_Step = 0;                           //Current Step of the Motor 0-4096
int Target_Angle = 0;                           //Current Target Angle of the Motor
int Target_Step = 0;                            //Target Step of the Motor

int Mode = 0;                                   //Current Mode Being Displayed

#define Clock 0                                 //Display Clock Mode
#define Student 1                               //Display Student ID Mode
#define Distance 2                              //Distance Mode
#define Continuous 3                            //Continuous Mode
#define Step 4                                  //Step Mode
#define Link 5                                  //Link Mode

void setup() {

  lcd.begin(16, 2);                             //Start the LCD

  ADMUX |= (1 << REFS0);                        //AVCC with External Capacitor at AREF Pin
  ADMUX |= (0 << ADLAR);                        //Result is Right Justified
  ADCSRA = 0b10000111;                          //ADC Enabled, Prescaler Set to 128

  TCCR2A = 0b00000000;                          //Timer 2 Setup
  TCCR2B = 0b00000101;                          //Timer 2 Set to 128 prescaler
  TCNT2 = 0;                                    //Timer 2 Initial Value
  TIMSK2 |= (1 << OCIE2A);                      //Enable Timer Compare Interrupt
  OCR2A = 125;                                  //Set Trigger for Interrupts - at 128 Prescaler, Counting to 125 is 1ms

  TCCR1A = 0b00000000;                          //Timer 1 Setup
  TCCR1B = 0b00000011;                          //Timer 1 Set to 64 prescaler
  TCNT1 = 0;                                    //Timer 1 Initial Value

  DDRD |= (1<<DDD0)|(1<<DDD1)|(1<<DDD2)|(1<<DDD3);  //Output Pins for the Motor on Port D Pins 0,1,2,3
}

void loop() {
  // put your main code here, to run repeatedly:

  Check_Select();                              //Check for Select Button Press for Mode Change
  
  switch (Mode) {

      case Clock:                              //Clock Mode
         lcd.setCursor(0,0);
         lcd.print("Clock");
         lcd.setCursor(0,1);
         lcd.print(Minutes);
         lcd.print(":");
         lcd.print(Seconds);
      break;
      case Student:                             //Student ID Mode
         Student_ID_Mode();
      break;
      case Distance:                            //Distance Sensor Mode
         Distance_Mode();
         lcd.setCursor(0,0);
         lcd.print("Distance");
      break;
      case Continuous:                          //Continuous Mode
         Check_Button_Continuous();             //Check for Relevant Button Presses
         Continuous_Mode();
         lcd.setCursor(0,0);
         lcd.print("Continuous");

         if (Motor_Direction_Manual == 1){      //Update Display with Current Direction

            lcd.setCursor(0,1);
            lcd.print("Anti-Clockwise");
         } else {

            lcd.setCursor(0,1);
            lcd.print("Clockwise");
         }
      break;
      case Step:                                //Step Mode
        Check_Button_Step();                    //Check for Relevant Button Presses
        lcd.setCursor(0,0);
        lcd.print("Step"); 
        Step_Mode();
        lcd.setCursor(0,1); 
        lcd.print(Steps_Left);
      break;
      case Link:                                //Link Mode         
         Link_Mode();
      break;
      
  }  
}

void Check_Select(){                            //Check for Select Button Press for Mode Change

  ADMUX &= ~(1 << MUX3);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX2);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX1);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX0);                        //Set Multiplexor Value to Pin A0
  ADCSRA |= (1 << ADSC);                        //Start ADC Conversion
  while(ADCSRA & (1 << ADSC));                  //Wait for ADC Conversion Interrupt
  ADCSRA |= (1 << ADIF);                        //Clear Interrupt

  if (ADC < 790 && ADC > 555){                  //Select Button
    
    TCNT1 = 0;
    while(TCNT1 < 50000);                       //Wait 200ms for Debounce
    Mode++;
    lcd.clear();
    if(Mode > Link){                            //When Next Mode is Selected at Last Mode, Go Back to The First Mode - Clock

      Mode = Clock;
    }
  }
}

void Check_Button_Continuous(){                 //Check for Relevant Button Presses for Continuous Mode

  ADMUX &= ~(1 << MUX3);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX2);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX1);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX0);                        //Set Multiplexor Value to Pin A0
  ADCSRA |= (1 << ADSC);                        //Start ADC Conversion
  while(ADCSRA & (1 << ADSC));                  //Wait for ADC Conversion Interrupt
  ADCSRA |= (1 << ADIF);                        //Clear Interrupt

  if (ADC < 50 && ADC >= 0){                    //Right Button

    TCNT1 = 0;
    while(TCNT1 < 50000);                       //Wait 200ms for Debounce
    if (Mode = Continuous){                     //Set Direction to Anti-Clockwise in Continuous Mode

      Motor_Direction_Manual = 1;
      lcd.clear();
    }    
  }
  if (ADC < 195 && ADC > 50){                   //Up Button

    TCNT1 = 0;
    while(TCNT1 < 50000);                       //Wait 200ms for Debounce
    if (Mode = Continuous){                     //Decrease Delay / Increase Rotation Speed in Continuous Mode

      if (Motor_Manual_Delay > 50){
        
        Motor_Manual_Delay = Motor_Manual_Delay - 50;        
      }      
    }    
  }
  if (ADC < 380 && ADC > 195){                  //Down Button

    TCNT1 = 0;
    while(TCNT1 < 50000);                       //Wait 200ms for Debounce
    if (Mode = Continuous){                     //Inrease Delay / Reduce Rotation Speed in Continuous Mode
        
        Motor_Manual_Delay = Motor_Manual_Delay + 50;              
    }    
  }
  if (ADC < 555 && ADC > 380){                  //Left Button
    
    TCNT1 = 0;
    while(TCNT1 < 50000);                       //Wait 200ms for Debounce
    if (Mode = Continuous){

      Motor_Direction_Manual = 0;
      lcd.clear();
    }    
  }
}

void Check_Button_Step(){                       //Check for Relevant Button Presses for Step Mode

  ADMUX &= ~(1 << MUX3);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX2);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX1);                        //Set Multiplexor Value to Pin A0
  ADMUX &= ~(1 << MUX0);                        //Set Multiplexor Value to Pin A0
  ADCSRA |= (1 << ADSC);                        //Start ADC Conversion
  while(ADCSRA & (1 << ADSC));                  //Wait for ADC Conversion Interrupt
  ADCSRA |= (1 << ADIF);                        //Clear Interrupt

  if (ADC < 50 && ADC >= 0){                     //Right Button

    TCNT1 = 0;
    while (TCNT1 < 50000);                      //Wait 200ms for Debounce
    if (Mode = Step){                           //Increase the Amount of Steps to Take in Step Mode

      Steps_Left = Steps_Left + 50;
    }
  }
  if (ADC < 195 && ADC > 50){                   //Up Button

    TCNT1 = 0;
    while (TCNT1 < 50000);                      //Wait 200ms for Debounce
    if (Mode = Step){                           //Reset Steps in Step Mode

      Steps_Left = 0;
      Start_Toggle = 0;
      lcd.clear();
    }
  }
  if (ADC < 380 && ADC > 195){                  //Down Button

    TCNT1 = 0;
    while (TCNT1 < 50000);                      //Wait 200ms for Debounce
    if (Mode = Step){                           //Set the Start Toggle Variable to the ON Position in Step Mode

      if (Steps_Left > 0){

        Start_Toggle = 1;
      }      
    }
  }
  if (ADC < 555 && ADC > 380){                  //Left Button
    
    TCNT1 = 0;
    while (TCNT1 < 50000);                       //Wait 200ms for Debounce
    if (Mode = Step){

      if (Steps_Left > 0){

        Steps_Left = Steps_Left - 50;
      }
    }
  }
}

ISR(TIMER2_COMPA_vect){

  if (TCNT2 >= 125){                            //At 128 prescaler, Counting to 125 is 1 ms on Timer 2

        Milliseconds++;
        TCNT2 = 0;  

        if (Milliseconds >= 1000){              //Increase Seconds When 1000ms Reached

          Seconds++;
          Milliseconds = 0;      
        }  
        if (Seconds >= 60){                     //Increase Minutes When 60s Reached

          Minutes++;
          Seconds = 0;
          lcd.clear();                          //Clear LCD to Ensure No Left Over Images
        }
      }    
}

void Student_ID_Mode(){

  lcd.setCursor(0,0);                           //Set Cursor at Line 1
  lcd.print("Anthony D");

  lcd.setCursor(0,1);                           //Set Cursor at Line 2
  lcd.print("12905269");
}

void Continuous_Mode(){  

    if (TCNT1 >= Motor_Manual_Delay){           //Step Motor at Speed Set by User

      Motor_Stepper();                          //Run Code to Turn the Motor
      TCNT1 = 0;                              
    }   
}

void Motor_Stepper(){                           //Motor Control

  for (int x = 0; x < 1; x++){

    switch (Motor_Current_Step){

      case 0:
          PORTD |= (1 << PORTD0);
          PORTD &= ~(1 << PORTD1);
          PORTD &= ~(1 << PORTD2);
          PORTD &= ~(1 << PORTD3);
      break;
      case 1:
          PORTD |= (1 << PORTD0) | (1 << PORTD1);
          PORTD &= ~(1 << PORTD2);
          PORTD &= ~(1 << PORTD3);
      break;
      case 2:
          PORTD |= (1 << PORTD1); 
          PORTD &= ~(1 << PORTD0);
          PORTD &= ~(1 << PORTD2);
          PORTD &= ~(1 << PORTD3);
      break;
      case 3:
          PORTD |=  (1 << PORTD1) | (1 << PORTD2);
          PORTD &= ~(1 << PORTD0);
          PORTD &= ~(1 << PORTD3);
      break;
      case 4:
          PORTD |= (1 << PORTD2);
          PORTD &= ~(1 << PORTD0);
          PORTD &= ~(1 << PORTD1);
          PORTD &= ~(1 << PORTD3);
      break;
      case 5:
          PORTD |= (1 << PORTD2) | (1 << PORTD3); 
          PORTD &= ~(1 << PORTD0);
          PORTD &= ~(1 << PORTD1);
      break;
      case 6:
          PORTD |= (1 << PORTD3); 
          PORTD &= ~(1 << PORTD0);
          PORTD &= ~(1 << PORTD1);
          PORTD &= ~(1 << PORTD2);
      break;
      case 7:
          PORTD |= (1 << PORTD0) | (1 << PORTD3);
          PORTD &= ~(1 << PORTD1);
          PORTD &= ~(1 << PORTD2); 
      break;
    }   
    Motor_Direction(); 
  }
}

void Motor_Direction(){

   if (Motor_Direction_Manual == 0){          //0 is Clockwise, 1 is Anti-Clockwise

      Motor_Current_Step++;
   } else {

      Motor_Current_Step--;
   }
   if (Motor_Current_Step > 7){

      Motor_Current_Step = 0;
   }
   if (Motor_Current_Step < 0){

      Motor_Current_Step = 7;
   }  
}

void Step_Mode(){

  if (Steps_Left >= 1){

      if (Start_Toggle == 1){

        Motor_Direction_Manual = 0;            //Set Direction to Clockwise
        //TCNT1 = 0;
        if (TCNT1 >= 250){                     //Run at 1ms Delay Between Steps on Timer 1 at 64 Prescaler

          Motor_Stepper();      
          TCNT1 = 0;

          Steps_Left--;      

          if (Steps_Left  == 999){             //If Number Goes from 4 Digits to 3 Digits Refresh Screen to Remove Lef Over Images

            lcd.clear();
          }
          if (Steps_Left == 99){               //If Number Goes from 3 Digits to 2 Digits Refresh Screen to Remove Lef Over Images

            lcd.clear();
          }
          if (Steps_Left == 9){                //If Number Goes from 2 Digits to 1 Digit Refresh Screen to Remove Lef Over Images

            lcd.clear();
          }
          if (Steps_Left == 0){

            Start_Toggle = 0;
            lcd.clear();
          }
        }    
      }
   }
}

void Distance_Mode(){                           //Distance Sensor for Distance Mode With Delays

  ADMUX &= ~(1 << MUX3);                        //Set Multiplexor Value to Pin A5
  ADMUX |=  (1 << MUX2);                        //Set Multiplexor Value to Pin A5
  ADMUX &= ~(1 << MUX1);                        //Set Multiplexor Value to Pin A5
  ADMUX |=  (1 << MUX0);                        //Set Multiplexor Value to Pin A5
  ADCSRA |= (1 << ADSC);                        //Start ADC Conversion
  while(ADCSRA & (1 << ADSC));                  //Wait for ADC Conversion Interrupt
  ADCSRA |= (1 << ADIF);                        //Clear Interrupt

  TCNT1 = 0;
  while(TCNT1 < 50000);                         //200ms Delay
  lcd.clear();

  float y = ADC;
  float Denominator_1 = y / 14507;
  float Power = 1 / 1.044;
  float Denominator = pow(Denominator_1, Power);
  float Calc = 1 / Denominator;

  lcd.setCursor(0,1);
  lcd.print(Calc, 1);                           //Print to 1 Decimal Place
  lcd.print("cm");
}

int Distance_Link(){                            //Distance Sensor Without Delays for Link Mode

  ADMUX &= ~(1 << MUX3);                        //Set Multiplexor Value to Pin A5
  ADMUX |=  (1 << MUX2);                        //Set Multiplexor Value to Pin A5
  ADMUX &= ~(1 << MUX1);                        //Set Multiplexor Value to Pin A5
  ADMUX |=  (1 << MUX0);                        //Set Multiplexor Value to Pin A5
  ADCSRA |= (1 << ADSC);                        //Start ADC Conversion
  while(ADCSRA & (1 << ADSC));                  //Wait for ADC Conversion Interrupt
  ADCSRA |= (1 << ADIF);                        //Clear Interrupt

  float y = ADC;
  float Denominator_1 = y / 14507;
  float Power = 1 / 1.044;
  float Denominator = pow(Denominator_1, Power);
  float Calc = 1 / Denominator;

  return Calc;
}

void Link_Mode(){
  //4096 Steps in a Rotation

  lcd.setCursor(0,0);
  lcd.print("Link (Degrees)");
  
  float DistanceCM = Distance_Link();

  if(DistanceCM < 150 && DistanceCM > 20){                                            //Only Rotate When Measured Distance is Within 20cm-150cm Limits
  
    Target_Angle = ((DistanceCM - 20) / (150 - 20)) * 360;                            //Normalise 20-150 to 0-360, Calculate Target as an Angle
    Target_Step = Target_Angle * (4096 / 360);                                        //Calculate the Target in Number of Steps

    if (Current_Step > Target_Step && Target_Step > 0 && Target_Step < 4096){         //Check to Ensure Target is Within Correct Range

      Steps_Left_Link = Current_Step - Target_Step;                                   //Calculate Number of Steps to Take to Reach Target
      Motor_Direction_Manual = 1;                                                     //Set Direction to Anti-Clockwise
      Link_Motor();
      Current_Step--;
    }
    if (Current_Step < Target_Step && Target_Step > 0 && Target_Step < 4096){         //Check to Ensure Target is Withing Correct Range

      Steps_Left_Link = Current_Step - Target_Step;                                   //Calculate Number of Steps to Take to Reach Target
      Motor_Direction_Manual = 0;                                                     //Set Direction to Clockwise
      Link_Motor();
      Current_Step++;
    }
    if (Current_Step == Target_Step && Target_Step > 0 && Target_Step < 4096){        //If Target Reached Set Distance to Target to 0

      Steps_Left_Link = 0;
    }

    float x = Current_Step;
    float Angle_Display = 360 - (x / 4096) * 360; //Convert To Angle Between 0 & 360 Where 360 is Close proximity and 0 is Far From Sensor
    lcd.setCursor(0,1);
    lcd.print(Angle_Display, 0);                  //Print With No Decimals
  }
}

void Link_Motor(){

  if (Steps_Left_Link >= 1){

    if (TCNT1 >= 250){                             //1ms Delay at 64 Prescaler

      Motor_Stepper();                             //Run Code to Turn the Motor
      TCNT1 = 0;                                   //Reset Timer 1 to 0

      Steps_Left_Link--;
    }
  }
}
