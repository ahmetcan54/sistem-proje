/*
 * Soru 2: fork() + sinyal yönetimi
 *
 * Senaryo:
 *   - Ebeveyn bir alt süreç (child) oluşturur.
 *   - Child: sonsuz döngüde "Calisiyor..." yazdırır.
 *   - Ebeveyn: alarm(3) ile 3 saniyede bir SIGALRM alır.
 *       SIGALRM #1 (3. sn)  → child'a SIGSTOP
 *       SIGALRM #2 (6. sn)  → child'a SIGCONT
 *       SIGALRM #3 (9. sn)  → child'a SIGSTOP
 *       SIGALRM #4 (10. sn) → child'a SIGCONT + SIGINT  (~10 saniye)
 *   - Ebeveyn waitpid() ile child'ın bitmesini bekler.
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
    printf("[Child %d] SIGINT alindi, cikis yapiliyor.\n", getpid());
    fflush(stdout);
    exit(0);
}

static void child_sigcont_handler(int sig)
{
    (void)sig;
    printf("[Child %d] SIGCONT alindi, devam ediyorum.\n", getpid());
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
        printf("[Child %d] Calisiyor... (%d)\n", getpid(), ++sayac);
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
    case 1:                         /* 3. saniye: durdur */
        printf("[Parent] SIGALRM #1 -> child'a SIGSTOP\n");
        fflush(stdout);
        kill(child_pid, SIGSTOP);
        alarm(3);
        break;

    case 2:                         /* 6. saniye: devam ettir */
        printf("[Parent] SIGALRM #2 -> child'a SIGCONT\n");
        fflush(stdout);
        kill(child_pid, SIGCONT);
        alarm(3);
        break;

    case 3:                         /* 9. saniye: tekrar durdur */
        printf("[Parent] SIGALRM #3 -> child'a SIGSTOP\n");
        fflush(stdout);
        kill(child_pid, SIGSTOP);
        alarm(1);                   /* 1 saniye sonra SIGINT gönder */
        break;

    case 4:                         /* ~10. saniye: sonlandır */
        printf("[Parent] ~10 saniye doldu, child'a SIGINT gonderiliyor.\n");
        fflush(stdout);
        kill(child_pid, SIGCONT);   /* duruyorsa uyandır */
        kill(child_pid, SIGINT);
        bitmeli = 1;                /* ana döngüyü bitir */
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
    printf("[Parent %d] Child PID = %d\n", getpid(), child_pid);
    fflush(stdout);

    struct sigaction sa;
    sa.sa_handler = parent_alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    alarm(3);   /* ilk zamanlayıcı */

    /* SIGINT, bitmeli bayrağı 1 olana kadar bekle */
    while (!bitmeli)
        pause();

    int durum;
    waitpid(child_pid, &durum, 0);
    printf("[Parent] Child sonlandi. Program bitiyor.\n");

    return 0;
}
