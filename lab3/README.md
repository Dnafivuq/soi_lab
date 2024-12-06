# lab 3

## Rozwiązanie
Plik main.c zawiera implementacje w c programu typu "producent-konsument" na procesach, używając semaforów. Zostały zaimplementowane struktury oraz metody pozwalające na dowolną kombinacje ilości konsumentów, producentów i kolejek. W funkcji `main()` został zrealizowany problem używający 3 producentów, 4 kosumentów i 4 buforów będacych kolejkami FIFO.


### Implementacja bufora (kolejki)
Każdy bufor tworzony jest za pomocą metody `buffer_constr()`, która allokuje pamięć w sektorze współdzielonym między procesami-dziećmi utworzonymi przez funkcje `fork()`. Metoda pozwala na ustawienie wielkości kolejki, oraz jej nazwy (pojedyńczy znak). Co więcej, każdy bufor posiada własny semafor `mutex`, oraz `full` i `empty`.


### Dostępu do bufora
Producenci i konsumenci są synchronizowani za pomocą semaforów znajdujących się w każdym buforze.
 - producenci mogą dodawać swoje przedmioty, tylko i wyłącznie kiedy kolejka jest niepełna,
 - konsumenci mogą zabierać przedmioty z kolejki, tylko i wyłącznie jeśli jest niepusta.

Za pomocą semafora `mutex` zapobiegamy wyścigom związanym z dostępem do danych znajdujących się w buforze (tylko jeden obiekt może jednocześnie modyfikować zawartość kolejki). Semafory `full` i `empty` zapewniają, że producent, bądź też konsument nie zablokuje kolejki jeśli warunek dodawania, zabierania nie zostanie spełniony. 


### Uwagi
Użyta w rozwiązaniu struktura oraz metody semafora, pochodzą z biblioteki `<semaphore.h>`, dodatkowo zostały użyte inne metody np. `mmap()`, które instnieją tylko na systemach Linux'owych, co oznacza, że program może nie działać poprawnie na innych systemach np. windows. 

### Użycie
Aby wytestować działanie programu należy uruchomić plik `main.out` (`./main.out`).


### Zawartość repozytorium
 - main.c - plik implementujący bufory
 - main.out - skompilowany plik `main.c`