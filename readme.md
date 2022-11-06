# CAN bus node for Hyundai Ioniq motor CAN

- receives message from CAN
- calculates energy values from the measured voltage and current
- sends some data via serial interface

# Features:
- listen to Hyundai Ioniqs motor CAN bus with 500kBaud
- extracting specific CAN signals
- accumulating energy, based on voltage and current
- store accumulated values non-volatile, triggered by timeout of the BMS message
- output the data to serial line as pairs of names and values
- Battery live data: U, I, P, SOC, BatteryMaxTemp
- Cumulated data: ECharge, EDrive
- Vehicle data: speed, ODO
- Device Statistics: Uptime, EepromWriteCounter
- calculate Ri (based on voltage variation due to current jumps), collect and serial report Ri in list

# Hardware
ATMega32, MCP2515 CAN controller via SPI, MCP2551 CAN transceiver

# Change Log:
- 2022-08-30
  -- newly created (based on ESP32 arduino code and hausbus project)
- 2022-08-31
  -- added Ri calculation
- 2022-11-06
  -- collected all lib files in the main directory, to have a complete bundle
  -- added to github.com/uhi22/IoniqMotorCAN

# Todo
* There are some cases, where the EEPROM write is interrupted by power-loss. Solve the dataloss, e.g. by
     maintaining redundant blocks, where always one instance is present, no matter when the power-loss happens.
* Calculate the integral over the dissipated power over the Ri
* Calculate the Ri also in case of limited available power

