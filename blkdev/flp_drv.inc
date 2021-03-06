;**********************************************************
;  �����।�⢥���� ࠡ�� � ����஫��஬ ������� ��᪠
;**********************************************************
; ���� ��室���� ⥪��  �㫠��� �������� ������쥢��.
; ������� � ��ࠡ�⪠ Mario79
fdd_read_and_write:
     pusha
read_sector:
     cmp  eax,1
     jne  write_sector
     call save_HTS_values
     call flp_readsector
     call give_back_application_data
     jmp  fdd_read_end
write_sector:
     cmp  eax,2
     jne  fdd_read_end
     call save_HTS_values
     call take_data_from_application
     call flp_writesector
fdd_read_end:
     popa
     ret

save_HTS_values:
     mov  [FDD_Sector],bl
     mov  [FDD_Head],bh
     shr  ebx,16
     mov  [FDD_Track],bl
     mov  [FDD_Type],bh
     ret

give_back_application_data:  ; ���᫠�� �ਫ������   
     mov edi,[3010h]
     mov edi,[edi+10h]
     add edi,ecx
give_back_application_data_1:
     mov esi,0xD000   ;FDD_DataBuffer  ;0x40000
     xor ecx,ecx
     mov cx,128
     cld
     rep movsd
     ret

take_data_from_application:   ; ����� �� �ਫ������
     mov esi,[3010h]
     mov esi,[esi+10h]
     add esi,ecx
take_data_from_application_1:
     mov edi,0xD000   ;FDD_DataBuffer  ;0x40000
     xor ecx,ecx
     mov cx,128
     cld
     rep movsd
     ret

flp_initialization:
; ��⠭����� ���� ��ࠡ��稪 ���뢠��� ����
        call    SetUserInterrupts
; ������� ���� ��᪮����
        call    FDDMotorON        
; ���樠����஢��� ��६����
;        mov     [FDD_Track],0
;        mov     [FDD_Head],0
;        mov     [FDD_Sector],1
; �஢��� ४����஢�� � ���� �㫥��� ��஦��
        call    RecalibrateFDD
        call    SeekTrack
        ret

flp_readsector:
        call    flp_initialization               
; ������ ᥪ��
        call    ReadSectWithRetr
;        call  ReadSector
;        cmp     [FDC_Status],0
;        jne     @@SectorNotFound
        mov [fdc_irq_func],fdc_null
;        call    FDDMotorOFF
        ret

flp_writesector:
        call    flp_initialization               
; ������� ᥪ��
        call    WriteSectWithRetr
;        call    WriteSector
;        cmp     [FDC_Status],0
;        jne     @@SectorNotFound
        mov [fdc_irq_func],fdc_null
;        call    FDDMotorOFF
        ret

@@DiskNotFound:
        ret
@@SectorNotFound:
        ret

; ���� �����襭�� ����樨 � ����஫��஬ (FDC_Status)
FDC_Normal         equ 0 ;��ଠ�쭮� �����襭��
FDC_TimeOut        equ 1 ;�訡�� ⠩�-���
FDC_DiskNotFound   equ 2 ;� ��᪮���� ��� ��᪠
FDC_TrackNotFound  equ 3 ;��஦�� �� �������
FDC_SectorNotFound equ 4 ;ᥪ�� �� ������

; ���ᨬ���� ���祭�� ���न��� ᥪ�� (�������
; ���祭�� ᮮ⢥������ ��ࠬ��ࠬ �⠭���⭮��
; �������� ������� ��᪠ ��ꥬ�� 1,44 ��)
MAX_Track   equ 79
MAX_Head    equ  1
MAX_Sector  equ 18

; ���稪 ⨪�� ⠩���
TickCounter dd ?
; ��� �����襭�� ����樨 � ����஫��஬ ����
FDC_Status  DB ?
; ���� ���뢠��� �� ����
FDD_IntFlag DB ?
; ������ ��砫� ��᫥���� ����樨 � ����
FDD_Time    DD ?
; ����� ��᪮����
FDD_Type    db 0
; ���न���� ᥪ��
FDD_Track   DB ?
FDD_Head    DB ?
FDD_Sector  DB ?

; ���� १���� ����樨
FDC_ST0 DB ?
FDC_ST1 DB ?
FDC_ST2 DB ?
FDC_C   DB ?
FDC_H   DB ?
FDC_R   DB ?
FDC_N   DB ?
; ���稪 ����७�� ����樨 �⥭��
ReadRepCounter  DB ?
; ���稪 ����७�� ����樨 ४����஢��
RecalRepCounter DB ?
; ������� ����� ��� �࠭���� ���⠭���� ᥪ��
;FDD_DataBuffer:  times 512 db 0   ;DB 512 DUP (?)
fdd_motor_status db 0
timer_fdd_motor  dd 0

;*************************************
;* ������������� ������ ��� ��� ���� *
;*************************************
Init_FDC_DMA:
        pushad
        mov al,0
        out 0x0c,al     ; reset the flip-flop to a known state.
        mov al,6                ; mask channel 2 so we can reprogram it.
        out 0x0a,al
        mov al,[dmamode] ; 0x46 -> Read from floppy - 0x4A Write to floppy
        out 0x0b,al
        mov al,0
        out 0x0c,al     ; reset the flip-flop to a known state.
        mov eax,0xD000
        out 0x04,al     ; set the channel 2 starting address to 0
        shr eax,8
        out 0x04,al
        shr eax,8
        out 0x81,al
        mov al,0
        out 0x0c, al    ; reset flip-flop
        mov al, 0xff    ;set count (actual size -1)
        out 0x5, al
        mov al,0x1  ;[dmasize]       ;(0x1ff = 511 / 0x23ff =9215)
        out 0x5,al
        mov al,2
        out 0xa,al
        popad
        ret

;***********************************
;* �������� ���� � ���� ������ FDC *
;* ��ࠬ����:                      *
;* AL - �뢮���� ����.            *
;***********************************
FDCDataOutput:
;        pusha
        push ax cx dx
        mov     AH,AL     ;��������� ���� � AH
; ������ ��६����� ���ﭨ� ����஫���
        mov     [FDC_Status],FDC_Normal
; �஢���� ��⮢����� ����஫��� � �ਥ�� ������
        mov     DX,3F4h   ;(���� ���ﭨ� FDC)
        xor     CX,CX     ;��⠭����� ���稪 ⠩�-���
@@TestRS:
        in      AL,DX     ;������ ॣ���� RS
        and     AL,0C0h   ;�뤥���� ࠧ��� 6 � 7
        cmp     AL,80h    ;�஢���� ࠧ��� 6 � 7
        je      @@OutByteToFDC
        loop    @@TestRS
; �訡�� ⠩�-���
        mov     [FDC_Status],FDC_TimeOut
        jmp @@End_5
; �뢥�� ���� � ���� ������
@@OutByteToFDC:
        inc     DX
        mov     AL,AH
        out     DX,AL
@@End_5:
;        popa
        pop dx cx ax 
        ret

;******************************************
;*   ��������� ���� �� ����� ������ FDC   *
;* ��楤�� �� ����� �室��� ��ࠬ��஢. *
;* ��室�� �����:                       *
;* AL - ��⠭�� ����.                   *
;******************************************
FDCDataInput:
        push    ECX
        push    DX
; ������ ��६����� ���ﭨ� ����஫���
        mov     [FDC_Status],FDC_Normal
; �஢���� ��⮢����� ����஫��� � ��।�� ������
        mov     DX,3F4h   ;(���� ���ﭨ� FDC)
        xor     CX,CX     ;��⠭����� ���稪 ⠩�-���
@@TestRS_1:
        in      AL,DX     ;������ ॣ���� RS
        and     AL,0C0h   ;�뤫��� ࠧ��� 6 � 7
        cmp     AL,0C0h   ;�஢���� ࠧ��� 6 � 7
        je      @@GetByteFromFDC
        loop    @@TestRS_1
; �訡�� ⠩�-���
        mov     [FDC_Status],FDC_TimeOut
        jmp @@End_6
; ����� ���� �� ���� ������
@@GetByteFromFDC:
        inc     DX
        in      AL,DX
@@End_6:  pop     DX
        pop     ECX
        ret

;*********************************************
;* ���������� ���������� �� ����������� ���� *
;*********************************************
FDCInterrupt:
; ������� ���뢠���
;        sti
;        push    AX
; ��⠭����� 䫠� ���뢠���
        mov     [FDD_IntFlag],1
; ��᫠�� ������� EOI ����஫���� ���뢠���
;        mov     AL,20h
;        out     20h,AL
;        pop     AX
        ret


;******************************************
;* ���������� ����� ���������� ���������� *
;*             ����                       *
;******************************************
SetUserInterrupts:
         mov     [fdc_irq_func],FDCInterrupt
         ret

;*******************************************
;* �������� ���������� �� ����������� ���� *
;*******************************************
WaitFDCInterrupt:
        pusha
; ������ ���� ���ﭨ� ����樨
        mov     [FDC_Status],FDC_Normal
; ������ 䫠� ���뢠���
        mov     [FDD_IntFlag],0
; ���㫨�� ���稪 ⨪��
        mov     eax,[timer_ticks]
        mov     [TickCounter],eax
; ������� ��⠭���� 䫠�� ���뢠��� ����
@@TestRS_2:
        cmp     [FDD_IntFlag],0
        jnz     @@End_7           ;���뢠��� �ந��諮
        mov     eax,[timer_ticks]
        sub     eax,[TickCounter]
        cmp     eax,50  ;25   ;5 ;������� 5 ⨪��
        jb      @@TestRS_2
;        jl      @@TestRS_2
; �訡�� ⠩�-���
        mov     [FDC_Status],FDC_TimeOut
;        mov   [flp_status],0
@@End_7:  popa
        ret

;*********************************
;* �������� ����� ��������� "A:" *
;*********************************
FDDMotorON:
        pusha
;        cmp     [fdd_motor_status],1
;        je      fdd_motor_on
        mov     al,[flp_number]
        cmp     [fdd_motor_status],al
        je      fdd_motor_on
; �ந����� ��� ����஫��� ����
        mov     DX,3F2h ;���� �ࠢ����� �����⥫ﬨ
        mov     AL,0
        out     DX,AL
; ����� � ������� ���� ��᪮����
        cmp     [flp_number],1
        jne     FDDMotorON_B
;        call    FDDMotorOFF_B
        mov     AL,1Ch    ; Floppy A
        jmp     FDDMotorON_1
FDDMotorON_B:
;        call    FDDMotorOFF_A
        mov     AL,2Dh    ; Floppy B
FDDMotorON_1:
        out     DX,AL
; ���㫨�� ���稪 ⨪��
        mov     eax,[timer_ticks]
        mov     [TickCounter],eax
; ������� 0,5 �
@@dT:
        mov     eax,[timer_ticks]
        sub     eax,[TickCounter]
        cmp     eax,50  ;10
        jb      @@dT
        cmp     [flp_number],1
        jne     fdd_motor_on_B
        mov     [fdd_motor_status],1
        jmp     fdd_motor_on
fdd_motor_on_B:
        mov     [fdd_motor_status],2
fdd_motor_on:
        call    save_timer_fdd_motor
        popa
        ret

;*****************************************
;*  ���������� ��������� �������         *
;*****************************************
save_timer_fdd_motor:
        mov     eax,[timer_ticks]
        mov     [timer_fdd_motor],eax
        ret

;*****************************************
;*  �������� �������� ���������� ������  *
;*****************************************
check_fdd_motor_status:
        cmp     [fdd_motor_status],0
        je      end_check_fdd_motor_status
        mov     eax,[timer_ticks]
        sub     eax,[timer_fdd_motor]
        cmp     eax,500
        jb      end_check_fdd_motor_status
        call    FDDMotorOFF
        mov     [fdd_motor_status],0
end_check_fdd_motor_status:
        ret

;**********************************
;* ��������� ����� ���������      *
;**********************************
FDDMotorOFF:
        push    AX
        push    DX
        cmp     [flp_number],1
        jne     FDDMotorOFF_1
        call    FDDMotorOFF_A
        jmp     FDDMotorOFF_2
FDDMotorOFF_1:
        call    FDDMotorOFF_B
FDDMotorOFF_2:
        pop     DX
        pop     AX
        ; ��� 䫠��� ���஢���� � �裡 � ���ॢ����� ���ଠ樨
        mov    [root_read],0
        mov    [flp_fat],0
        ret

FDDMotorOFF_A:
        mov     DX,3F2h ;���� �ࠢ����� �����⥫ﬨ
        mov     AL,0Ch  ; Floppy A
        out     DX,AL
        ret

FDDMotorOFF_B:
        mov     DX,3F2h ;���� �ࠢ����� �����⥫ﬨ
        mov     AL,5h  ; Floppy B
        out     DX,AL
        ret

;*******************************
;* ������������ ��������� "A:" *
;*******************************
RecalibrateFDD:
        pusha
        call    save_timer_fdd_motor
; ������ ������� "�������஢��"
        mov     AL,07h
        call    FDCDataOutput
        mov     AL,00h
        call    FDCDataOutput
; ������� �����襭�� ����樨
        call    WaitFDCInterrupt
;        cmp    [FDC_Status],0
;        je    no_fdc_status_error
;        mov   [flp_status],0
;no_fdc_status_error:
        call    save_timer_fdd_motor
        popa
        ret

;*****************************************************
;*                    ����� �������                  *
;* ��ࠬ���� ��।����� �१ �������� ��६����: *
;* FDD_Track - ����� ��஦�� (0-79);                 *
;* FDD_Head - ����� ������� (0-1).                   *
;* ������� ����樨 �������� � FDC_Status.        *
;*****************************************************
SeekTrack:
        pusha
        call    save_timer_fdd_motor
; ������ ������� "����"
        mov     AL,0Fh
        call    FDCDataOutput
        ; ��।��� ���� ����� �������/������⥫�
        mov     AL,[FDD_Head]
        shl     AL,2
        call    FDCDataOutput
        ; ��।��� ���� ����� ��஦��
        mov     AL,[FDD_Track]
        call    FDCDataOutput
; ������� �����襭�� ����樨
        call    WaitFDCInterrupt
        cmp     [FDC_Status],FDC_Normal
        jne     @@Exit
; ���࠭��� १���� ���᪠
        mov     AL,08h
        call    FDCDataOutput
        call    FDCDataInput
        mov     [FDC_ST0],AL
        call    FDCDataInput
        mov     [FDC_C],AL
; �஢���� १���� ���᪠
        ; ���� �����襭?
        test    [FDC_ST0],100000b
        je      @@Err
        ; ������� �४ ������?
        mov     AL,[FDC_C]
        cmp     AL,[FDD_Track]
        jne     @@Err
        ; ����� ������� ᮢ������ � �������?
        mov     AL,[FDC_ST0]
        and     AL,100b
        shr     AL,2
        cmp     AL,[FDD_Head]
        jne     @@Err
        ; ������ �����襭� �ᯥ譮
        mov     [FDC_Status],FDC_Normal
        jmp @@Exit
@@Err:  ; �४ �� ������
        mov     [FDC_Status],FDC_TrackNotFound
;        mov   [flp_status],0
@@Exit:
        call    save_timer_fdd_motor
        popa
        ret

;*******************************************************
;*               ������ ������� ������                 *
;* ��ࠬ���� ��।����� �१ �������� ��६����:   *
;* FDD_Track - ����� ��஦�� (0-79);                   *
;* FDD_Head - ����� ������� (0-1);                     *
;* FDD_Sector - ����� ᥪ�� (1-18).                  *
;* ������� ����樨 �������� � FDC_Status.          *
;* � ��砥 �ᯥ譮�� �믮������ ����樨 �⥭��       *
;* ᮤ�ন��� ᥪ�� �㤥� ����ᥭ� � FDD_DataBuffer. *
;*******************************************************
ReadSector:
        pushad
        call    save_timer_fdd_motor
; ��⠭����� ᪮���� ��।�� 500 �����/�
        mov     AX,0
        mov     DX,03F7h
        out     DX,AL
; ���樠����஢��� ����� ��אַ�� ����㯠 � �����
        mov     [dmamode],0x46
        call    Init_FDC_DMA
; ������ ������� "�⥭�� ������"
        mov     AL,0E6h  ;�⥭�� � �����४���� ०���
        call    FDCDataOutput
        mov     AL,[FDD_Head]
        shl     AL,2
        call    FDCDataOutput
        mov     AL,[FDD_Track]
        call    FDCDataOutput
        mov     AL,[FDD_Head]
        call    FDCDataOutput
        mov     AL,[FDD_Sector]
        call    FDCDataOutput
        mov     AL,2    ;��� ࠧ��� ᥪ�� (512 ����)
        call    FDCDataOutput
        mov     AL,18  ;+1; 3Fh  ;�᫮ ᥪ�஢ �� ��஦��
        call    FDCDataOutput
        mov     AL,1Bh  ;���祭�� GPL
        call    FDCDataOutput
        mov     AL,0FFh ;���祭�� DTL
        call    FDCDataOutput

; ������� ���뢠��� �� �����襭�� ����樨
        call    WaitFDCInterrupt
        cmp     [FDC_Status],FDC_Normal
        jne     @@Exit_1
; ���뢠�� ����� �����襭�� ����樨
        call    GetStatusInfo
        test    [FDC_ST0],11011000b
        jnz     @@Err_1
        mov     [FDC_Status],FDC_Normal
        jmp @@Exit_1
@@Err_1:  mov     [FDC_Status],FDC_SectorNotFound
;        mov   [flp_status],0
@@Exit_1:
        call    save_timer_fdd_motor
        popad
        ret

;*******************************************************
;*   ������ ������� (� ����������� �������� ��� ����)  *
;* ��ࠬ���� ��।����� �१ �������� ��६����:   *
;* FDD_Track - ����� ��஦�� (0-79);                   *
;* FDD_Head - ����� ������� (0-1);                     *
;* FDD_Sector - ����� ᥪ�� (1-18).                  *
;* ������� ����樨 �������� � FDC_Status.          *
;* � ��砥 �ᯥ譮�� �믮������ ����樨 �⥭��       *
;* ᮤ�ন��� ᥪ�� �㤥� ����ᥭ� � FDD_DataBuffer. *
;*******************************************************
ReadSectWithRetr:
        pusha
; ���㫨�� ���稪 ����७�� ����樨 ४����஢��
        mov     [RecalRepCounter],0
@@TryAgain:
; ���㫨�� ���稪 ����७�� ����樨 �⥭��
        mov     [ReadRepCounter],0
@@ReadSector_1:
        call    ReadSector
        cmp     [FDC_Status],0
        je      @@Exit_2
        cmp     [FDC_Status],1
        je      @@Err_3 
        ; �஥��⭮� ����७�� �⥭��
        inc     [ReadRepCounter]
        cmp     [ReadRepCounter],3
        jb      @@ReadSector_1
        ; �஥��⭮� ����७�� ४����஢��
        call    RecalibrateFDD
        call    SeekTrack
        inc     [RecalRepCounter]
        cmp     [RecalRepCounter],3
        jb      @@TryAgain
;        mov   [flp_status],0
@@Exit_2:
        popa
        ret
@@Err_3:
        mov   [flp_status],0
        popa
        ret

;*******************************************************
;*               ������ ������� ������                 *
;* ��ࠬ���� ��।����� �१ �������� ��६����:   *
;* FDD_Track - ����� ��஦�� (0-79);                   *
;* FDD_Head - ����� ������� (0-1);                     *
;* FDD_Sector - ����� ᥪ�� (1-18).                  *
;* ������� ����樨 �������� � FDC_Status.          *
;* � ��砥 �ᯥ譮�� �믮������ ����樨 �����       *
;* ᮤ�ন��� FDD_DataBuffer �㤥� ����ᥭ� � ᥪ��.  *
;*******************************************************
WriteSector:
        pushad
        call    save_timer_fdd_motor
; ��⠭����� ᪮���� ��।�� 500 �����/�
        mov     AX,0
        mov     DX,03F7h
        out     DX,AL
; ���樠����஢��� ����� ��אַ�� ����㯠 � �����
        mov     [dmamode],0x4A
        call    Init_FDC_DMA
; ������ ������� "������ ������"
        mov     AL,0xC5  ;0x45  ;������ � �����४���� ०���
        call    FDCDataOutput
        mov     AL,[FDD_Head]
        shl     AL,2
        call    FDCDataOutput
        mov     AL,[FDD_Track]
        call    FDCDataOutput
        mov     AL,[FDD_Head]
        call    FDCDataOutput
        mov     AL,[FDD_Sector]
        call    FDCDataOutput
        mov     AL,2    ;��� ࠧ��� ᥪ�� (512 ����)
        call    FDCDataOutput
        mov     AL,18; 3Fh  ;�᫮ ᥪ�஢ �� ��஦��
        call    FDCDataOutput
        mov     AL,1Bh  ;���祭�� GPL
        call    FDCDataOutput
        mov     AL,0FFh ;���祭�� DTL
        call    FDCDataOutput

; ������� ���뢠��� �� �����襭�� ����樨
        call    WaitFDCInterrupt
        cmp     [FDC_Status],FDC_Normal
        jne     @@Exit_3
; ���뢠�� ����� �����襭�� ����樨
        call    GetStatusInfo
        test    [FDC_ST0],11000000b  ;11011000b
        jnz     @@Err_2
        mov     [FDC_Status],FDC_Normal
        jmp @@Exit_3
@@Err_2:  mov     [FDC_Status],FDC_SectorNotFound
@@Exit_3:
        call    save_timer_fdd_motor
        popad
        ret

;*******************************************************
;*   ������ ������� (� ����������� �������� ��� ����)  *
;* ��ࠬ���� ��।����� �१ �������� ��६����:   *
;* FDD_Track - ����� ��஦�� (0-79);                   *
;* FDD_Head - ����� ������� (0-1);                     *
;* FDD_Sector - ����� ᥪ�� (1-18).                  *
;* ������� ����樨 �������� � FDC_Status.          *
;* � ��砥 �ᯥ譮�� �믮������ ����樨 �����       *
;* ᮤ�ন��� FDD_DataBuffer �㤥� ����ᥭ� � ᥪ��.  *
;*******************************************************
WriteSectWithRetr:
        pusha
; ���㫨�� ���稪 ����७�� ����樨 ४����஢��
        mov     [RecalRepCounter],0
@@TryAgain_1:
; ���㫨�� ���稪 ����७�� ����樨 �⥭��
        mov     [ReadRepCounter],0
@@WriteSector_1:
        call    WriteSector
        cmp     [FDC_Status],0
        je      @@Exit_4
        cmp     [FDC_Status],1
        je      @@Err_4
        ; �஥��⭮� ����७�� �⥭��
        inc     [ReadRepCounter]
        cmp     [ReadRepCounter],3
        jb      @@WriteSector_1
        ; �஥��⭮� ����७�� ४����஢��
        call    RecalibrateFDD
        call    SeekTrack
        inc     [RecalRepCounter]
        cmp     [RecalRepCounter],3
        jb      @@TryAgain_1
@@Exit_4:
        popa
        ret
@@Err_4:
        mov   [flp_status],0
        popa
        ret

;*********************************************
;* �������� ���������� � ���������� �������� *
;*********************************************
GetStatusInfo:
        push    AX
        call    FDCDataInput
        mov     [FDC_ST0],AL
        call    FDCDataInput
        mov     [FDC_ST1],AL
        call    FDCDataInput
        mov     [FDC_ST2],AL
        call    FDCDataInput
        mov     [FDC_C],AL
        call    FDCDataInput
        mov     [FDC_H],AL
        call    FDCDataInput
        mov     [FDC_R],AL
        call    FDCDataInput
        mov     [FDC_N],AL
        pop     AX
        ret

