#!/bin/bash
echo -
PS3='			Please select your board to copy the config file to main/config.h: '
options=("Heltec V2 GPS" "Heltec V2 MAVLink" "M5 Jacek" "T-Beam New Oled" "T-Beam Oled" "Followme" "T-Beam V10" "T-Beam V10 LCD" "Quit no changes")
select opt in "${options[@]}"
do
    case $opt in
        "Heltec V2 GPS")
            cp cfg_boards/heltec_v2_gps.cfg main/config.h -f
            echo "Heltec V2 GPS"
	    break;;
        "Heltec V2 MAVLink")
            echo "Heltec V2 MAVLink"
            cp cfg_boards/heltec_v2_mavlink.cfg main/config.h -f
            break;;
	"M5 Jacek")
            echo "M5 Jacek"
            cp cfg_boards/m5_jacek.cfg main/config.h -f
            break;;
	"T-Beam New Oled")
            echo "T-Beam New Oled"
            cp cfg_boards/t-beam_new_oled.cfg main/config.h -f
            break;;
	"T-Beam Oled")
            echo "T-Beam Oled"
            cp cfg_boards/t-beam_oled.cfg main/config.h -f
            break;;
	"Followme")
            echo "Followme"
            cp cfg_boards/followme.cfg main/config.h -f
            break;;
	"T-Beam V10")
            echo "T-Beam V10"
            cp cfg_boards/t-beam_v10.cfg main/config.h -f
            break;;
	"T-Beam V10 LCD")
            echo "T-Beam V10 LCD"
            cp cfg_boards/t-beam_v10_lcd.cfg main/config.h -f
            break;;        
	"Quit no changes")
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done
echo -
echo - This are the first lines of file main/config.h
echo -
head -20 main/config.h