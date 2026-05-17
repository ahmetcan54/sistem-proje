/*
 * Soru 2: fork() + sinyal yönetimi
 *
 * Senaryo:
 *   - Ebeveyn, bir alt süreç (child) oluşturur.
 *   - Child: sonsuz döngüde "Çalışıyorum..." yazdırır.
 *   - Ebeveyn: alarm(3) ile her 3 saniyede bir SIGALRM alır.
 *     İlk  SIGALRM → child'a SIGSTOP  (durdur)
 *     İkinci SIGALRM → child'a SIGCONT (devam ettir) + yeni alarm(3)
 *     (bu iki adım dönüşümlü tekrarlanır)
 *   - ~10 saniye sonra ebeveyn child'a SIGINT gönderir ve bekler.
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

/* SIGINT geldiğinde child temiz çıkış yapar */
static void child_sigint_handler(int sig)
{
    (void)sig;
    printf("[Child %d] SIGINT alindi, cikis yapiliyor.\n", getpid());
    fflush(stdout);
    exit(0);
}

/* SIGSTOP/SIGCONT kernel tarafından işlendiği için ayrıca handler gerekmez;
   ancak child'ın duraksatıldığını/devam ettiğini log'lamak için SIGCONT
   yakalanabilir. */
static void child_sigcont_handler(int sig)
{
    (void)sig;
    printf("[Child %d] SIGCONT alindi, devam ediyorum.\n", getpid());
    fflush(stdout);
}

static void run_child(void)
{
    /* Child için sinyal işleyicilerini kur */
    signal(SIGINT,  child_sigint_handler);
    signal(SIGCONT, child_sigcont_handler);

    int sayac = 0;
    while (1) {
        printf("[Child %d] Calisiyor... (%d)\n", getpid(), ++sayac);
        fflush(stdout);
        sleep(1);   /* Her saniye bir satır */
    }
}

/* ------------------------------------------------------------------ */
/*  Parent tarafı                                                       */
/* ------------------------------------------------------------------ */

static pid_t child_pid;         /* global: sinyal işleyicisi kullanacak */
static int   durduruldu = 0;    /* 0 = çalışıyor, 1 = SIGSTOP gönderildi */
static int   alarm_sayaci = 0;  /* kaç SIGALRM geldi */

static void parent_alarm_handler(int sig)
{
    (void)sig;
    alarm_sayaci++;

    if (!durduruldu) {
        /* Child çalışıyorsa durdur */
        printf("[Parent] SIGALRM #%d → child'a SIGSTOP\n", alarm_sayaci);
        fflush(stdout);
        kill(child_pid, SIGSTOP);
        durduruldu = 1;
    } else {
        /* Child duruyorsa devam ettir */
        printf("[Parent] SIGALRM #%d → child'a SIGCONT\n", alarm_sayaci);
        fflush(stdout);
        kill(child_pid, SIGCONT);
        durduruldu = 0;
    }

    /* Bir sonraki SIGALRM'ı planla (10 saniyeye kadar) */
    if (alarm_sayaci < 3) {     /* 3*3 = 9 s < 10 s → 4. alarm'dan önce SIGINT */
        alarm(3);
    }
}

int main(void)
{
    child_pid = fork();

    if (child_pid < 0) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        /* ---- ALT SÜREÇ ---- */
        run_child();
        /* run_child() içinde exit() çağrıldığı için buraya ulaşılmaz */
    }

    /* ---- EBEVEYN SÜREÇ ---- */
    printf("[Parent %d] Child PID = %d\n", getpid(), child_pid);
    fflush(stdout);

    /* SIGALRM işleyicisini kur */
    signal(SIGALRM, parent_alarm_handler);

    /* İlk zamanlayıcıyı başlat: 3 saniye sonra ilk SIGALRM */
    alarm(3);

    /* ~10 saniye bekle (sinyal kesintilerine karşı dayanıklı döngü).
     * sleep() bir sinyal alınca kalan süreyi döndürür; sıfıra düşene kadar
     * yeniden uyku yapılır, böylece SIGALRM işleyicisi tam çalışır. */
    unsigned int kalan = 10;
    while (kalan > 0)
        kalan = sleep(kalan);

    /* Bekleyen alarmı iptal et */
    alarm(0);

    printf("[Parent] ~10 saniye doldu, child'a SIGINT gonderiliyor.\n");
    fflush(stdout);
    /* Durdurulmuşsa önce uyandır ki SIGINT işlenebilsin */
    if (durduruldu)
        kill(child_pid, SIGCONT);
    kill(child_pid, SIGINT);

    /* Child'ın bitmesini bekle, zombie oluşmasını engelle */
    int durum;
    waitpid(child_pid, &durum, 0);
    printf("[Parent] Child sonlandi. Program bitiyor.\n");

    return 0;
}
