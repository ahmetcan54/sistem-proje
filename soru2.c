/*
 * Soru 2: fork() + sinyal yönetimi
 *
 * Her SIGALRM'da: child SIGSTOP → 2 sn bekle → child SIGCONT
 * ~10 saniye sonra child'a SIGINT gönderilir.
 *
 * Öğrenci: Ahmet Can Alpay  |  b251210350
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

/* ------------------------------------------------------------------ */
/*  Child tarafı                                                        */
/* ------------------------------------------------------------------ */

static void child_sigint_handler(int sig)
{
    (void)sig;
    signal(SIGINT, child_sigint_handler); /* handler'ı yeniden kur */
    printf("Çocuk: SIGINT alındı ancak devam ediliyor...\n");
    fflush(stdout);
    exit(0);
}

static void child_sigcont_handler(int sig)
{
    (void)sig;
    signal(SIGCONT, child_sigcont_handler);
    printf("Çocuk: İşlem yeniden başlatıldı\n");
    fflush(stdout);
}

static void run_child(void)
{
    signal(SIGINT,  child_sigint_handler);
    signal(SIGCONT, child_sigcont_handler);

    int sayac = 0;
    while (1) {
        printf("Çocuk sayacı: %d\n", sayac++);
        fflush(stdout);
        sleep(1);
    }
}

/* ------------------------------------------------------------------ */
/*  Parent tarafı                                                       */
/* ------------------------------------------------------------------ */

static pid_t child_pid;
static volatile sig_atomic_t bitmeli = 0;
static int alarm_sayaci = 0;

static void parent_alarm_handler(int sig)
{
    (void)sig;
    signal(SIGALRM, parent_alarm_handler); /* handler'ı yeniden kur */
    alarm_sayaci++;

    /* Her alarm döngüsünde: durdur → 2 sn bekle → devam ettir */
    printf("Ebeveyn: Çocuk durduruluyor...\n");
    fflush(stdout);
    kill(child_pid, SIGSTOP);

    sleep(2);

    printf("Ebeveyn: Çocuk devam ediyor...\n");
    fflush(stdout);
    kill(child_pid, SIGCONT);

    if (alarm_sayaci < 2) {
        alarm(3); /* bir sonraki döngü */
    } else {
        /* ~10 saniye doldu (3+2+3+2 = 10 sn) */
        printf("Ebeveyn: SIGINT gönderiliyor...\n");
        fflush(stdout);
        kill(child_pid, SIGINT);
        bitmeli = 1;
    }
}

int main(void)
{
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0)
        run_child();

    /* ---- EBEVEYN ---- */
    signal(SIGALRM, parent_alarm_handler);
    alarm(3); /* ilk zamanlayıcı */

    while (!bitmeli)
        pause();

    int durum;
    waitpid(child_pid, &durum, 0);
    printf("Ebeveyn: Çocuk sonlandırıldı.\n");

    return 0;
}
