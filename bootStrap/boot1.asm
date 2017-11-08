org	0x9000  

[BITS 16]  

		cli		; Clear Interrupt Flag

		mov     ax, 0xb800 
        mov     es, ax
        mov     ax, 0x00
        mov     bx, 0
        mov     cx, 80*25*2 
CLS:
        mov     [es:bx], ax
        add     bx, 1
        loop    CLS 
 
Initialize_PIC:
		;ICW1 - 두 개의 PIC를 초기화 
		mov		al, 0x11
		out		0x20, al
		out		0xa0, al

		;ICW2 - 발생된 인터럽트 번호에 얼마를 더할지 결정
		mov		al, 0x20
		out		0x21, al
		mov		al, 0x28
		out		0xa1, al

		;ICW3 - 마스터/슬레이브 연결 핀 정보 전달
		mov		al, 0x04
		out		0x21, al
		mov		al, 0x02
		out		0xa1, al

		;ICW4 - 기타 옵션 
		mov		al, 0x01
		out		0x21, al
		out		0xa1, al

		mov		al, 0xFF
		;out		0x21, al
		out		0xa1, al

Initialize_Serial_port:
		xor		ax, ax
		xor		dx, dx
		mov		al, 0xe3
		int		0x14

READY_TO_PRINT:
		xor		si, si
		xor		bh, bh
PRINT_TO_SERIAL:
		mov		al, [msgRMode+si]
		mov		ah, 0x01
		int		0x14
		add		si, 1
		cmp		al, 0
		jne		PRINT_TO_SERIAL
PRINT_NEW_LINE:
		mov		al, 0x0a
		mov		ah, 0x01
		int		0x14
		mov		al, 0x0d
		mov		ah, 0x01
		int		0x14

; OS assignment 2
; add your code here
; print current date to boch display

		mov		si,0	;si레지스터에 0을 넣는다
		mov 		bh,0	;bh에 0을 넣고
		mov		ah,0x4	;현재 날짜를 알려주는 int 1a의 4h 함수를 
		int		0x1a	;인터럽트를 발생시킨다
		mov		bh,ch	;날짜중 연도의 상위두 글자(19h~20h)를 bh에 넣는다
		call		PACK	;PACK으로 이동한다
		mov		bh,cl	;날짜중 연도의 하위 두 글자 (00h~99h)를 bh에 넣는다
		call		PACK
		mov		bh,dh	;날짜중 월을 의미하는 (01h~12h)를 bh에 넣는다
		call		PACK
		mov		bh,dl	;날짜중 일을 의미하는 (01h~31h)를 bh에 넣는다
		call		PACK
		jmp		EPACK	;PACK
PACK:
		mov		bl,bh	;bh에 들어있는 1바이트값을 bl에 복사한다
		shr		bh,4	;bh레지스터의 비트를 4비트 오른쪽으로 이동시킨다
		add		bh,48	;이후 48 즉 16진수로 30h를 증가시키면 4비트에 대한 값이 8비트 10진수 아스키로 패킹이 된다
		mov		[msg+si],bh;1바이트 아스키를 msg+si 주소위치에 저장한다
		inc		si	;si레지스터를 1증가시킨다
		and		bl,0x0f	;처음에 복사해둔 8비트 16진수 값의 상위 4비트를 0으로 만들고
		add		bl,48	;10진수 아스키값으로 다시 패킹한다
		mov		[msg+si],bl	;이값을 bl에서 msg+si 주소 위치에 저장한다
		inc		si	;다시 si를 1증가시킨다
		ret
EPACK:	;날짜에대한 패킹이 끝나 날짜 데이터가 완성이 되면 


		mov 		ax, 1301h	;인터럽트 10h의 13함수를 부를 준비를 한다
		mov 		bx, 0x0e	;색상 값을 넣어준다 
		mov 		cx, msgend- msg	;cx에는 msg와 msgend의 주소의 차를 이용하여 길이를 저장한다
		mov 		dx, 40 ;출력될 위치는 80중에 가운데 40부터 시작한다
		mov 		si, 0	;si레지스터를 초기화한뒤
		push 		si	;스택에 넣어준다
		pop 		es	;pop해주면서 넣어준 si의 값을 복구한다
		mov 		bp, msg ;bp에 날자 데이터가 저장된 주소를 넣어준다 
		int 		10h	;이후 인터럽트 10을 호출한다




Activate_A20Gate:
		mov		ax,	0x2401
		int		0x15

;Detecting_Memory:
;		mov		ax, 0xe801
;		int		0x15

PROTECTED:
        xor		ax, ax
        mov		ds, ax              

		call	SETUP_GDT

        mov		eax, cr0  
        or		eax, 1	  
        mov		cr0, eax  

		jmp		$+2
		nop
		nop
		jmp		CODEDESCRIPTOR:ENTRY32

SETUP_GDT:
		lgdt	[GDT_DESC]
		ret

[BITS 32]  

ENTRY32:
		mov		ax, 0x10
		mov		ds, ax
		mov		es, ax
		mov		fs, ax
		mov		gs, ax

		mov		ss, ax
  		mov		esp, 0xFFFE
		mov		ebp, 0xFFFE	

		mov		edi, 80*2
		lea		esi, [msgPMode]
		call	PRINT

		;IDT TABLE
	    cld
		mov		ax,	IDTDESCRIPTOR
		mov		es, ax
		xor		eax, eax
		xor		ecx, ecx
		mov		ax, 256
		mov		edi, 0
 
IDT_LOOP:
		lea		esi, [IDT_IGNORE]
		mov		cx, 8
		rep		movsb
		dec		ax
		jnz		IDT_LOOP

		lidt	[IDTR]

		sti
		jmp	CODEDESCRIPTOR:0x10000

PRINT:
		push	eax
		push	ebx
		push	edx
		push	es
		mov		ax, VIDEODESCRIPTOR
		mov		es, ax
PRINT_LOOP:
		or		al, al
		jz		PRINT_END
		mov		al, byte[esi]
		mov		byte [es:edi], al
		inc		edi
		mov		byte [es:edi], 0x07

OUT_TO_SERIAL:
		mov		bl, al
		mov		dx, 0x3fd
CHECK_LINE_STATUS:
		in		al, dx
		and		al, 0x20
		cmp		al, 0
		jz		CHECK_LINE_STATUS
		mov		dx, 0x3f8
		mov		al, bl
		out		dx, al

		inc		esi
		inc		edi
		jmp		PRINT_LOOP
PRINT_END:
LINE_FEED:
		mov		dx, 0x3fd
		in		al, dx
		and		al, 0x20
		cmp		al, 0
		jz		LINE_FEED
		mov		dx, 0x3f8
		mov		al, 0x0a
		out		dx, al
CARRIAGE_RETURN:
		mov		dx, 0x3fd
		in		al, dx
		and		al, 0x20
		cmp		al, 0
		jz		CARRIAGE_RETURN
		mov		dx, 0x3f8
		mov		al, 0x0d
		out		dx, al

		pop		es
		pop		edx
		pop		ebx
		pop		eax
		ret

GDT_DESC:
        dw GDT_END - GDT - 1    
        dd GDT                 
GDT:
		NULLDESCRIPTOR equ 0x00
			dw 0 
			dw 0 
			db 0 
			db 0 
			db 0 
			db 0
		CODEDESCRIPTOR  equ 0x08
			dw 0xffff             
			dw 0x0000              
			db 0x00                
			db 0x9a                    
			db 0xcf                
			db 0x00                
		DATADESCRIPTOR  equ 0x10
			dw 0xffff              
			dw 0x0000              
			db 0x00                
			db 0x92                
			db 0xcf                
			db 0x00                
		VIDEODESCRIPTOR equ 0x18
			dw 0xffff              
			dw 0x8000              
			db 0x0b                
			db 0x92                
			db 0x40                    
			;db 0xcf                    
			db 0x00                 
		IDTDESCRIPTOR	equ 0x20
			dw 0xffff
			dw 0x0000
			db 0x02
			db 0x92
			db 0xcf
			db 0x00
GDT_END:
IDTR:
		dw 256*8-1
		dd 0x00020000
IDT_IGNORE:
		dw ISR_IGNORE
		dw CODEDESCRIPTOR
		db 0
		db 0x8E
		dw 0x0000
ISR_IGNORE:
		push	gs
		push	fs
		push	es
		push	ds
		pushad
		pushfd
		cli
		nop
		sti
		popfd
		popad
		pop		ds
		pop		es
		pop		fs
		pop		gs
		iret

msgRMode db "Real Mode", 0
msgPMode db "Protected Mode", 0
msg db "99999999",0
msgend:
times 	2048-($-$$) db 0x00
