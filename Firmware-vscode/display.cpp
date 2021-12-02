#include "thermo.h"

//Non-modified libraries (have to be present in the arduino libraries folder)
#include <LiquidCrystal.h>

#pragma region Definitions

#define PIN_RS 7                //LCD pins
#define PIN_E 6
#define PIN_D4 2
#define PIN_D5 A3
#define PIN_D6 4
#define PIN_D7 5
#define LBL_SPACING 1_ui8 //" "
#define LBL_LEN 2_ui8 //"S:"
#define LBLVALUE_LEN 6_ui8 //"123.45"            //Display configuration
#define LBLVALUE_ACCESSIBLE 6_ui8
#define LBLMODE_LEN 4_ui8 //" N0 ", "ERR!"
#define LBLMODE_ACCESSIBLE 2_ui8
#define LBLPOWER_LEN 5_ui8 //" 100 " "+100%"
#define LBLPOWER_ACCESSIBLE 3_ui8
#define DECIMAL_PLACES 3_ui8 //"123"         //Index of the last character in decimal part or length of decimal part of setpoint and input (interchangeable due to comma presence)
#define LCD_X 16_ui8
#define LCD_Y 2_ui8
#define MENU_STATE_SETPOINT 0_ui8
#define MENU_STATE_MODE 1_ui8
#define MENU_STATE_CALIBRATION 2_ui8
#define MENU_STATE_POWER 3_ui8
#define RIGHT_COLUMN_OFFSET (LBL_LEN + LBLVALUE_LEN + LBL_SPACING)
#define RIGHT_COLUMN_MARGIN (RIGHT_COLUMN_OFFSET + LBL_LEN)
#define LEFT_COLUMN_OFFSET 0_ui8
#define LEFT_COLUMN_MARGIN (LEFT_COLUMN_OFFSET + LBL_LEN)

#pragma endregion

//bool menuBlankState = 0;                                                      // Backup used to determine if some fields are already blanked out and no update is needed (menu in mode #2)
int16_t encoderValue = 0;
uint8_t relativeCursorX = 0;                                                              //
uint8_t absoluteCursorY = 0;                                                              //
uint8_t prevRegulationMode = 0xff;
uint8_t menuState = MENU_STATE_SETPOINT;      // Menu state flag: 0 - setpoint, 1 - mode, 2 - calibration (input field), 3 - power (for manual mode)
LiquidCrystal lcd(PIN_RS, PIN_E, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

void print_double(double dbl, int x, int y);
void pwrprint();

void lcd_send_space()
{
	lcd.print(' ');
}

void lcd_init()
{
	lcd.begin(LCD_X, LCD_Y);
	lcd.clear();
}

void lcd_draw_message(const char* msg)
{
	lcd.clear();
	lcd.print(msg);
}

void lcd_draw_interface()
{
	lcd.clear();
	lcd.setCursor(LEFT_COLUMN_OFFSET, 0);
	lcd.print("S:");                                       //Print basic symbols on the screen
	lcd.setCursor(LEFT_COLUMN_OFFSET, 1);
	lcd.print("R:");
	lcd.setCursor(RIGHT_COLUMN_OFFSET, 1);
	lcd.print("P:");
	lcd.setCursor(RIGHT_COLUMN_OFFSET, 0);
	lcd.print("M:");
}

void lcd_process_cursor_type()                              // Cursor modes switching
{
	switch (cursorType)                              // 0 - disabled, 1 - underlining, 2 - underlining + blinking
	{
	case CURSOR_NONE:
		lcd.noBlink();
		lcd.noCursor();
		break;
	case CURSOR_UNDERLINE:
		lcd.noBlink();                        // Don't forget to turn previous mode off, because it isn't reset automatically
		lcd.cursor();
		break;
	case CURSOR_BLINK:
		lcd.blink();
	}
}

void print_double(double dbl, int x, int y)   //Encapsulated for convenience, prints a double on the screen at specified coordinates
{
	char b[7];
	dtostrf(dbl, 6, 2, b);
	lcd.setCursor(x, y);
	lcd.print(b);
}

void pwrprint()
{
	uint8_t ap = abs(power);
	lcd.setCursor(RIGHT_COLUMN_MARGIN, 1_ui8);
	uint8_t spaces = ap > 9_ui8 ? (ap > 99_ui8 ? 0_ui8 : 1_ui8) : 2_ui8;
	while (spaces-- > 0_ui8) //Clear previously printed characters that may not be overwritten
	{
		lcd.print(' ');
	}
	lcd.print(power < 0 ? '-' : ' ');
	lcd.print(ap);
	lcd.print('%');
	prevPower = power;
}

void lcd_process_cursor_position()                             // Setting the cursor position, including shift chosen according to the field being edited
{
	absoluteCursorY = ((menuState == MENU_STATE_CALIBRATION) || (menuState == MENU_STATE_POWER)) ? 1 : 0;
	uint8_t margin = 0;
	switch (menuState)
	{
	case MENU_STATE_MODE:
	case MENU_STATE_POWER:
		margin += RIGHT_COLUMN_MARGIN + 1_ui8; // + Sign or space
		break;
	default:
		margin = LEFT_COLUMN_MARGIN;
		break;
	}
	lcd.setCursor(relativeCursorX + margin, absoluteCursorY);
}

void lcd_process_fast()                              // Main routine for display logic
{
	int16_t encoderRawValue = encoder->getValue();                  // Get encoder and button value/state
	ClickEncoder::Button button = encoder->getButton();
	bool force = false;
	if (button != ClickEncoder::Open) {          // Button handling
												 /*#ifdef DEBUG
												 Serial.print("Button: ");
												 Serial.println(button);
												 #endif*/
		switch (button) {
		case ClickEncoder::Clicked:             // Clicked
			switch (cursorType)
			{
			case CURSOR_UNDERLINE:                             // If underlining cursor is on then switch to the blinking one
				cursorType = CURSOR_BLINK;
				break;
			case CURSOR_BLINK:
				cursorType = CURSOR_UNDERLINE;                            // If blinking is on, then do vice-versa
				break;
			default: break;
			}
			break;
		case ClickEncoder::DoubleClicked:       // Double-clicked
			if (cursorType == CURSOR_NONE)                      // Cycle: Off -> setpoint field -> mode field -> input field -> Off
			{
				cursorType = CURSOR_UNDERLINE;
			}
			else
			{
				if (((menuState > MENU_STATE_MODE) && (regulationMode != MODE_MANUAL)) || (menuState == MENU_STATE_POWER))
				{
					cursorType = CURSOR_NONE;
					menuState = MENU_STATE_SETPOINT;
				}
				else
				{
					++menuState;
				}
			}
			encoderValue = 0;
			force = true;
			break;
		default:
			break;
		}
	}
	if ((encoderRawValue != 0) || force)                         // Check if anything changed since last cycle
	{
		encoderValue += encoderRawValue;
		encoderValue %= LBLMODE_ACCESSIBLE * LBLPOWER_ACCESSIBLE * LBLVALUE_ACCESSIBLE; //Truncate the value to avoid overflow while preserving modular arithmetic properties
		/*#ifdef DEBUG
		Serial.println("Encoder!");
		#endif*/
		uint8_t bound = 1;     // Choose bounds: 6 chars for setpoint field, 2 characters for mode field, 6 characters for input field, 3 characters for power field
		switch (menuState)
		{
		case MENU_STATE_SETPOINT:
		case MENU_STATE_CALIBRATION:
			bound = LBLVALUE_ACCESSIBLE;
			break;
		case MENU_STATE_MODE:
			bound = LBLMODE_ACCESSIBLE;
			break;
		case MENU_STATE_POWER:
			bound = LBLPOWER_ACCESSIBLE;
		default:
			break;
		}
		switch (cursorType)                             // Determine what to do with the values
		{
		case CURSOR_UNDERLINE:                               // 1 - Move cursor
			relativeCursorX = (encoderValue >= 0 ? encoderValue % bound : (bound - 1 + (encoderValue + 1) % bound));
			if (relativeCursorX == 3)                            // Skip the comma in setpoint field
			{
				(encoderRawValue > 0) ? relativeCursorX++ : relativeCursorX--;
			}
			break;
		case CURSOR_BLINK:                            // 2 - Change value
		{
			int k = relativeCursorX;
			if (relativeCursorX > DECIMAL_PLACES) k--;    // Fix skipped comma index incrementation (in order to fit into multipliers array's bounds)
			switch (menuState)                      // Determine what value to change
			{
			case MENU_STATE_SETPOINT:                    // 0 - setpoint
				Setpoint += mult[k] * encoderRawValue;        // Change setpoint using array of multipliers chosen according to the cursor index
				break;
			case MENU_STATE_MODE:                    // 1 - mode
				switch (relativeCursorX)
				{
				case 0:                     // Scroll between N, A, D and M
					regulationMode = static_cast<uint8_t>(abs((regulationMode + encoderRawValue) % 4));
					break;
				case 1:                     // Scroll between 0 and 2
					channelIndex = static_cast<uint8_t>(abs((channelIndex + encoderRawValue) % 3));
					break;
				default:
					break;
				}
				break;
			case MENU_STATE_CALIBRATION:                      // 2 - calibration
				calibration[channelIndex] += mult[k] * encoderRawValue; //Similar to setpoint change
				break;
			case MENU_STATE_POWER:                      // 3 - power
				power += mult[k] * encoderRawValue;
				check_power();
				break;
			default: break;
			}
			break;
		}
		}
	}
}

void lcd_process_slow()
{
	if (Setpoint != prevSetpoint)                  // Searching for changed data and partially updating screen
	{
		prevSetpoint = Setpoint;
		print_double(prevSetpoint, 2, 0);
	}
	if (Input != prevInputValue) // Remember to fill blanked fields
	{
		prevInputValue = Input;
		print_double(prevInputValue, LEFT_COLUMN_MARGIN, 1);
	}
	if (power != prevPower)
	{
		pwrprint();
	}
	if (condition[0] && (errorStatusDelay > ERROR_TIMEOUT))                    // Major error sign - label "ERR!" in mode field
	{
		lcd.setCursor(RIGHT_COLUMN_MARGIN, 0);
		lcd.print("ERR!");
	}
	else
	{
		if ((channelIndex != prevChannelIndex) || (regulationMode != prevRegulationMode))             // Mode field structure (normal): "M: {N,A,D,M}[0..2]{!}"
		{
			lcd.setCursor(RIGHT_COLUMN_MARGIN, 0);
			lcd_send_space();
			lcd.print(convert_regulation_mode(regulationMode));
			lcd.print(channelIndex);
			lcd_send_space();
			prevChannelIndex = channelIndex;
			prevRegulationMode = regulationMode;
			if (!myPID.GetMode() == MANUAL) myPID.SetMode(AUTOMATIC); //Reset PID to fix a strange bug causing power to stay at ~20% after channel selection no matter how input changes
		}
		if ((condition[1] && cjc) || condition[2] || condition[3])                  // Warning sign - exclamation mark in the right upper corner of the screen
		{
			lcd.setCursor(RIGHT_COLUMN_MARGIN + LBLMODE_LEN, 0);
			lcd.print('!');
		}
	}
}

bool lcd_is_editing()
{
	return (cursorType != CURSOR_NONE);
}