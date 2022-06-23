# 💧 2. Úkol na IOS - VUT

Hodnocení: 15/15

### Výstup z hodnotících testů
```
15:celkem bodu za projekt
#-- automaticke hodnoceni -----------------------------
= make
:ok:make
= prepare tests: resources
:kontrola syntaxe vystupu => check_syntax.out
= base_* : zakladni testy
:ok:test_a_base_counter: navratovy kod je 0
1:ok:test_a_base_counter
1:ok:test_b_base_O: posloupnost O ok
1:ok:test_c_base_H: posloupnost H ok
1:ok:test_d_base_molecule: posloupnost molecule ok
1:ok:test_e_not_enough:zpracovani not enough
:ok:test_g_counter: navratovy kod je 0
1:ok:test_g_counter
1:ok:test_h_molecule: posloupnost molekul ok
1:ok:test_i_O: posloupnost O
1:ok:test_j_H: posloupnost H
2:ok:test_k_sync: synchronizace poradi molekul + not enough
2:ok:test_n_sync_sleep: synchronizace poradi molekul + not enough (castejsi prepinani procesu)
2:ok:test_o_sync_nosleep: synchronizace poradi molekul + not enough (sleep -> 0ms)
= test spravneho ukonceni pri chybe
1:ok:test_q_error_1: osetreni chybneho vstupu
= resources
: pocet procesu ok (7, mel by byt 7)
: pocet volani wait (waitpid) ok
:ok: pripojeni ke sdilene pameti a uvolneni je korektni
: pocet volani shmat a shmdt se lisi
:ok: korektni uvolneni nepojmenovane semafory
#------------------------------------------------------
16:celkove score (max pro hodnoceni 15)
15:celkem bodu za projekt
```
