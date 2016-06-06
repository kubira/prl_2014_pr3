/*
 * Soubor:    mm.c
 * Autor:     Radim KUBIŠ, xkubis03
 * Vytvořeno: 5. dubna 2014
 *
 * Popis:
 *    Tento program byl vytvořen jako 3. projekt do předmětu Paralelní
 *    a distribuované algoritmy (PRL).
 *
 *    Náplní projektu bylo vytvořit pomocí knihovny OpenMPI program
 *    implementující algoritmus Carry Look Ahead Parallel Binary Adder,
 *    který byl prezentován na přednášce.
 *
 * Poznámka:
 *    Pro změnu mezi měřením délky trvání algoritmu a výpisem výsledku
 *    je třeba zakomentovat/odkomentovat řádky s výskytem slova MEASUREMENT
 *    v komentáři.
 */

#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// Tag pro zasílání vlastních bitů X jednotlivým procesorům
#define TAGX 0
// Tag pro zasílání vlastních bitů Y jednotlivým procesorům
#define TAGY 1
// Tag pro zasílání hodnoty D
#define TAGD 2
// Hodnota S = stop
#define SVAL 0
// Hodnota P = propagate
#define PVAL 1
// Hodnota G = generate
#define GVAL 2
// Tabulka binární asociativní operace sumy prefixů
const int SPGtable[3][3] = {
     /*S*/ /*P*/ /*G*/
    {SVAL, SVAL, SVAL}, /*S*/
    {SVAL, PVAL, GVAL}, /*P*/
    {GVAL, GVAL, GVAL}  /*G*/
};
// Tabulka pro X a Y
const int XYtable[2][2] = {
    /*0*/  /*1*/
    {SVAL, PVAL}, /*0*/
    {PVAL, GVAL}  /*1*/
};

/*
 * Funkce pro výpočet a výpis doby trvání řazení vstupní posloupnosti čísel.
 *
 * start - struktura s hodnotami počátečního času algoritmu
 * stop  - struktura s hodnotami koncového času algoritmu
 *
 */
void getTimeDiff(struct timespec *start, struct timespec *stop) {
    // Proměnná pro uložení doby trvání řadícího algoritmu
    double diff = 0;

    // Výpočet rozdílu celých sekund a převod na nanosekundy
    diff = ((stop->tv_sec - start->tv_sec) * 1000000000);
    // Výpočet rozdílu nanosekund a přičtení rozdílu do celkové doby trvání
    diff += (stop->tv_nsec-start->tv_nsec);

    // Výpis doby trvání algoritmu v milisekundách na standardní výstup
    fprintf(stdout, "%f\n", (diff / 1000000));
}

/*
 * Funkce main - hlavní funkce programu.
 *
 * argc - počet vstupních parametrů příkazové řádky
 * argv - pole vstupních parametrů příkazové řádky
 *
 * Návratová hodnota:
 *         0 - program proběhl v pořádku
 *     jinak - program byl ukončen s chybou
 */
int main(int argc, char *argv[]) {
    // Proměnná pro uložení ranku procesoru
    int myRank = 0;
    // Proměnná pro uložení hodnoty bitu X procesoru
    int X = 0;
    // Proměnná pro uložení hodnoty bitu Y procesoru
    int Y = 0;
    // Proměnná pro uložení hodnoty bitu Z procesoru
    int Z = 0;
    // Proměnná pro uložení hodnoty bitu C procesoru
    int C = 0;
    // Proměnná pro uložení hodnoty bitu D procesoru
    int D = 0;
    // Pomocná proměnná pro uložení levé hodnoty D
    int leftD = 0;
    // Pomocná proměnná pro uložení pravé hodnoty D
    int rightD = 0;
    // Proměnná pro načítaný znak ze souboru
    int inputChar = 0;
    // Ukazatel na vstupní hodnotu X
    int *inputX = NULL;
    // Ukazatel na vstupní hodnotu Y
    int *inputY = NULL;
    // Ukazatel na vstupní soubor
    FILE *inputFile = NULL;
    // Proměnná pro uložení počtu vstupních bitů
    int numberOfBits = 0;
    // Proměnná pro uložení počtu procesorů algoritmu
    int numberOfProcessors = 0;
    // Proměnná pro uložení ranku prvního listového procesoru
    int firstListRank = 0;
    // Proměnná pro stav MPI
    MPI_Status stat;

    // Inicializace MPI
    MPI_Init(&argc, &argv);
    // Získání počtu procesorů algoritmu
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcessors);
    // Získání ranku procesoru
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    // Výpočet počtu vstupních bitů
    numberOfBits = (numberOfProcessors + 1) / 2;
    // Výpočet ranku prvního listového procesoru
    firstListRank = numberOfBits - 1;

    // Rozdělení větví programu na kořenový, vnitřní a listový uzel
    if(myRank == 0) {
        // VĚTEV PRO KOŘENOVÝ UZEL STROMU

        // Struktura pro počáteční čas algoritmu
        struct timespec start;
        // Struktura pro koncový čas algoritmu
        struct timespec finish;

        // Alokace místa pro vstupní hodnotu X
        inputX = (int*)malloc(numberOfBits * sizeof(int));
        // Kontrola alokace
        if(inputX == NULL) {
            // Tisk chyby
            perror("inputX malloc");
            // Ukončení s chybou
            exit(EXIT_FAILURE);
        }
        // Alokace místa pro vstupní hodnotu Y
        inputY = (int*)malloc(numberOfBits * sizeof(int));
        // Kontrola alokace
        if(inputY == NULL) {
            // Uvolnění paměti po inputX
            free(inputX);
            // Tisk chyby
            perror("inputY malloc");
            // Ukončení s chybou
            exit(EXIT_FAILURE);
        }

        // Otevření souboru s čísly
        inputFile = fopen("numbers", "r");
        // Kontrola otevření
        if(inputFile == NULL) {
            // Uvolnění paměti po inputX
            free(inputX);
            // Uvolnění paměti po inputY
            free(inputY);
            // Tisk chyby
            perror("fopen");
            // Ukončení s chybou
            exit(EXIT_FAILURE);
        }

        // Načítání hodnoty X ze souboru
        for(int i = 0; i < numberOfBits; i++) {
            // Načtení jednoho bitu hodnoty X
            // a úprava ze znaku na číslo
            inputX[i] = (fgetc(inputFile) - '0');
            // Kontrola načtení hodnoty
            if(inputX[i] == EOF) {
                // Uvolnění paměti po inputX
                free(inputX);
                // Uvolnění paměti po inputY
                free(inputY);
                // Tisk chyby
                perror("inputX fgetc EOF");
                // Uzavření souboru
                if(fclose(inputFile) != 0) {
                    // Tisk chyby
                    perror("fclose inputFile");
                }
                // Ukončení s chybou
                exit(EXIT_FAILURE);
            }
        }
        // Přeskočení dalších čísel a znaku konce řádku
        while(fgetc(inputFile) != '\n');
        // Načítání hodnoty Y ze souboru
        for(int i = 0; i < numberOfBits; i++) {
            // Načtení jednoho bitu hodnoty Y
            // a úprava ze znaku na číslo
            inputY[i] = (fgetc(inputFile) - '0');
            // Kontrola načtení hodnoty
            if(inputX[i] == EOF) {
                // Uvolnění paměti po inputX
                free(inputX);
                // Uvolnění paměti po inputY
                free(inputY);
                // Tisk chyby
                perror("inputY fgetc EOF");
                // Uzavření souboru
                if(fclose(inputFile) != 0) {
                    // Tisk chyby
                    perror("fclose inputFile");
                }
                // Ukončení s chybou
                exit(EXIT_FAILURE);
            }
        }

        // Uzavření souboru s čísly
        if(fclose(inputFile) != 0) {
            // Tisk chyby
            perror("fclose inputFile");
        }

        // Pokud je zadáno více než jeden bit
        if(numberOfBits > 1) {
            // Zasílání hodnot bitů X a Y jednotlivým listovým procesorům
            for(int i = 0; i < numberOfBits; i++) {
                // Zaslání hodnoty bitu X
                MPI_Send(&inputX[i], 1, MPI_INT, (firstListRank + i), TAGX, MPI_COMM_WORLD);
                // Zaslání hodnoty bitu Y
                MPI_Send(&inputY[i], 1, MPI_INT, (firstListRank + i), TAGY, MPI_COMM_WORLD);
            }

            // Bariéra pro počátek měření času
            // MPI_Barrier(MPI_COMM_WORLD);  /* MEASUREMENT */
            // Inicializace počátečního času algoritmu
            // clock_gettime(CLOCK_MONOTONIC_RAW, &start);  /* MEASUREMENT */

            // Příjem hodnoty D od levého synovského uzlu
            MPI_Recv(&leftD, 1, MPI_INT, ((myRank * 2) + 1), TAGD, MPI_COMM_WORLD, &stat);
            // Příjem hodnoty D od pravého synovského uzlu
            MPI_Recv(&rightD, 1, MPI_INT, ((myRank * 2) + 2), TAGD, MPI_COMM_WORLD, &stat);

            // Nastavení hodnoty D na neutrální prvek
            D = PVAL;

            // Získání hodnoty, která se zasílá levému synovskému uzlu
            leftD = SPGtable[rightD][D];

            // Zaslání hodnoty levému synovskému uzlu
            MPI_Send(&leftD, 1, MPI_INT, ((myRank * 2) + 1), TAGD, MPI_COMM_WORLD);
            // Zaslání hodnoty pravému synovskému uzlu
            MPI_Send(&D, 1, MPI_INT, ((myRank * 2) + 2), TAGD, MPI_COMM_WORLD);
        } else {
            // POKUD MÁM POUZE JEDEN BIT NA VSTUPU

            // Inicializace počátečního času algoritmu
            // clock_gettime(CLOCK_MONOTONIC_RAW, &start);  /* MEASUREMENT */

            // Výpočet hodnoty Z pouze z bitů X a Y
            Z = inputX[0] + inputY[0];

            // Pokud došlo k přetečení
            if(Z == 2) {  /* MEASUREMENT */
                // Tisk informace o přetečení počtu bitů
                fprintf(stdout, "overflow\n");  /* MEASUREMENT */
            }  /* MEASUREMENT */

            // Tisk identifikace procesoru a bitu sčítaných čísel
            fprintf(stdout, "%d:%d\n", myRank, (Z % 2));  /* MEASUREMENT */
        }

        // Bariéra pro měření délky trvání algoritmu
        // MPI_Barrier(MPI_COMM_WORLD); /* MEASUREMENT */

        // Inicializace koncového času algoritmu
        // clock_gettime(CLOCK_MONOTONIC_RAW, &finish);  /* MEASUREMENT */

        // Volání funkce pro výpočet a výpis délky řazení algoritmu
        // getTimeDiff(&start, &finish);  /* MEASUREMENT */

        // Uvolnění místa v paměti po vstupní hodnotě X
        free(inputX);
        // Uvolnění místa v paměti po vstupní hodnotě Y
        free(inputY);
    } else if (myRank >= firstListRank) {
        // VĚTEV PROGRAMU PRO LISTOVÉ UZLY STROMU

        // Příjem hodnoty bitu X
        MPI_Recv(&X, 1, MPI_INT, 0, TAGX, MPI_COMM_WORLD, &stat);
        // Příjem hodnoty bitu Y
        MPI_Recv(&Y, 1, MPI_INT, 0, TAGY, MPI_COMM_WORLD, &stat);

        // Bariéra pro počátek měření času
        // MPI_Barrier(MPI_COMM_WORLD);  /* MEASUREMENT */

        // Získání hodnoty D podle X a Y
        D = XYtable[X][Y];

        // Zaslání hodnoty D rodičovskému uzlu
        MPI_Send(&D, 1, MPI_INT, ((myRank - 1) / 2), TAGD, MPI_COMM_WORLD);

        // Příjem nové hodnoty D od rodičovského uzlu
        MPI_Recv(&D, 1, MPI_INT, ((myRank - 1) / 2), TAGD, MPI_COMM_WORLD, &stat);

        // Podle nové přijaté hodnoty D se nastaví bit přenosu
        if(D == GVAL) {
            // Přenos nastal
            C = 1;
        } else {
            // Přenos nenastal
            C = 0;
        }

        // Výpočet hodnoty Z z bitů X, Y a C
        Z = X + Y + C;

        // Pokud jsem první listový uzel,
        // hlídám přetečení počtu bitů čísla
        if(myRank == firstListRank && Z > 1) {  /* MEASUREMENT */
            // Tisk informace o přetečení počtu bitů
            fprintf(stdout, "overflow\n");  /* MEASUREMENT */
        }  /* MEASUREMENT */

        // Tisk identifikace procesoru a bitu sčítaných čísel
        fprintf(stdout, "%d:%d\n", myRank, (Z % 2));  /* MEASUREMENT */

        // Čekání na bariéře
        // MPI_Barrier(MPI_COMM_WORLD);  /* MEASUREMENT */
    } else {
        // VĚTEV PROGRAMU PRO VNITŘNÍ UZLY STROMU

        // Bariéra pro počátek měření času
        // MPI_Barrier(MPI_COMM_WORLD);  /* MEASUREMENT */

        // Příjem hodnoty D od levého synovského uzlu
        MPI_Recv(&leftD, 1, MPI_INT, ((myRank * 2) + 1), TAGD, MPI_COMM_WORLD, &stat);
        // Příjem hodnoty D od pravého synovského uzlu
        MPI_Recv(&rightD, 1, MPI_INT, ((myRank * 2) + 2), TAGD, MPI_COMM_WORLD, &stat);

        // Získání hodnoty D podle hodnot synovských uzlů
        D = SPGtable[leftD][rightD];

        // Zaslání hodnoty D rodičovskému uzlu
        MPI_Send(&D, 1, MPI_INT, ((myRank - 1) / 2), TAGD, MPI_COMM_WORLD);

        // Příjem hodnoty D od rodičovského uzlu
        MPI_Recv(&D, 1, MPI_INT, ((myRank - 1) / 2), TAGD, MPI_COMM_WORLD, &stat);

        // Získání hodnoty, která se zasílá levému synovskému uzlu
        leftD = SPGtable[rightD][D];

        // Zaslání hodnoty levému synovskému uzlu
        MPI_Send(&leftD, 1, MPI_INT, ((myRank * 2) + 1), TAGD, MPI_COMM_WORLD);
        // Zaslání hodnoty pravému synovskému uzlu
        MPI_Send(&D, 1, MPI_INT, ((myRank * 2) + 2), TAGD, MPI_COMM_WORLD);

        // Čekání na bariéře
        // MPI_Barrier(MPI_COMM_WORLD);  /* MEASUREMENT */
    }

    // Ukončení práce s MPI
    MPI_Finalize();
    // Ukončení programu bez chyby
    exit(EXIT_SUCCESS);
}
