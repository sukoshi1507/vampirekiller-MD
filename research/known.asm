; Desassemblage partiel annote, banque physique 00, fenetre Z80 4000-5FFF.
; Format: adresse | octets attendus | instruction | commentaire.
; Ce fichier documente les octets; ce n'est pas encore une source reassemblable.
; Les noms semantiques restent provisoires sauf comportement explicite.

; Hook d'interruption installe par le demarrage.
4028 | F3          | DI                    | Entree du hook.
4029 | 3A 00 E6    | LD A,($E600)          | Indicateur initialise par la routine 5C92.
402C | B7          | OR A                  |
402D | C2 C5 40    | JP NZ,$40C5           | Chemin alternatif; fonction a preciser.
4030 | F3          | DI                    |
4031 | 3E 0E       | LD A,$0E              |
4033 | 32 00 80    | LD ($8000),A          | Banque physique 0E dans la fenetre 8000.
4036 | 3E 0F       | LD A,$0F              |
4038 | 32 00 A0    | LD ($A000),A          | Banque physique 0F dans la fenetre A000.
403B | CD 64 89    | CALL $8964            | Offset physique 1C964 avec banque 0E; role a tracer.
403E | F3          | DI                    |
403F | 3A F2 F0    | LD A,($F0F2)          | Banque sauvegardee de la fenetre 8000.
4042 | 32 00 80    | LD ($8000),A          |
4045 | 3A F3 F0    | LD A,($F0F3)          | Banque sauvegardee de la fenetre A000.
4048 | 32 00 A0    | LD ($A000),A          |
404B | 21 05 C0    | LD HL,$C005           |
404E | CB 46       | BIT 0,(HL)            | Garde contre une execution imbriquee.
4050 | C2 5F 40    | JP NZ,$405F           |
4053 | 34          | INC (HL)              |
4054 | FB          | EI                    |
4055 | CD 9D 4B    | CALL $4B9D            | Lecture d'entrees visible dans les sous-appels.
4058 | CD 4D 41    | CALL $414D            | Dispatch selon l'octet C000.
405B | AF          | XOR A                 |
405C | 32 05 C0    | LD ($C005),A          |
405F | FB          | EI                    |
4060 | C9          | RET                   |

; Aide d'addition, utilisee par la table de dispatch.
4061 | 85          | ADD A,L               |
4062 | 6F          | LD L,A                |
4063 | D0          | RET NC                |
4064 | 24          | INC H                 |
4065 | C9          | RET                   |

; CALL suivi d'une table inline de pointeurs little-endian.
406B | E1          | POP HL                | Recupere l'adresse de retour, debut de table.
406C | 87          | ADD A,A               | Index de deux octets.
406D | CD 61 40    | CALL $4061            | HL += 2*A, avec report.
4070 | 5E          | LD E,(HL)             |
4071 | 23          | INC HL                |
4072 | 56          | LD D,(HL)             |
4073 | EB          | EX DE,HL              |
4074 | E9          | JP (HL)               | Saut indirect: ne pas desassembler la table comme du code.

; Point d'entree de l'en-tete AB.
4075 | F3          | DI                    |
4076 | 31 F0 F0    | LD SP,$F0F0           |
4079 | CD 38 01    | CALL $0138            | BIOS RSLREG: registre des slots primaires.
407C | 0F          | RRCA                  |
407D | 0F          | RRCA                  |
407E | E6 03       | AND $03               | Slot primaire de la page 4000-7FFF.
4080 | 4F          | LD C,A                |
4081 | 06 00       | LD B,$00              |
4083 | 21 C1 FC    | LD HL,$FCC1           | Table des slots etendus.
4086 | 09          | ADD HL,BC             |
4087 | 7E          | LD A,(HL)             |
4088 | E6 80       | AND $80               |
408A | B1          | OR C                  |
408B | 4F          | LD C,A                |
408C | 23          | INC HL                |
408D | 23          | INC HL                |
408E | 23          | INC HL                |
408F | 23          | INC HL                |
4090 | 7E          | LD A,(HL)             |
4091 | E6 0C       | AND $0C               | Bits de sous-slot de la page 4000.
4093 | B1          | OR C                  |
4094 | 26 80       | LD H,$80              |
4096 | CD 24 00    | CALL $0024            | BIOS ENASLT: meme slot en page 8000-BFFF.
4099 | 21 00 C0    | LD HL,$C000           |
409C | 11 01 C0    | LD DE,$C001           |
409F | 01 EF 30    | LD BC,$30EF           |
40A2 | 36 00       | LD (HL),$00           |
40A4 | ED B0       | LDIR                  | Mise a zero C000-F0EF inclus, soit 30F0 octets.
40A6 | CD 36 53    | CALL $5336            | Banques initiales 01,02,03 et miroirs F0F1-F0F3.
40A9 | CD 92 5C    | CALL $5C92            | Parcourt des slots; fonction precise a etablir.
40AC | CD 36 53    | CALL $5336            | Restaure les banques initiales.
40AF | CD 59 4B    | CALL $4B59            | Initialisation video, dont CHGMOD avec A=5.
40B2 | F3          | DI                    |
40B3 | 3E C3       | LD A,$C3              | Opcode JP absolu.
40B5 | 32 9F FD    | LD ($FD9F),A          | Ecrit le hook BIOS.
40B8 | 21 28 40    | LD HL,$4028           |
40BB | 22 A0 FD    | LD ($FDA0),HL         | Hook = JP 4028.
40BE | AF          | XOR A                 |
40BF | 32 DB F3    | LD ($F3DB),A          | Desactive le clic clavier (CLIKSW).
40C2 | FB          | EI                    |
40C3 | 18 FE       | JR $40C3              | Attente active; la progression passe par le hook.

; Debut du dispatch. Quatorze pointeurs occupent 4161-417C.
414D | 21 03 C0    | LD HL,$C003           |
4150 | 34          | INC (HL)              | Compteur 8 bits.
4151 | ED 4B 00 C0 | LD BC,($C000)         | C=C000; B=C001.
4155 | 79          | LD A,C                |
4156 | FE 03       | CP $03                |
4158 | 30 04       | JR NC,$415E           |
415A | 21 91 43    | LD HL,$4391           |
415D | E5          | PUSH HL               | Retour special pour les etats 0,1,2.
415E | CD 6B 40    | CALL $406B            | Table inline immediatement apres CALL.
4161 | 7D 41 A0 41 AB 41 D1 41 2C 42 57 42 94 42 B2 42 24 43 77 43 87 43 DA 43 F0 43 14 44 | DW $417D,$41A0,$41AB,$41D1,$422C,$4257,$4294,$42B2,$4324,$4377,$4387,$43DA,$43F0,$4414 | 14 cibles visibles; plages d'index a confirmer dynamiquement.

; Extrait video: A=5 avant l'appel CHGMOD.
4B59 | CD 76 50    | CALL $5076            |
4B5C | CD FE 47    | CALL $47FE            |
4B5F | 3E 05       | LD A,$05              |
4B61 | CD 5F 00    | CALL $005F            | BIOS CHGMOD: selection du mode 5.

; Routine de mise en place des banques initiales.
5336 | F3          | DI                    |
5337 | E5          | PUSH HL               |
5338 | 21 F1 F0    | LD HL,$F0F1           |
533B | 3E 01       | LD A,$01              |
533D | 32 00 60    | LD ($6000),A          | Fenetre 6000-7FFF <- banque 01.
5340 | 77          | LD (HL),A             |
5341 | 3C          | INC A                 |
5342 | 32 00 80    | LD ($8000),A          | Fenetre 8000-9FFF <- banque 02.
5345 | 23          | INC HL                |
5346 | 77          | LD (HL),A             |
5347 | 3C          | INC A                 |
5348 | 32 00 A0    | LD ($A000),A          | Fenetre A000-BFFF <- banque 03.
534B | 23          | INC HL                |
534C | 77          | LD (HL),A             |
534D | E1          | POP HL                |
534E | FB          | EI                    |
534F | C9          | RET                   |
