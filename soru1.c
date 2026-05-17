/*
 * Soru 1: Dinamik NxN matris üzerinde pointer aritmetiği kullanarak
 * ana köşegen ve ters köşegen toplamını hesaplayan special_sum fonksiyonu.
 *
 * Kısıt: matris elemanlarına [][] sözdizimi KULLANILMAMALIDIR.
 * Tüm erişimler *(mat + i*cols + j) biçiminde yapılır.
 *
 * Öğrenci: Ahmet Can Alpay  |  b251210350
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * special_sum: NxN matrisin ana köşegen (i==j) ve ters köşegen (i+j==cols-1)
 * elemanlarını toplar. Kare matris için orta eleman (yalnızca tek boyutlu N'de
 * her iki köşegende bulunur) bir kez sayılır.
 *
 * Parametre:
 *   mat  - 1 boyutlu diziye sıkıştırılmış NxN matrisin başlangıç adresi
 *   rows - satır sayısı
 *   cols - sütun sayısı
 * Dönüş: toplam değer
 */
int special_sum(int *mat, int rows, int cols)
{
    int toplam = 0;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int ana_kosegen   = (i == j);              /* i == j  → ana köşegen */
            int ters_kosegen  = (i + j == cols - 1);   /* i+j == N-1 → ters köşegen */

            if (ana_kosegen || ters_kosegen) {
                /* Pointer aritmetiği ile erişim — [][] kullanılmıyor */
                toplam += *(mat + i * cols + j);
            }
        }
    }
    return toplam;
}

/* Matrisi ekrana yazar (debug/gösterim amaçlı, yine pointer aritmetiği) */
static void yazdir_matris(int *mat, int rows, int cols)
{
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%4d", *(mat + i * cols + j));
        }
        printf("\n");
    }
}

int main(void)
{
    int n;
    printf("Matris boyutunu girin (NxN): ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Gecersiz boyut.\n");
        return 1;
    }

    /* Dinamik bellek tahsisi */
    int *mat = malloc((size_t)n * (size_t)n * sizeof(int));
    if (!mat) {
        fprintf(stderr, "Bellek tahsisi basarisiz.\n");
        return 1;
    }

    printf("Matris elemanlarini satir satir girin (%dx%d = %d eleman):\n",
           n, n, n * n);
    for (int i = 0; i < n * n; i++) {
        if (scanf("%d", mat + i) != 1) {        /* pointer aritmetiği ile yazma */
            fprintf(stderr, "Okuma hatasi.\n");
            free(mat);
            return 1;
        }
    }

    printf("\nGirilen matris:\n");
    yazdir_matris(mat, n, n);

    int sonuc = special_sum(mat, n, n);
    printf("\nAna kosegen + ters kosegen toplami (cift sayilanlar once bir kez): %d\n",
           sonuc);

    free(mat);
    return 0;
}
