#include "thermo.h"

//Non-modified libraries (have to be present in the arduino libraries folder)
#include <LiquidCrystal.h>

bool menuBlankState = 0;                                                      // Backup used to determine if some fields are already blanked out and no update is needed (menu in mode #2)
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
	lcd.setCursor(RIGHT_COLUMN_MARGIN + (ap > 9_ui8 ? (ap > 99_ui8 ? 0_ui8 : 1_ui8) : 2_ui8), 1);
	lcd.print(power < 0 ? '-' : ' ');
	lcd.print(power);
	lcd.print('%');
	prevPower = power;
}

void lcd_process_cursor_position()                             // Setting the cursor position, including shift chosen according to the field being edited
{
	absoluteCursorY = ((menuState == MENU_STATE_CALIBRATION) || (menuState == MENU_STATE_POWER)) ? 1 : 0;
	//if ((menuState == MENU_STATE_MODE) && (relativeCursorX > 1_ui8)) relativeCursorX = 1;         // A workaround for mode field
	uint8_t margin = 0;
	switch (menuState)
	{
	case MENU_STATE_MODE:
		++margin;
	case MENU_STATE_POWER:
		margin += RIGHT_COLUMN_MARGIN;
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
	if ((cursorType != CURSOR_NONE) && (channelIndex == CHANNEL_DS18B20))              // Blank out input and power fields if menu is enabled in mode #2 (due to non-updating values)
	{
		if (menuBlankState)
		{
			if (menuState == MENU_STATE_CALIBRATION) print_double(prevInputValue, LEFT_COLUMN_MARGIN, 1); // Don't blank out input field when user's editing calibration values
			if (regulationMode == MODE_MANUAL) pwrprint();     // always update power value in manual mode (otherwise it will appear to be stuck while in fact it will be changing)
		}
		else
		{
			for (uint8_t i = 0; i < LBLVALUE_LEN; i++)
			{
				lcd.setCursor(LEFT_COLUMN_MARGIN + i, 1);
				lcd_send_space();
			}
			for (uint8_t i = 0; i < LBLPOWER_LEN; i++)
			{
				lcd.setCursor(RIGHT_COLUMN_MARGIN + i, 1);
				lcd_send_space();
			}
			menuBlankState = true;
		}
	}
	else
	{
		if ((Input != prevInputValue) || menuBlankState)   // Remember to fill blanked fields
		{
			prevInputValue = Input;
			print_double(prevInputValue, LEFT_COLUMN_MARGIN, 1);
		}
		if ((power != prevPower) || menuBlankState)
		{
			pwrprint();
		}
		menuBlankState = 0;
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
			//char b[5];
			//sprintf(b, " %c%1u ", convert_regulation_mode(regulationMode), channelIndex);
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