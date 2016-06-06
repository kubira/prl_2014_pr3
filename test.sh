#!/bin/bash

################################################################################
#                                                                              #
# Soubor:   test.sh                                                            #
# Autor:    Radim KUBIŠ, xkubis03                                              #
# Vytvořen: 5. dubna 2014                                                      #
#                                                                              #
# Popis:                                                                       #
#    Tento skript slouží pro spuštění jednoho testu algoritmu                  #
#    Carry Look Ahead Parallel Binary Adder.                                   #
#                                                                              #
#    Jediný nepovinný parametr tohoto skriptu udává počet bitů                 #
#    sčítaných čísel.                                                          #
#                                                                              #
################################################################################

# Pokud nebyl zadán počet bitů sčítaných čísel
if [ $# -lt 1 ]; then
    # Nastavím počet na hodnotu 4
    number_of_bits=4;
# Pokud byl zadán požadovaný počet bitů sčítaných čísel
else
    # Nastavím tuto hodnotu pro test
    number_of_bits=$1;
fi;

# Kontrola počtu zadaných bitů > 0
if [ $number_of_bits -lt 1 ]; then
    # Tisk chyby
    echo "Chyba: Počet zadaných bitů '$number_of_bits' není validní"
    # Ukončení skriptu s chybou
    exit 1
fi;

# Přeložím zdrojový kód programu
mpicc --prefix /usr/local/share/OpenMPI -o mm mm.c -lrt -std=gnu99

# Vyrobím soubor 'numbers' s náhodnými čísly v binární soustavě pro sečtení
#echo -n "" > numbers
#for (( i=1; i<=$((number_of_bits*2)); i++ ))
#do
#  echo -n "$((RANDOM%2))" >> numbers
#  if [ $((i%$number_of_bits)) == 0 ]
#  then
#    echo "" >> numbers
#  fi
#done

#cat numbers

# Vypočítám počet potřebných procesorů na základě počtu bitů
number_of_processors=$((number_of_bits*2-1))

# Spustím test algoritmu
mpirun --prefix /usr/local/share/OpenMPI -np $number_of_processors mm

# Uklidím vytvořené soubory po ukončení testu
#rm -f mm numbers
rm -f mm
