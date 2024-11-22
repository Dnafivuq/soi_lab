### lab 1

## Rozwiązanie
Plik 'lab1' zawiera obraz systemu minix z zaimplementowanymi zadaniami 1 i 3 z t1.txt

# Implementacja _syscall'i
Obydwa syscall'e zostały zdefiniowane w pliku `/usr/src/mm/main.c` na końcu pliku.
 - `do_maxchildproc()`: zwraca id i liczbe dzieci procesu o największej liczbie dzieci
 - `do_maxdescenproc()`: zwraca id procesu o największej liczbie potomków, pomijając id wskazanego procesu

# Użycie
Aby wytestować działanie zaimplementowanych syscall'i zostały utworzone odpowiednie programy w katalogu `/home`.
 - zad1: definicja programu w pliku `zad1.c`, aby uruchomić należy wpisać komende `./zad1.o`
 - zad2: definicja programu w pliku `zad2.c`, aby uruchomić należy wpisać komende `./zad2.o [id_wykluczonego_procesu]`
