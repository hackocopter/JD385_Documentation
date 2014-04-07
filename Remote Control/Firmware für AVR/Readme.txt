Anbei ein paar Programme, um die Fernbedienung JD-385 simulieren/empfangen
zu können.

Als Hardware sowohl für Sender als auch Empfänger dient eine
kleine Schaltung mit einem ATMega328 (oder anderen, bitte im Programm
dann die SPI Belegung anpassen) und einem RFM70-Modul. Das Schaltbild 
ist in RFM70-rx-tx-sch.png zu finden.

Monitor.c dekodiert die Pakete der Fernsteuerung. Es wird nur Kanal 8 
empfangen. Da der Empfänger sehr breitbandig ist, hört er auf dem
Labortisch auch den hüpfenden Sender zuverlässig.

RFM70-fb-rx.c ist ein Empfänger, der auf den Sender synchronisiert und
ihm in der Frequenz folgt. Über Pulsfolgen an einigen Pins (siehe
Programm) kann man verfolgen, ob es zu Aussetzern kommt. Das Folgen lässt 
sich auch stoppen und man kann gezielt einzelne Kanäle einstellen.
Der Helptext im Programm sollte ausreichen.

RFM70-fb-tx.c ist ein Sender, der passend zu seiner Seriennummer hüpfen
kann, der aber auch auf festen Kanälen senden kann. Der Helptext zeigt
die möglichen Funktionen.

RFM70.c is eine Lib für das Modul. Sie stammt aus dem Internet und wurde
für die Fernbedienung modifiziert und an einigen Stellen etwas umgeräumt.
Nicht alle Funktionen sind getestet. Mit Überraschungen sollte man daher
rechnen.

Wer Fehler findet oder Anregungen hat, schickt mir bitte eine Mail. Dann
will ich das gern einarbeiten.

Der nächste Schritt ist, das alles in den originalen Sender zu stopfen.
Sein HF-Modul ist zum RFM70 kompatibel (_nicht_ zum nRF24L01!).
