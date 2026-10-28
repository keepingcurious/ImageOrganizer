# BildOrganisator

Dieses Programm hilft, Bilder nach Datum zu sortieren und Duplikate zu erkennen.

## Beschreibung

Ausgangssituation:  
Fotos wurden mit DSLR oder Smartphone aufgenommen oder per USB-Stick übertragen.
Unendlich viele Ordner mit unterschiedlicher Struktur und Nomenklatur.
Teilweise wurden Bilder auf externe Festplatten ausgelagert usw.
Kurzum: Chaos.

Ziel:  
Alle Bilder sollen zentral, chronologisch und einheitlich strukturiert gespeichert sein.

Umsetzung:  
Alle Ordner mit Bildern an einem Ort bündeln. Das Chaos konzentrieren.
Dann das Programm Ordnung schaffen lassen.

Wie:  
Das Programm liest die EXIF-Daten eines Bildes aus. 
Dies ist in der Regel genauer als das Änderungsdatum.
Danach erstellt es eine Ordnerstruktur:
```text
2026
| |______ 01
| |______ 02
|  ...
|  ...
| |______ 12
2025
| |______ 01
| |______ 02
|  ...
|  ...
| |______ 12
|
|
... 
  ```

Die Ordner werden nur erstellt, wenn ein Bild mit dem jeweiligen Datum existiert.

### Besonderheit  
Das Programm nutzt den MD-5 Algorithmus, um Duplikate zu erkennen.  
Wurde ein Bild schon einmal bearbeitet, so wird es nicht ein zweites Mal sortiert.




## Getting Started

### Dependencies

* Dieses Programm ist für Apple-Silicon optimiert.
* Es wird die EXIV2-Library genutzt.

### Installing

* Entsprechende Codezeilen zur Hardwarebeschleunigung müssen für andere OS ggf. auskommentiert werden.

### Executing program

* Das Programm fragt nach einem Quell-Ordner. Dieser beinhaltet die zu sortierenden Bilder
* Das Programm fragt nach einem Ziel-Ordner. Darin werden die sortierten Bilder gespeichert.


## Authors

Contributors names and contact info

keepingcurious  
mit Unterstützung von google gemini

## Version History

* 0.1
    * Initial Release

## License

This project is licensed under the [**GNU General Public License v3.0 (GPL-3.0)**] License.  
Weitere Details findest du in der Datei [LICENSE](LICENSE).

