/*
 * Soru 2: fork() + sinyal yönetimi
 *
 * Senaryo:
 *   - Ebeveyn, bir alt süreç (child) oluşturur.
 *   - Child: sonsuz döngüde "Calisiyor..." yazdırır.
 *   - Ebeveyn: alarm(3) ile her 3 saniyede bir SIGALRM alır.
 *     1. SIGALRM → child'a SIGSTOP  (durdur)
 *     2. SIGALRM → child'a SIGCONT  (devam ettir)
 *     3. SIGALRM → child'a SIGSTOP  (tekrar durdur)
 *   - ~10 saniye sonra ebeveyn child'a SIGCONT + SIGINT gönderir ve bekler.
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
static int   durduruldu  = 0;
static int   alarm_sayaci = 0;

static void parent_alarm_handler(int sig)
{
    (void)sig;
    alarm_sayaci++;

    if (!durduruldu) {
        printf("[Parent] SIGALRM #%d -> child'a SIGSTOP\n", alarm_sayaci);
        fflush(stdout);
        kill(child_pid, SIGSTOP);
        durduruldu = 1;
    } else {
        printf("[Parent] SIGALRM #%d -> child'a SIGCONT\n", alarm_sayaci);
        fflush(stdout);
        kill(child_pid, SIGCONT);
        durduruldu = 0;
    }

    /* 10 saniye içinde toplam 3 alarm: saniye 3, 6, 9 */
    if (alarm_sayaci < 3)
        alarm(3);
}

int main(void)
{
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        run_child();
    }

    /* ---- EBEVEYN ---- */
    printf("[Parent %d] Child PID = %d\n", getpid(), child_pid);
    fflush(stdout);

    /* sigaction ile kalıcı handler kur (signal() _POSIX modunda handler'ı sıfırlar) */
    struct sigaction sa;
    sa.sa_handler = parent_alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   /* SA_RESTART YOK: sleep() sinyal gelince kesilsin */
    sigaction(SIGALRM, &sa, NULL);

    alarm(3);   /* ilk zamanlayıcı */

    /* sleep() sinyal gelince kesilir ve kalan süreyi döndürür;
     * döngü ile toplam 10 saniye uyku sağlanır */
    unsigned int kalan = 10;
    while (kalan > 0)
        kalan = sleep(kalan);

    alarm(0);   /* varsa bekleyen alarmı iptal et */

    printf("[Parent] ~10 saniye doldu, child'a SIGINT gonderiliyor.\n");
    fflush(stdout);

    /* Durdurulmuşsa önce uyandır, sonra sonlandır */
    if (durduruldu)
        kill(child_pid, SIGCONT);
    kill(child_pid, SIGINT);

    int durum;
    waitpid(child_pid, &durum, 0);
    printf("[Parent] Child sonlandi. Program bitiyor.\n");

    return 0;
}
