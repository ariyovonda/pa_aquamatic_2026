@echo off
REM Dummy MQTT publisher for all aquaponics sensors
REM Send one JSON payload every 15 seconds

set "MQTT_HOST=localhost"
set "MQTT_PORT=1883"
set "TOPIC_PREFIX=aquaponics/sensors"
set "INTERVAL=15"
set "MOSQUITTO_CMD=C:\Program Files\Mosquitto\mosquitto_pub.exe"

set /a TEMPERATURE=280
set /a PH=750
set /a TDS=520
set /a DO=75
set /a TURBIDITY=24

echo Dummy MQTT Sensor Publisher
echo ============================
echo Broker: %MQTT_HOST%:%MQTT_PORT%
echo Topic prefix: %TOPIC_PREFIX%
echo Interval: %INTERVAL% seconds
echo.

:loop
REM update random drift values
set /a d=(%random% %% 5) - 2
set /a TEMPERATURE+=d
if %TEMPERATURE% lss 260 set /a TEMPERATURE=260
if %TEMPERATURE% gtr 300 set /a TEMPERATURE=300

set /a d=(%random% %% 5) - 2
set /a PH+=d
if %PH% lss 700 set /a PH=700
if %PH% gtr 800 set /a PH=800

set /a d=(%random% %% 21) - 10
set /a TDS+=d
if %TDS% lss 400 set /a TDS=400
if %TDS% gtr 800 set /a TDS=800

set /a d=(%random% %% 5) - 2
set /a DO+=d
if %DO% lss 50 set /a DO=50
if %DO% gtr 100 set /a DO=100

set /a d=(%random% %% 11) - 5
set /a TURBIDITY+=d
if %TURBIDITY% lss 0 set /a TURBIDITY=0
if %TURBIDITY% gtr 250 set /a TURBIDITY=250

set /a TEMP_INT=TEMPERATURE / 10
set /a TEMP_FRAC=TEMPERATURE %% 10
if %TEMP_FRAC% lss 10 set TEMP_FRAC=0%TEMP_FRAC%
set TEMP_STR=%TEMP_INT%.%TEMP_FRAC%

set /a PH_INT=PH / 100
set /a PH_FRAC=PH %% 100
if %PH_FRAC% lss 10 set PH_FRAC=0%PH_FRAC%
set PH_STR=%PH_INT%.%PH_FRAC%

set /a DO_INT=DO / 10
set /a DO_FRAC=DO %% 10
if %DO_FRAC% lss 10 set DO_FRAC=0%DO_FRAC%
set DO_STR=%DO_INT%.%DO_FRAC%

set /a TURB_INT=TURBIDITY / 10
set /a TURB_FRAC=TURBIDITY %% 10
if %TURB_FRAC% lss 10 set TURB_FRAC=0%TURB_FRAC%
set TURB_STR=%TURB_INT%.%TURB_FRAC%

set "TEMP_PAYLOAD={""value"": %TEMP_STR%}"
set "PH_PAYLOAD={""value"": %PH_STR%}"
set "TDS_PAYLOAD={""value"": %TDS%}"
set "DO_PAYLOAD={""value"": %DO_STR%}"
set "TURB_PAYLOAD={""value"": %TURB_STR%}"

"%MOSQUITTO_CMD%" -h %MQTT_HOST% -p %MQTT_PORT% -t "%TOPIC_PREFIX%/temperature" -m "%TEMP_PAYLOAD%" -q 1
"%MOSQUITTO_CMD%" -h %MQTT_HOST% -p %MQTT_PORT% -t "%TOPIC_PREFIX%/ph" -m "%PH_PAYLOAD%" -q 1
"%MOSQUITTO_CMD%" -h %MQTT_HOST% -p %MQTT_PORT% -t "%TOPIC_PREFIX%/tds" -m "%TDS_PAYLOAD%" -q 1
"%MOSQUITTO_CMD%" -h %MQTT_HOST% -p %MQTT_PORT% -t "%TOPIC_PREFIX%/do" -m "%DO_PAYLOAD%" -q 1
"%MOSQUITTO_CMD%" -h %MQTT_HOST% -p %MQTT_PORT% -t "%TOPIC_PREFIX%/turbidity" -m "%TURB_PAYLOAD%" -q 1
if errorlevel 1 (
    echo [ERROR] One or more publishes failed
) else (
    echo [%time%] Published temp=%TEMP_PAYLOAD%, ph=%PH_PAYLOAD%, tds=%TDS_PAYLOAD%, do=%DO_PAYLOAD%, turbidity=%TURB_PAYLOAD%
)

timeout /t %INTERVAL% /nobreak >nul
goto loop
