### lab 2

## Rozwiązanie
Plik 'lab2.zip' zawiera obraz systemu minix z zaimplementowanymi zadaniem szeregowania piorytetowego.
Szeregowanie polega na przydzielaniu czasu procesowi według równania: `kwant czasu * piorytet`. Dodatkowo istnieją odpowiednie syscall'e pozwalające na odczyt i modyfikacje piorytetu danego procesu.

# Implementacja szeregowania
W pliku `/usr/src/kernel/proc.h` w strukturze proc zostały dodane 2 pola - `p_age` i `p_prio`, oraz zostały wprowadzone zmiany w `usr/src/kernel/proc.c` w funkcji `sched()`, tak aby uwzględniać nowy sposób szeregowania.

# Implementacja syscall'i oraz taskcall'i
Obydwa syscall'e zostały zdefiniowane w pliku `/usr/src/mm/main.c` na końcu pliku:
 - `do_getprio()`: zwraca piorytet procesu o podanym id,
 - `do_setprio()`: ustawia piorytet procesu o podanym id, na zadany piorytet.

Natomiast taskcall'e zdefiniowane są w pliku `/usr/src/kernel/system.c`:
 - `do_getprio()`,
 - `do_setprio()`,
w praktyce syscalle i taskcalle zwracają to samo z taką różnicą, że to taskcalle posiadają faktyczną logikę zmiany/odczytania piorytetu, wynika to z oczywistego faktu, że jądro pozwala na edytowanie tablicy procesów z poziomu kernela, nie MM.

# Użycie
Aby wytestować działanie zaimplementowanych syscall'i (taskcall'i) zostały zdefiniowane metody "biblioteczne" `getprio(pid)`, `setprio(pid, prio)` w pliku `usr/include/prio.h`, aby z nich skorzystać należy zawrzeć `#include <prio.h>` w pliku źródłowym własnego programu. Dodatkowo zostały utworzone przykładowe programy ukazujące testowanie zdefiniowanych metod w katalogu `/home`:
 - dummy.o: program uruchamiający proces o zadanym piorytecie (uwaga: posiada nieskończoną pętle!),
 - test.sh: skrypt shellowy odpalający 2 programy `dummy.o` w tle z zdefiniowanymi piorytetami,
 - getprio.o: program pozwalający na odczytanie piorytetu procesu o zadanym przez nas pid.
