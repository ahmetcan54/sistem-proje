/*
 * Soru 2: fork() + sinyal yönetimi
 *
 * Akış:
 *   3. sn  → Ebeveyn: SIGSTOP  (Çocuk durduruluyor)
 *   6. sn  → Ebeveyn: SIGCONT  (Çocuk devam ediyor)
 *   9. sn  → Ebeveyn: SIGSTOP  (tekrar durdur)
 *   10. sn → Ebeveyn: SIGCONT + SIGINT  (~10 sn sonunda sonlandır)
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
    printf("Çocuk: SIGINT alındı ancak devam ediliyor...\n");
    fflush(stdout);
    exit(0);
}

static void child_sigcont_handler(int sig)
{
    (void)sig;
    printf("Çocuk: İşlem yeniden başlatıldı\n");
    fflush(stdout);
}

static void run_child(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa.sa_handler = child_sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = child_sigcont_handler;
    sigaction(SIGCONT, &sa, NULL);

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
    alarm_sayaci++;

    switch (alarm_sayaci) {
    case 1:                             /* 3. saniye: durdur */
        printf("Ebeveyn: Çocuk durduruluyor...\n");
        fflush(stdout);
        kill(child_pid, SIGSTOP);
        alarm(3);
        break;

    case 2:                             /* 6. saniye: devam ettir */
        printf("Ebeveyn: Çocuk devam ediyor...\n");
        fflush(stdout);
        kill(child_pid, SIGCONT);
        alarm(3);
        break;

    case 3:                             /* 9. saniye: tekrar durdur */
        printf("Ebeveyn: Çocuk durduruluyor...\n");
        fflush(stdout);
        kill(child_pid, SIGSTOP);
        alarm(1);
        break;

    case 4:                             /* ~10. saniye: devam ettir + sonlandır */
        printf("Ebeveyn: Çocuk devam ediyor...\n");
        fflush(stdout);
        kill(child_pid, SIGCONT);

        printf("Ebeveyn: SIGINT gönderiliyor...\n");
        fflush(stdout);
        kill(child_pid, SIGINT);

        bitmeli = 1;
        break;
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
    struct sigaction sa;
    sa.sa_handler = parent_alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    alarm(3);

    while (!bitmeli)
        pause();

    int durum;
    waitpid(child_pid, &durum, 0);
    printf("Ebeveyn: Çocuk sonlandırıldı.\n");

    return 0;
}
