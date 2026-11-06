; Regions selectionnees. z80dis 1.0.6.
; Mapping du listing: 00/01/02/03 dans 4000/6000/8000/A000.
; Les appels doivent etre interpretes selon le mapping actif.
; Adresse CPU | octets | instruction. Noms semantiques non generes.

; Region 417D-42FF
417D | 10 0B       | DJNZ 0x418a
417F | CD 4C 62    | CALL 0x624c
4182 | 3A 22 C4    | LD A,(0xc422)
4185 | B7          | OR A
4186 | C8          | RET z
4187 | AF          | XOR A
4188 | 18 3F       | JR 0x41c9
418A | 10 0C       | DJNZ 0x4198
418C | 21 04 C0    | LD HL,0xc004
418F | 35          | DEC (HL)
4190 | C0          | RET nz
4191 | CD 47 4D    | CALL 0x4d47
4194 | AF          | XOR A
4195 | C3 4B 42    | JP 0x424b
4198 | CD B9 47    | CALL 0x47b9
419B | CD 02 62    | CALL 0x6202
419E | 18 2C       | JR 0x41cc
41A0 | 21 04 C0    | LD HL,0xc004
41A3 | 35          | DEC (HL)
41A4 | C0          | RET nz
41A5 | CD E7 7A    | CALL 0x7ae7
41A8 | C3 49 42    | JP 0x4249
41AB | 10 14       | DJNZ 0x41c1
41AD | CD 20 4E    | CALL 0x4e20
41B0 | 3A 13 C4    | LD A,(0xc413)
41B3 | B7          | OR A
41B4 | C0          | RET nz
41B5 | AF          | XOR A
41B6 | 32 00 C0    | LD (0xc000),A
41B9 | 3E 20       | LD A,0x20
41BB | 32 04 C0    | LD (0xc004),A
41BE | C3 52 42    | JP 0x4252
41C1 | CD B9 47    | CALL 0x47b9
41C4 | CD E7 4D    | CALL 0x4de7
41C7 | 3E 20       | LD A,0x20
41C9 | 32 04 C0    | LD (0xc004),A
41CC | 21 01 C0    | LD HL,0xc001
41CF | 34          | INC (HL)
41D0 | C9          | RET
41D1 | 10 11       | DJNZ 0x41e4
41D3 | 21 04 C0    | LD HL,0xc004
41D6 | 35          | DEC (HL)
41D7 | 28 F3       | JR z,0x41cc
41D9 | CB 56       | BIT 2,(HL)
41DB | 21 29 4D    | LD HL,0x4d29
41DE | CA CB 4A    | JP z,0x4acb
41E1 | C3 CF 4A    | JP 0x4acf
41E4 | 10 06       | DJNZ 0x41ec
41E6 | CD C6 44    | CALL 0x44c6
41E9 | C3 CC 41    | JP 0x41cc
41EC | 10 0D       | DJNZ 0x41fb
41EE | 3E 01       | LD A,1
41F0 | 32 1A C4    | LD (0xc41a),A
41F3 | CD D3 63    | CALL 0x63d3
41F6 | 3E A0       | LD A,0xa0
41F8 | C3 C9 41    | JP 0x41c9
41FB | 10 25       | DJNZ 0x4222
41FD | CD 93 4E    | CALL 0x4e93
4200 | 21 04 C0    | LD HL,0xc004
4203 | 3A 03 C0    | LD A,(0xc003)
4206 | 1F          | RRA
4207 | D8          | RET c
4208 | 35          | DEC (HL)
4209 | C0          | RET nz
420A | CD F0 47    | CALL 0x47f0
420D | AF          | XOR A
420E | 32 1A C4    | LD (0xc41a),A
4211 | CD D0 62    | CALL 0x62d0
4214 | 21 04 E6    | LD HL,0xe604
4217 | 7E          | LD A,(HL)
4218 | B7          | OR A
4219 | 28 05       | JR z,0x4220
421B | 36 00       | LD (HL),0
421D | CD 31 5E    | CALL 0x5e31
4220 | 18 27       | JR 0x4249
4222 | 3E 8A       | LD A,0x8a
4224 | CD 9F 50    | CALL 0x509f
4227 | 3E 50       | LD A,0x50
4229 | C3 C9 41    | JP 0x41c9
422C | 21 10 C4    | LD HL,0xc410
422F | 7E          | LD A,(HL)
4230 | D6 01       | SUB 1
4232 | 27          | DAA
4233 | 77          | LD (HL),A
4234 | CD D4 47    | CALL 0x47d4
4237 | CD 13 45    | CALL 0x4513
423A | CD E6 62    | CALL 0x62e6
423D | 21 13 C4    | LD HL,0xc413
4240 | 36 01       | LD (HL),1
4242 | CD 4F 79    | CALL 0x794f
4245 | AF          | XOR A
4246 | 32 0D C4    | LD (0xc40d),A
4249 | 3E 20       | LD A,0x20
424B | 32 04 C0    | LD (0xc004),A
424E | 21 00 C0    | LD HL,0xc000
4251 | 34          | INC (HL)
4252 | AF          | XOR A
4253 | 32 01 C0    | LD (0xc001),A
4256 | C9          | RET
4257 | CD 25 5C    | CALL 0x5c25
425A | 3A 0C C4    | LD A,(0xc40c)
425D | B7          | OR A
425E | 3E 0C       | LD A,12
4260 | C2 B6 41    | JP nz,0x41b6
4263 | 3A 0A C4    | LD A,(0xc40a)
4266 | A7          | AND A
4267 | C2 8C 42    | JP nz,0x428c
426A | 3A 1B C4    | LD A,(0xc41b)
426D | A7          | AND A
426E | 3E 09       | LD A,9
4270 | C2 B6 41    | JP nz,0x41b6
4273 | 3A 08 C4    | LD A,(0xc408)
4276 | B7          | OR A
4277 | 3E 0A       | LD A,10
4279 | C2 B6 41    | JP nz,0x41b6
427C | 3A 09 C4    | LD A,(0xc409)
427F | A7          | AND A
4280 | 3E 08       | LD A,8
4282 | C2 B6 41    | JP nz,0x41b6
4285 | 3A 13 C4    | LD A,(0xc413)
4288 | B7          | OR A
4289 | C0          | RET nz
428A | 18 BD       | JR 0x4249
428C | CD 95 44    | CALL 0x4495
428F | 3E 0B       | LD A,11
4291 | C3 B6 41    | JP 0x41b6
4294 | 21 10 C4    | LD HL,0xc410
4297 | 7E          | LD A,(HL)
4298 | B7          | OR A
4299 | 28 10       | JR z,0x42ab
429B | CD D0 62    | CALL 0x62d0
429E | AF          | XOR A
429F | 32 20 C4    | LD (0xc420),A
42A2 | 3C          | INC A
42A3 | 32 0D C4    | LD (0xc40d),A
42A6 | 3E 04       | LD A,4
42A8 | C3 B6 41    | JP 0x41b6
42AB | 3E 8B       | LD A,0x8b
42AD | CD 9F 50    | CALL 0x509f
42B0 | 18 97       | JR 0x4249
42B2 | 10 2F       | DJNZ 0x42e3
42B4 | 3A 00 E6    | LD A,(0xe600)
42B7 | B7          | OR A
42B8 | 28 05       | JR z,0x42bf
42BA | CD 14 43    | CALL 0x4314
42BD | 20 14       | JR nz,0x42d3
42BF | 3A A7 C0    | LD A,(0xc0a7)
42C2 | A7          | AND A
42C3 | C0          | RET nz
42C4 | 21 04 C0    | LD HL,0xc004
42C7 | 35          | DEC (HL)
42C8 | C0          | RET nz
42C9 | 21 02 C0    | LD HL,0xc002
42CC | 7E          | LD A,(HL)
42CD | E6 BF       | AND 0xbf
42CF | 77          | LD (HL),A
42D0 | C3 B5 41    | JP 0x41b5
42D3 | 3E 03       | LD A,3
42D5 | 32 10 C4    | LD (0xc410),A
42D8 | AF          | XOR A
42D9 | 67          | LD H,A
42DA | 6F          | LD L,A
42DB | 22 05 C4    | LD (0xc405),HL
42DE | 32 07 C4    | LD (0xc407),A
42E1 | 18 B8       | JR 0x429b
42E3 | CD B9 47    | CALL 0x47b9
42E6 | 21 3A 4D    | LD HL,0x4d3a
42E9 | CD CB 4A    | CALL 0x4acb
42EC | 3A 00 E6    | LD A,(0xe600)
42EF | B7          | OR A
42F0 | 28 06       | JR z,0x42f8
42F2 | 21 00 43    | LD HL,0x4300
42F5 | CD CB 4A    | CALL 0x4acb
42F8 | CD 13 45    | CALL 0x4513
42FB | 3E 78       | LD A,0x78
42FD | C3 C9 41    | JP 0x41c9

; Region 4314-44E8
4314 | 3E 07       | LD A,7
4316 | CD 41 01    | CALL 0x0141
4319 | 2F          | CPL
431A | E6 02       | AND 2
431C | 21 14 E6    | LD HL,0xe614
431F | 4E          | LD C,(HL)
4320 | 77          | LD (HL),A
4321 | A9          | XOR C
4322 | A6          | AND (HL)
4323 | C9          | RET
4324 | 10 4A       | DJNZ 0x4370
4326 | 21 02 D0    | LD HL,0xd002
4329 | 34          | INC (HL)
432A | 3E 06       | LD A,6
432C | 96          | SUB (HL)
432D | 20 15       | JR nz,0x4344
432F | 77          | LD (HL),A
4330 | 3E FF       | LD A,0xff
4332 | 32 00 D0    | LD (0xd000),A
4335 | 3E 20       | LD A,0x20
4337 | 32 15 C4    | LD (0xc415),A
433A | 21 34 CF    | LD HL,0xcf34
433D | 34          | INC (HL)
433E | 7E          | LD A,(HL)
433F | FE 02       | CP 2
4341 | CA B5 41    | JP z,0x41b5
4344 | CD D5 62    | CALL 0x62d5
4347 | 21 10 C4    | LD HL,0xc410
434A | 7E          | LD A,(HL)
434B | C6 01       | ADD A,1
434D | 27          | DAA
434E | 77          | LD (HL),A
434F | 21 11 C4    | LD HL,0xc411
4352 | 7E          | LD A,(HL)
4353 | C6 01       | ADD A,1
4355 | 27          | DAA
4356 | 77          | LD (HL),A
4357 | 21 00 D0    | LD HL,0xd000
435A | 34          | INC (HL)
435B | 23          | INC HL
435C | AF          | XOR A
435D | 77          | LD (HL),A
435E | 32 09 C4    | LD (0xc409),A
4361 | 32 08 C4    | LD (0xc408),A
4364 | 3E 04       | LD A,4
4366 | C3 B6 41    | JP 0x41b6
4369 | 21 00 00    | LD HL,0
436C | 22 00 C0    | LD (0xc000),HL
436F | C9          | RET
4370 | CD B9 47    | CALL 0x47b9
4373 | AF          | XOR A
4374 | C3 C9 41    | JP 0x41c9
4377 | CD 2E 5A    | CALL 0x5a2e
437A | 3E 06       | LD A,6
437C | D2 B6 41    | JP nc,0x41b6
437F | CD F5 62    | CALL 0x62f5
4382 | 3E 05       | LD A,5
4384 | C3 B6 41    | JP 0x41b6
4387 | 21 01 C7    | LD HL,0xc701
438A | 7E          | LD A,(HL)
438B | E6 FE       | AND 0xfe
438D | 77          | LD (HL),A
438E | C3 47 43    | JP 0x4347
4391 | CD BB 4B    | CALL 0x4bbb
4394 | 21 01 C4    | LD HL,0xc401
4397 | CD B4 4B    | CALL 0x4bb4
439A | B7          | OR A
439B | C8          | RET z
439C | 21 04 C0    | LD HL,0xc004
439F | 36 00       | LD (HL),0
43A1 | 21 00 C0    | LD HL,0xc000
43A4 | 46          | LD B,(HL)
43A5 | 10 13       | DJNZ 0x43ba
43A7 | E6 30       | AND 0x30
43A9 | C8          | RET z
43AA | 3E 40       | LD A,0x40
43AC | 32 02 C0    | LD (0xc002),A
43AF | 3A 00 E6    | LD A,(0xe600)
43B2 | B7          | OR A
43B3 | 20 0F       | JR nz,0x43c4
43B5 | 36 03       | LD (HL),3
43B7 | 23          | INC HL
43B8 | 70          | LD (HL),B
43B9 | C9          | RET
43BA | 36 01       | LD (HL),1
43BC | 3E 00       | LD A,0
43BE | CD 9F 50    | CALL 0x509f
43C1 | C3 47 4D    | JP 0x4d47
43C4 | AF          | XOR A
43C5 | 32 04 E6    | LD (0xe604),A
43C8 | 3E 01       | LD A,1
43CA | 32 05 E6    | LD (0xe605),A
43CD | 32 06 E6    | LD (0xe606),A
43D0 | 3E 03       | LD A,3
43D2 | 32 07 E6    | LD (0xe607),A
43D5 | 3E 0D       | LD A,13
43D7 | C3 B6 41    | JP 0x41b6
43DA | 3A 0B C0    | LD A,(0xc00b)
43DD | 1F          | RRA
43DE | D0          | RET nc
43DF | AF          | XOR A
43E0 | 32 0A C4    | LD (0xc40a),A
43E3 | CD B8 44    | CALL 0x44b8
43E6 | 3E FE       | LD A,0xfe
43E8 | CD 9F 50    | CALL 0x509f
43EB | 3E 05       | LD A,5
43ED | C3 B6 41    | JP 0x41b6
43F0 | 10 09       | DJNZ 0x43fb
43F2 | CD BA 94    | CALL 0x94ba
43F5 | C0          | RET nz
43F6 | 3E 0F       | LD A,15
43F8 | C3 C9 41    | JP 0x41c9
43FB | 10 0D       | DJNZ 0x440a
43FD | 21 04 C0    | LD HL,0xc004
4400 | 35          | DEC (HL)
4401 | C0          | RET nz
4402 | CD 07 95    | CALL 0x9507
4405 | 3E 05       | LD A,5
4407 | C3 B6 41    | JP 0x41b6
440A | AF          | XOR A
440B | 32 0C C4    | LD (0xc40c),A
440E | CD 87 93    | CALL 0x9387
4411 | C3 CC 41    | JP 0x41cc
4414 | 10 36       | DJNZ 0x444c
4416 | 3A 06 C0    | LD A,(0xc006)
4419 | E6 33       | AND 0x33
441B | C8          | RET z
441C | E6 03       | AND 3
441E | C2 7D 5E    | JP nz,0x5e7d
4421 | 3A 0B E6    | LD A,(0xe60b)
4424 | B7          | OR A
4425 | C2 32 44    | JP nz,0x4432
4428 | CD FD 5C    | CALL 0x5cfd
442B | 21 03 00    | LD HL,3
442E | 22 00 C0    | LD (0xc000),HL
4431 | C9          | RET
4432 | 3D          | DEC A
4433 | 28 0B       | JR z,0x4440
4435 | 21 B8 B8    | LD HL,0xb8b8
4438 | 22 02 E6    | LD (0xe602),HL
443B | CD 82 5D    | CALL 0x5d82
443E | 18 09       | JR 0x4449
4440 | 21 B8 B0    | LD HL,0xb0b8
4443 | 22 02 E6    | LD (0xe602),HL
4446 | CD 61 5D    | CALL 0x5d61
4449 | C3 CC 41    | JP 0x41cc
444C | 10 33       | DJNZ 0x4481
444E | CD 21 5E    | CALL 0x5e21
4451 | 20 03       | JR nz,0x4456
4453 | C3 B2 5D    | JP 0x5db2
4456 | 3A 0B E6    | LD A,(0xe60b)
4459 | 47          | LD B,A
445A | 21 04 E6    | LD HL,0xe604
445D | B6          | OR (HL)
445E | 77          | LD (HL),A
445F | 3A 15 E6    | LD A,(0xe615)
4462 | B7          | OR A
4463 | 28 12       | JR z,0x4477
4465 | 3A 0E E6    | LD A,(0xe60e)
4468 | 57          | LD D,A
4469 | 3A 0F E6    | LD A,(0xe60f)
446C | CB 40       | BIT 0,B
446E | 28 0C       | JR z,0x447c
4470 | 32 05 E6    | LD (0xe605),A
4473 | 7A          | LD A,D
4474 | 32 06 E6    | LD (0xe606),A
4477 | AF          | XOR A
4478 | 32 01 C0    | LD (0xc001),A
447B | C9          | RET
447C | 32 07 E6    | LD (0xe607),A
447F | 18 F6       | JR 0x4477
4481 | CD EF 5C    | CALL 0x5cef
4484 | AF          | XOR A
4485 | 21 08 E6    | LD HL,0xe608
4488 | 11 09 E6    | LD DE,0xe609
448B | 01 0E 00    | LD BC,14
448E | 36 00       | LD (HL),0
4490 | ED B0       | LDIR
4492 | C3 CC 41    | JP 0x41cc
4495 | 21 54 68    | LD HL,0x6854
4498 | 01 10 30    | LD BC,0x3010
449B | 11 70 D0    | LD DE,0xd070
449E | 3E 04       | LD A,4
44A0 | E5          | PUSH HL
44A1 | C5          | PUSH BC
44A2 | CD 46 49    | CALL 0x4946
44A5 | C1          | POP BC
44A6 | E1          | POP HL
44A7 | CD 0E 5D    | CALL 0x5d0e
44AA | 21 B0 44    | LD HL,0x44b0
44AD | C3 CB 4A    | JP 0x4acb
44B0 | 6C          | LD L,H
44B1 | 58          | LD E,B
44B2 | 40          | LD B,B
44B3 | 31 45 43    | LD SP,0x4345
44B6 | 35          | DEC (HL)
44B7 | FF          | RST 0x38
44B8 | 11 54 68    | LD DE,0x6854
44BB | 01 10 30    | LD BC,0x3010
44BE | 21 70 D0    | LD HL,0xd070
44C1 | 3E 01       | LD A,1
44C3 | C3 46 49    | JP 0x4946
44C6 | 21 05 C4    | LD HL,0xc405
44C9 | 01 FB 1B    | LD BC,0x1bfb
44CC | 54          | LD D,H
44CD | 5D          | LD E,L
44CE | 1C          | INC E
44CF | 36 00       | LD (HL),0
44D1 | ED B0       | LDIR
44D3 | 21 E9 44    | LD HL,0x44e9
44D6 | 11 10 C4    | LD DE,0xc410
44D9 | 01 03 00    | LD BC,3
44DC | ED B0       | LDIR
44DE | 3E 20       | LD A,0x20
44E0 | 32 15 C4    | LD (0xc415),A
44E3 | 3E 80       | LD A,0x80
44E5 | 32 18 C4    | LD (0xc418),A
44E8 | C9          | RET

; Region 483E-484B
483E | 7E          | LD A,(HL)
483F | 23          | INC HL
4840 | 3C          | INC A
4841 | C8          | RET z
4842 | 3D          | DEC A
4843 | 56          | LD D,(HL)
4844 | 23          | INC HL
4845 | 5E          | LD E,(HL)
4846 | 23          | INC HL
4847 | CD 14 48    | CALL 0x4814
484A | 18 F2       | JR 0x483e

; Region 4A27-4ACA
4A27 | CD 30 4A    | CALL 0x4a30
4A2A | CD 4F 4B    | CALL 0x4b4f
4A2D | 10 F8       | DJNZ 0x4a27
4A2F | C9          | RET
4A30 | C5          | PUSH BC
4A31 | D5          | PUSH DE
4A32 | E5          | PUSH HL
4A33 | D5          | PUSH DE
4A34 | CD A5 4A    | CALL 0x4aa5
4A37 | D1          | POP DE
4A38 | 42          | LD B,D
4A39 | 53          | LD D,E
4A3A | 58          | LD E,B
4A3B | CB 3A       | SRL D
4A3D | CB 1B       | RR E
4A3F | 7A          | LD A,D
4A40 | C6 80       | ADD A,0x80
4A42 | 57          | LD D,A
4A43 | 21 10 C1    | LD HL,0xc110
4A46 | CD 51 4A    | CALL 0x4a51
4A49 | E1          | POP HL
4A4A | 01 08 00    | LD BC,8
4A4D | 09          | ADD HL,BC
4A4E | D1          | POP DE
4A4F | C1          | POP BC
4A50 | C9          | RET
4A51 | D5          | PUSH DE
4A52 | 06 08       | LD B,8
4A54 | C5          | PUSH BC
4A55 | 01 04 00    | LD BC,4
4A58 | CD 75 46    | CALL 0x4675
4A5B | EB          | EX DE,HL
4A5C | 01 80 00    | LD BC,0x80
4A5F | 09          | ADD HL,BC
4A60 | EB          | EX DE,HL
4A61 | C1          | POP BC
4A62 | 10 F0       | DJNZ 0x4a54
4A64 | D1          | POP DE
4A65 | C9          | RET
4A66 | C5          | PUSH BC
4A67 | CD 51 4A    | CALL 0x4a51
4A6A | 3E 04       | LD A,4
4A6C | 83          | ADD A,E
4A6D | FE 80       | CP 0x80
4A6F | 20 05       | JR nz,0x4a76
4A71 | 3E 04       | LD A,4
4A73 | 82          | ADD A,D
4A74 | 57          | LD D,A
4A75 | AF          | XOR A
4A76 | 5F          | LD E,A
4A77 | C1          | POP BC
4A78 | 10 EC       | DJNZ 0x4a66
4A7A | C9          | RET
4A7B | D5          | PUSH DE
4A7C | 06 10       | LD B,0x10
4A7E | C5          | PUSH BC
4A7F | 01 08 00    | LD BC,8
4A82 | CD 75 46    | CALL 0x4675
4A85 | EB          | EX DE,HL
4A86 | 01 80 00    | LD BC,0x80
4A89 | 09          | ADD HL,BC
4A8A | EB          | EX DE,HL
4A8B | C1          | POP BC
4A8C | 10 F0       | DJNZ 0x4a7e
4A8E | D1          | POP DE
4A8F | C9          | RET
4A90 | C5          | PUSH BC
4A91 | CD 7B 4A    | CALL 0x4a7b
4A94 | 3E 08       | LD A,8
4A96 | 83          | ADD A,E
4A97 | FE 80       | CP 0x80
4A99 | 20 05       | JR nz,0x4aa0
4A9B | 3E 08       | LD A,8
4A9D | 82          | ADD A,D
4A9E | 57          | LD D,A
4A9F | AF          | XOR A
4AA0 | 5F          | LD E,A
4AA1 | C1          | POP BC
4AA2 | 10 EC       | DJNZ 0x4a90
4AA4 | C9          | RET
4AA5 | 06 08       | LD B,8
4AA7 | 11 10 C1    | LD DE,0xc110
4AAA | C5          | PUSH BC
4AAB | E5          | PUSH HL
4AAC | EB          | EX DE,HL
4AAD | 1A          | LD A,(DE)
4AAE | 57          | LD D,A
4AAF | 06 04       | LD B,4
4AB1 | 79          | LD A,C
4AB2 | CB 12       | RL D
4AB4 | 38 01       | JR c,0x4ab7
4AB6 | AF          | XOR A
4AB7 | ED 6F       | RLD
4AB9 | 79          | LD A,C
4ABA | CB 12       | RL D
4ABC | 38 01       | JR c,0x4abf
4ABE | AF          | XOR A
4ABF | ED 6F       | RLD
4AC1 | 23          | INC HL
4AC2 | 10 ED       | DJNZ 0x4ab1
4AC4 | EB          | EX DE,HL
4AC5 | E1          | POP HL
4AC6 | 23          | INC HL
4AC7 | C1          | POP BC
4AC8 | 10 E0       | DJNZ 0x4aaa
4ACA | C9          | RET

; Region 4B41-4B58
4B41 | 47          | LD B,A
4B42 | E6 1F       | AND 0x1f
4B44 | 87          | ADD A,A
4B45 | 87          | ADD A,A
4B46 | 87          | ADD A,A
4B47 | 67          | LD H,A
4B48 | 78          | LD A,B
4B49 | E6 E0       | AND 0xe0
4B4B | 0F          | RRCA
4B4C | 0F          | RRCA
4B4D | 6F          | LD L,A
4B4E | C9          | RET
4B4F | 7A          | LD A,D
4B50 | C6 08       | ADD A,8
4B52 | 57          | LD D,A
4B53 | C0          | RET nz
4B54 | 7B          | LD A,E
4B55 | C6 08       | ADD A,8
4B57 | 5F          | LD E,A
4B58 | C9          | RET

; Region 4F83-5075
4F83 | 21 00 D1    | LD HL,0xd100
4F86 | 3A 00 D0    | LD A,(0xd000)
4F89 | 47          | LD B,A
4F8A | 3A 01 D0    | LD A,(0xd001)
4F8D | 4F          | LD C,A
4F8E | C3 AF 4F    | JP 0x4faf
4F91 | 21 40 D1    | LD HL,0xd140
4F94 | 11 20 00    | LD DE,0x20
4F97 | 06 16       | LD B,0x16
4F99 | C5          | PUSH BC
4F9A | 06 20       | LD B,0x20
4F9C | 7E          | LD A,(HL)
4F9D | CD 0B 4B    | CALL 0x4b0b
4FA0 | 23          | INC HL
4FA1 | 7A          | LD A,D
4FA2 | C6 08       | ADD A,8
4FA4 | 57          | LD D,A
4FA5 | 10 F5       | DJNZ 0x4f9c
4FA7 | C1          | POP BC
4FA8 | 7B          | LD A,E
4FA9 | C6 08       | ADD A,8
4FAB | 5F          | LD E,A
4FAC | 10 EB       | DJNZ 0x4f99
4FAE | C9          | RET
4FAF | 22 D5 C5    | LD (0xc5d5),HL
4FB2 | ED 43 D7 C5 | LD (0xc5d7),BC
4FB6 | F3          | DI
4FB7 | 21 F1 F0    | LD HL,0xf0f1
4FBA | 3E 0B       | LD A,11
4FBC | 32 00 60    | LD (0x6000),A
4FBF | 77          | LD (HL),A
4FC0 | 2C          | INC L
4FC1 | 3C          | INC A
4FC2 | 32 00 80    | LD (0x8000),A
4FC5 | 77          | LD (HL),A
4FC6 | 2C          | INC L
4FC7 | 3C          | INC A
4FC8 | 32 00 A0    | LD (0xa000),A
4FCB | 77          | LD (HL),A
4FCC | FB          | EI
4FCD | 3A 1A C4    | LD A,(0xc41a)
4FD0 | 21 4B 61    | LD HL,0x614b
4FD3 | A7          | AND A
4FD4 | 20 1A       | JR nz,0x4ff0
4FD6 | 3A D8 C5    | LD A,(0xc5d8)
4FD9 | 21 00 60    | LD HL,0x6000
4FDC | CD 61 40    | CALL 0x4061
4FDF | 6E          | LD L,(HL)
4FE0 | 3A D7 C5    | LD A,(0xc5d7)
4FE3 | 85          | ADD A,L
4FE4 | 26 00       | LD H,0
4FE6 | 6F          | LD L,A
4FE7 | 29          | ADD HL,HL
4FE8 | 11 13 60    | LD DE,0x6013
4FEB | 19          | ADD HL,DE
4FEC | 5E          | LD E,(HL)
4FED | 23          | INC HL
4FEE | 56          | LD D,(HL)
4FEF | EB          | EX DE,HL
4FF0 | ED 5B D5 C5 | LD DE,(0xc5d5)
4FF4 | 3E 06       | LD A,6
4FF6 | 08          | EX AF,AF'
4FF7 | 06 08       | LD B,8
4FF9 | 7E          | LD A,(HL)
4FFA | D5          | PUSH DE
4FFB | D9          | EXX
4FFC | F5          | PUSH AF
4FFD | 3A 1A C4    | LD A,(0xc41a)
5000 | A7          | AND A
5001 | 01 41 A0    | LD BC,0xa041
5004 | 20 0D       | JR nz,0x5013
5006 | 3A D8 C5    | LD A,(0xc5d8)
5009 | 87          | ADD A,A
500A | 21 BB 7E    | LD HL,0x7ebb
500D | CD 61 40    | CALL 0x4061
5010 | 4E          | LD C,(HL)
5011 | 23          | INC HL
5012 | 46          | LD B,(HL)
5013 | F1          | POP AF
5014 | 26 00       | LD H,0
5016 | 6F          | LD L,A
5017 | 29          | ADD HL,HL
5018 | 29          | ADD HL,HL
5019 | 29          | ADD HL,HL
501A | 29          | ADD HL,HL
501B | 09          | ADD HL,BC
501C | 01 FF 1C    | LD BC,0x1cff
501F | D1          | POP DE
5020 | ED A0       | LDI
5022 | ED A0       | LDI
5024 | ED A0       | LDI
5026 | ED A0       | LDI
5028 | 78          | LD A,B
5029 | 83          | ADD A,E
502A | 5F          | LD E,A
502B | ED A0       | LDI
502D | ED A0       | LDI
502F | ED A0       | LDI
5031 | ED A0       | LDI
5033 | 78          | LD A,B
5034 | 83          | ADD A,E
5035 | 5F          | LD E,A
5036 | ED A0       | LDI
5038 | ED A0       | LDI
503A | ED A0       | LDI
503C | ED A0       | LDI
503E | 78          | LD A,B
503F | 83          | ADD A,E
5040 | 5F          | LD E,A
5041 | ED A0       | LDI
5043 | ED A0       | LDI
5045 | ED A0       | LDI
5047 | ED A0       | LDI
5049 | D9          | EXX
504A | 23          | INC HL
504B | 1C          | INC E
504C | 1C          | INC E
504D | 1C          | INC E
504E | 13          | INC DE
504F | 10 A8       | DJNZ 0x4ff9
5051 | EB          | EX DE,HL
5052 | 01 60 00    | LD BC,0x60
5055 | 09          | ADD HL,BC
5056 | EB          | EX DE,HL
5057 | 08          | EX AF,AF'
5058 | 3D          | DEC A
5059 | C2 F6 4F    | JP nz,0x4ff6
505C | F3          | DI
505D | E5          | PUSH HL
505E | 21 F1 F0    | LD HL,0xf0f1
5061 | 3E 01       | LD A,1
5063 | 32 00 60    | LD (0x6000),A
5066 | 77          | LD (HL),A
5067 | 3C          | INC A
5068 | 32 00 80    | LD (0x8000),A
506B | 23          | INC HL
506C | 77          | LD (HL),A
506D | 3C          | INC A
506E | 32 00 A0    | LD (0xa000),A
5071 | 23          | INC HL
5072 | 77          | LD (HL),A
5073 | E1          | POP HL
5074 | FB          | EI
5075 | C9          | RET

; Region 53B6-53FB
53B6 | CD 8C 53    | CALL 0x538c
53B9 | 11 00 00    | LD DE,0
53BC | 0E 00       | LD C,0
53BE | 21 D8 BE    | LD HL,0xbed8
53C1 | CD 30 4A    | CALL 0x4a30
53C4 | 11 40 00    | LD DE,0x40
53C7 | 21 80 BD    | LD HL,0xbd80
53CA | 01 0E 30    | LD BC,0x300e
53CD | CD 27 4A    | CALL 0x4a27
53D0 | 21 00 BF    | LD HL,0xbf00
53D3 | 11 40 A4    | LD DE,0xa440
53D6 | 06 01       | LD B,1
53D8 | CD 66 4A    | CALL 0x4a66
53DB | C3 36 53    | JP 0x5336
53DE | CD 50 53    | CALL 0x5350
53E1 | 11 40 80    | LD DE,0x8040
53E4 | 21 24 88    | LD HL,0x8824
53E7 | 01 0E 0E    | LD BC,0xe0e
53EA | CD 27 4A    | CALL 0x4a27
53ED | 11 48 08    | LD DE,0x848
53F0 | 21 94 88    | LD HL,0x8894
53F3 | 01 0E 1A    | LD BC,0x1a0e
53F6 | CD 27 4A    | CALL 0x4a27
53F9 | C3 36 53    | JP 0x5336

; Region 564C-566F
564C | CD 9E 53    | CALL 0x539e
564F | 3A 00 D0    | LD A,(0xd000)
5652 | FE 0D       | CP 13
5654 | D4 8C 53    | CALL nc,0x538c
5657 | 3A 00 D0    | LD A,(0xd000)
565A | 87          | ADD A,A
565B | 21 42 57    | LD HL,0x5742
565E | CD 61 40    | CALL 0x4061
5661 | 5E          | LD E,(HL)
5662 | 23          | INC HL
5663 | 56          | LD D,(HL)
5664 | EB          | EX DE,HL
5665 | 11 04 80    | LD DE,0x8004
5668 | 06 BF       | LD B,0xbf
566A | CD 66 4A    | CALL 0x4a66
566D | C3 36 53    | JP 0x5336

; Region 570D-5741
570D | CD 27 57    | CALL 0x5727
5710 | CD 7A 53    | CALL 0x537a
5713 | 21 A7 BE    | LD HL,0xbea7
5716 | 3A 00 D0    | LD A,(0xd000)
5719 | 87          | ADD A,A
571A | CD 61 40    | CALL 0x4061
571D | 5E          | LD E,(HL)
571E | 23          | INC HL
571F | 56          | LD D,(HL)
5720 | EB          | EX DE,HL
5721 | CD 3E 48    | CALL 0x483e
5724 | C3 36 53    | JP 0x5336
5727 | CD 7A 53    | CALL 0x537a
572A | 21 88 BF    | LD HL,0xbf88
572D | CD 3E 48    | CALL 0x483e
5730 | C3 36 53    | JP 0x5336
5733 | CD 27 57    | CALL 0x5727
5736 | CD 7A 53    | CALL 0x537a
5739 | 21 A1 BF    | LD HL,0xbfa1
573C | CD 3E 48    | CALL 0x483e
573F | C3 36 53    | JP 0x5336

; Region 5780-57B3
5780 | CD FE 47    | CALL 0x47fe
5783 | CD 7A 53    | CALL 0x537a
5786 | 21 B0 9A    | LD HL,0x9ab0
5789 | 3A 00 D0    | LD A,(0xd000)
578C | B7          | OR A
578D | 28 22       | JR z,0x57b1
578F | 3D          | DEC A
5790 | 87          | ADD A,A
5791 | CD 61 40    | CALL 0x4061
5794 | 5E          | LD E,(HL)
5795 | 23          | INC HL
5796 | 56          | LD D,(HL)
5797 | 3A 01 D0    | LD A,(0xd001)
579A | 87          | ADD A,A
579B | 87          | ADD A,A
579C | CD 66 40    | CALL 0x4066
579F | EB          | EX DE,HL
57A0 | 7E          | LD A,(HL)
57A1 | 23          | INC HL
57A2 | E5          | PUSH HL
57A3 | 66          | LD H,(HL)
57A4 | 6F          | LD L,A
57A5 | CD 14 47    | CALL 0x4714
57A8 | E1          | POP HL
57A9 | 23          | INC HL
57AA | 7E          | LD A,(HL)
57AB | 23          | INC HL
57AC | 66          | LD H,(HL)
57AD | 6F          | LD L,A
57AE | CD 3E 48    | CALL 0x483e
57B1 | C3 36 53    | JP 0x5336

; Region 62D0-6332
62D0 | 3E 20       | LD A,0x20
62D2 | 32 15 C4    | LD (0xc415),A
62D5 | 3E 80       | LD A,0x80
62D7 | 32 18 C4    | LD (0xc418),A
62DA | CD DC 70    | CALL 0x70dc
62DD | CD 8D 54    | CALL 0x548d
62E0 | CD 68 57    | CALL 0x5768
62E3 | C3 B6 53    | JP 0x53b6
62E6 | CD 4C 56    | CALL 0x564c
62E9 | CD 0D 57    | CALL 0x570d
62EC | CD B7 63    | CALL 0x63b7
62EF | CD 02 64    | CALL 0x6402
62F2 | CD 49 5A    | CALL 0x5a49
62F5 | CD C5 63    | CALL 0x63c5
62F8 | CD 82 63    | CALL 0x6382
62FB | CD 80 57    | CALL 0x5780
62FE | CD E1 56    | CALL 0x56e1
6301 | CD 83 4F    | CALL 0x4f83
6304 | CD D4 47    | CALL 0x47d4
6307 | CD 33 63    | CALL 0x6333
630A | CD 91 4F    | CALL 0x4f91
630D | CD BE 91    | CALL 0x91be
6310 | CD 71 86    | CALL 0x8671
6313 | CD 47 91    | CALL 0x9147
6316 | CD E6 8E    | CALL 0x8ee6
6319 | 3A 00 CE    | LD A,(0xce00)
631C | FE 06       | CP 6
631E | CC 2D 63    | CALL z,0x632d
6321 | CD C7 47    | CALL 0x47c7
6324 | CD A9 9C    | CALL 0x9ca9
6327 | CD 54 61    | CALL 0x6154
632A | C3 BB 61    | JP 0x61bb
632D | CD 14 69    | CALL 0x6914
6330 | C3 A0 69    | JP 0x69a0
