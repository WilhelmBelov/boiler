# boiler
Arduino Nano Program controls 8 relay, each of which is connected to a heater with a capacity of 4 kW each for heating the boiler to a given temperature. Also, a temperature and pressure sensor is connected for control.

## Projekt bauen
Folgen einem Schema, das unter ist. 
Wir brauchen zu Project:
 1. Arduino Nano mikrocontroller
 2. Acht Relais
 3. Temperature Sensor
 4. Druck Sensor
 5. Drei Knöpfen
 6. Relais für Pumpe
 7. Stromversorgung
 8. 5-Volt-Stromrichter
 9. Resistor 4,7 kOm
 10. Kabels
 
Empfehlung für Arduinos Stromversorgung ist 12 V. Mit weniger (5V) kann Man Displays Problem haben.
![plot](./image/Arduino.png)

Bauen die Relais. Arduino stellt HIGH (Hochspannung oder so wie VCC - 5V) auf Pins (D2-D9), damit die Relais offen. Und Arduino stellt LOW (Niedrig Spannung oder so wie GND) auf Pins (D2-D9), damit die Relais schließen. Strom fdes Relais ist separat, vileicht vom 5-Volt-Stromrichter. Am Anfang steht immer LOW.
![plot](./image/Relais.png)

Bauen Pumpe. Arduino stellt HIGH auf Pin D13. Es gibt auf Arduino eine Lumineszenzdiode gegen das Zeichen "L". Wenn ist die Lumineszenzdiode an, dann es bedeutet, das HIGH auf Pin D13 steht.
![plot](./image/Pumpe.png)

Bauen Druck Sensor. Eingang: 0-150 psi. Ausgang: 0,5-4,5 V linearer Spannungsausgang 0 psi gibt 0,5 V aus, 75 psi gibt 2,5 V aus, 150 psi gibt 4,5 V aus. Kauf Interet Platform: https://www.ebay.de/itm/384977371166?chn=ps&var=652861261376&_trkparms=ispr%3D1&amdata=enc%3A1zT6-Mq_zSuOHVV8qfwRf2A31&norover=1&mkevt=1&mkrid=707-134425-41852-0&mkcid=2&mkscid=101&itemid=652861261376_384977371166&targetid=1716911581159&device=c&mktype=pla&googleloc=9044181&poi=&campaignid=17943303986&mkgroupid=140642150118&rlsatarget=pla-1716911581159&abcId=9301060&merchantid=7364532&gclid=Cj0KCQiA_bieBhDSARIsADU4zLeEDFnfmYdcE7FFeAYhwb3y_nJ414Tkexn2Y3HduTCS-r6IILnmIqAaAkXBEALw_wcB
Arduino misst Spannung von Pin A6. Eingang: 0-5 V. Ausgang: 0-1023 Bit. linearer Spannungsausgang 0 V gibt 0 Bit aus, 2,5 V gibt 512 Bit aus, 5 V gibt 1023 Bit aus. Der Druck wird nach der Formel berechnet: Druck (Bar) = analogRead(DruckPin) * 10.3421/818.4 - 1.293, wo analogRead(DruckPin) ist Ausgang Bit, 10.3421 für Übersetzung von Psi bis Bar. Das Programm misst den Druck 10 mal pro 30 Secund und zeigt eine Mittel. Internet Site mit Analogpinsanleitung: https://developer.alexanderklimov.ru/arduino/analogreadserial.php
![plot](./image/Druck%20Sensor.png)

Bauen Temperature Sensor. Der Sensor eingerichtet mit 4,7 kOm resistor. Das Programm misst die Temperature 1 mal pro 2-3 Secund (mit weniger Zeit empfiehlt  man nicht). Der Sensor arbeitet mit I2C, das heist, darf man langes Kabel benutzen. Genauichkeit ist bis Hundertstel (,00). Internet Site mit Anschlussanleitung: https://xn--18-6kcdusowgbt1a4b.xn--p1ai/%D1%82%D0%B5%D1%80%D0%BC%D0%BE%D0%B4%D0%B0%D1%82%D1%87%D0%B8%D0%BA-%D0%B0%D1%80%D0%B4%D1%83%D0%B8%D0%BD%D0%BE/
Kauf Interet Platform: https://www.ebay.de/itm/115542104461
![plot](./image/Temperature%20Sensor.png)

Bauen Knöpfen. Es gibt links, rechts und OK Knöpfen. Reaktionszeit: 900ms. Reaktionszeit: 200ms, wenn man scrollen muss.
![plot](./image/Knöpfen.png)

Bauen Pin A0. Pin arbeitet als ein Voltmeter: wenn die Spannung weniger als 4,5 V relativ Arduinos GND ist, dann alle Relais wird macht aus und die Benachrichtigung über Gefährlich geschrieben wird. Aber es arbeitet nicht, wenn Pin A0 so einfach zu VCC des 5-Volt-Stromrichters verbinden.
![plot](./image/Strom%20Sicherung.png)

Bauen Pin A7. Pin arbeitet auch als ein Voltmeter: wenn die Spannung LOW ist (weniger als 2,5 V), dann alle Relais wird macht aus und die Benachrichtigung über Gefährlich geschrieben wird.Vileich braucht man AREF Pin benutzen, der auf dem Arduino ist der Pin, der den Referenzwert der Spannung für die analogen Eingänge darstellt.
![plot](./image/Pumpes%20Sicherung.png)

##Algorithmus
Am Anfang der Monitor print Hello und print, das der Benutzer setzt die gehaltene Heiztemperatur. Wenn der Benutzer während 5 Minuten setzt die gehaltene Temperatur nicht, dann stellt mann die gehaltende Temperatur 40°C. Danach print der Monitor standarte Seite: actuell Druck (zeigt man einmal pro 30 Secunden), actuell Temperatur (zeigt man einmal pro 3 Secunden).
Das Programm prüft 4 Parameters: 
  - Strom Pin A0. Wenn der nicht in Ordnung ist (weniger als 4,5 V), dann alle Relais wird macht aus und gedrückt die Benachrichtigung über Gefährlich. Wenn der Benutzer drückt lange (bis 5 Secund) die taste Ok, dann das Programm neu starten. 
  - Pumpe Pin A7. Wenn der nicht in Ordnung ist (weniger als 4,5 V), dann alle Relais wird macht aus und gedrückt während 15 Minuten die Benachrichtigung über Gefährlich, danach das Programm neu starten. Wenn der Benutzer drückt lange (bis 5 Secund) die taste Ok, dann das Programm geht weiter und diesen Parameter prüft nicht mehr. 
  - Temperatur. Wenn der nicht in Ordnung ist (weniger als -2 oder mehr als 90), dann alle Relais wird macht aus und gedrückt während 15 Minuten die Benachrichtigung über Gefährlich, danach das Programm neu starten. Wenn der Benutzer drückt lange (bis 5 Secund) die taste Ok, dann das Programm geht weiter und diesen Parameter prüft nicht mehr.
  - Druck. Wenn der nicht in Ordnung ist (weniger als 0,5 oder mehr als 5 Bar), dann alle Relais wird macht aus und gedrückt während 15 Minuten die Benachrichtigung über Gefährlich, danach das Programm neu starten. Wenn der Benutzer drückt lange (bis 5 Secund) die taste Ok, dann das Programm geht weiter und diesen Parameter prüft nicht mehr.
Wenn Benutzer wohlt etwas ändert, muss man dann die Taste Ok drücken, danach wählt <Ten ändert> oder <Temperatur ändert>. 
Für <Ten ändert> gibt der Wert von 0 kWatt bis 32 kWatt (einmal ändert pro 4 kWatt). Da macht der Benutzer nur maximale Wert für kWatt. Der Algorithmus enthält möglicherweise weniger Schatten als vom Benutzer festgelegt, aber nicht mehr. Actuell kWatt gedrückt auch hier. One Teng schaltet sich in einem Intervall von 10 Sekunden ein oder aus. Man kann Änderungen rückgängig machen.
Für <Temperatur ändert> gibt der Wert von 20°c bis 80°C. Actuell gehaltene Temperatur gedrückt auch hier. Man kann Änderungen rückgängig machen. Das Programm haltet diese Temperatur mit dem Algorithmus, der unter ist:
        1. Das Programm arbeitet in zwei Modi:
              - Heizmodus (Modus Heizung) - wenn die aktuelle Temperatur unter der eingestellten Heiztemperatur liegt, Das heißt, das System sollte beheizt werden.
              - Wartemodus - wenn die aktuelle Temperatur höher als die eingestellte Heiztemperatur ist, Das heißt, das System abkühlen lassen.
           Da die Temperatur nicht sofort abnimmt / ansteigt, sobald die Heizungen aus- / eingeschaltet werden, dann werden zwei Parameter eingeführt, die den Unterschied berücksichtigen:
              - deltaHeat - die maximale Temperaturänderung in Richtung Erhöhung beim Abschalten der HeizungenAnfangswert 4,5
              - deltaWait - die maximale Temperaturänderung in Richtung der Abnahme beim Einschalten der HeizungenAnfangswert 0,5
           Zusätzlich zu den oben genannten Modi werden die folgenden zwei Modi zur bequemen Berechnung der Parameter q und verwendet:
              - Temperaturerreichmodus - einstellen wann Der Benutzer setzt die gehaltene Heiztemperatur sowie beim Starten des Programms zurück.
              - Warmhaltemodus - wann Die aktuelle Temperatur liegt nahe an der eingestellten und muss nur gehalten werden.
        2. Im Heizbetrieb:
        2.1 Überprüfung der aktuellen Temperatur:
            Wenn es den eingestellten Wert (+0,5) um deltaHeat Grad überschreitet, wird in den Standby-Modus gewechselt, definiere zwei Parameter preview_temperature und deltaHeat. preview_temperature ist im Wesentlichen gleich aktuelle Temperatur. Wenn die aktuelle Temperatur den Modus erreicht, dann deltaHeat unverändert lassen, sonst wird der Initialwert von deltaHeat gesetzt, gleich dem Überschuss der aktuellen Temperatur über die eingestellte (+0,5 - deltaHeat).
        2.2 Wenn die aktuelle Temperatur die eingestellte Temperatur nicht überschritten hat, wird deltaHeat gemessen, gleich der Differenz zwischen die niedrigstmögliche aktuelle Temperatur und preview_temperature (preview_temperature wird ermittelt als aktuelle Temperatur beim Umschalten des Modus). Auch, wenn der aktuelle Temperaturhaltemodus (Warmhaltemodus), dann wird die Erlaubnis gesetzt, nur ein Heizelement einzuschalten. Wenn es nicht genug und die Temperatur fällt weiter, dann wird der Auflösung ein weiteres Heizelement hinzugefügt, und so weiter bis zu 8.
        2.3 Ein enthaltenes Heizelement wird hinzugefügt, vorbehaltlich einer Reihe von Bedingungen:
               - Zum Einschalten wird das erste ausgeschaltete Heizelement am Anfang der Liste gesucht und eingeschaltet so viele Heizungen wie vom Benutzer erlaubt (Priorität).
               - Ein Heizelement wird eingeschaltet, wenn die Anzahl der angeschlossenen Heizelemente die vom Programm erlaubte Anzahl nicht überschreitet in Absatz 2.2 und wenn der aktuelle Heizmodus und seit dem letzten Ein-/Ausschalten vergangen ist mindestens 10 Sekunden. Der Algorithmus enthält möglicherweise weniger Schatten als vom Benutzer festgelegt, aber nicht mehr.
        3. Im Wartemodus:
        3.1 Überprüfung der aktuellen Temperatur: Wenn es um deltaWait Grad niedriger als der eingestellte Wert (-4,5) geworden ist, wechselt es in den Heizmodus auch die Erlaubnis einstellen, nur ein Heizelement einzuschalten und auch definiere zwei Parameter preview_temperature und deltaWait. preview_temperature ist im Wesentlichen gleich aktuelle Temperatur. Wenn die aktuelle Temperatur den Modus erreicht, dann deltaWait unverändert lassen, sonst wird der Initialwert von deltaWait gesetzt, gleich dem Überschuss der eingestellten Temperatur über die aktuelle (-4,5 + deltaWait).
        3.2 Wenn die aktuelle Temperatur immer noch höher als die eingestellte Temperatur ist, dann wird deltaHeat gemessen, gleich der Differenz zwischen maximale aktuelle Temperatur und preview_temperature (preview_temperature ist definiert als aktuelle Temperatur beim Umschalten des Modus).
        3.3 Ein Heizelement wird hinzugefügt, und wenn der aktuelle Standby-Modus und seit dem letzten Mindestens 10 Sekunden verstrichen Ein/Aus. Zum Ausschalten wird das erste eingeschaltete Heizelement gesucht vom Ende der Liste.     
       
Das Hinzufügen von (+0,5) in Absatz 2.1 und (-4,5) in Absatz 2.1 bedeutet, dass die Temperatur mit Unterstützung wird Variieren Sie von Heating_Temperature-4.5 bis Heating_Temperature+0.5 Grad, wobei Heating_temperature - eingestellte Heiztemperatur.
