#include <device/kbd.h>
#include <type.h>
#include <device/console.h>
#include <interrupt.h>
#include <device/io.h>
#include <ssulib.h>

static Key_Status KStat;
static char kbd_buf[BUFSIZ];
int buf_head, buf_tail,buf_cnt ;
static BYTE Kbd_Map[4][KBDMAPSIZE] = {
	{ /*  default */
		0x00, 0x00, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, '\t',
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x00, 'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x00, '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ /* capslock  */
		0x00, 0x00, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, '\t',
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', 0x00, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0x00, '\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ /* Shift */
		0x00, 0x00, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08, 0x00,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x00, 0x00, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0x00, '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ /* Capslock & Shift */
		0x00, 0x00, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08, 0x00,
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', 0x00, 0x00, 'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\"', '~', 0x00, '|', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', '<', '>', '?', 0x00, 0x00, 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '-', 0x00, 0x00, 0x00, '+', 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};

void init_kbd(void)
{
	KStat.ShiftFlag = 0;
	KStat.CapslockFlag = 0;
	KStat.NumlockFlag = 0;
	KStat.ScrolllockFlag = 0;
	KStat.ExtentedFlag = 0;
	KStat.PauseFlag = 0;
	buf_cnt=0;//compare backspace with normal char 
	buf_head = 0;
	buf_tail = 0;
	reg_handler(33, kbd_handler);
}

void UpdateKeyStat(BYTE Scancode)
{
	if(Scancode & 0x80)
	{
		if(Scancode == 0xB6 || Scancode == 0xAA)
		{
			KStat.ShiftFlag = FALSE;
		}
	}
	else
	{
		if(Scancode == 0x3A && KStat.CapslockFlag)
		{
			KStat.CapslockFlag = FALSE;
		}
		else if(Scancode == 0x3A)
			KStat.CapslockFlag = TRUE;

		if(Scancode == 0x45 && KStat.NumlockFlag)
		{
			KStat.NumlockFlag = FALSE;//numlock off
		}
		else if(Scancode == 0x45)
		{
			KStat.NumlockFlag = TRUE;//numlock on
		}
		else if(Scancode == 0x36 || Scancode == 0x2A)
		{
			KStat.ShiftFlag = TRUE;
		}
	}

	if(Scancode == 0xE0)
	{
		KStat.ExtentedFlag = TRUE;//extended on
	}
}

BOOL ConvertScancodeToASCII(BYTE Scancode, BYTE *Asciicode)
{
	if(KStat.PauseFlag > 0)
	{
		KStat.PauseFlag--;
		return FALSE;
	}
	if(Scancode == 0xE1)
	{
		*Asciicode = 0x00;
		KStat.PauseFlag = 2;
		return FALSE;
	}
	else if(Scancode == 0xE0)
	{
		*Asciicode = 0x00;
		return FALSE;
	}
	if((KStat.NumlockFlag==FALSE)||(KStat.ExtentedFlag==TRUE)){
		if(Scancode == 0x49){//pgup scancode in
			scroll_screen(-1);//scroll up
			return FALSE;
		}
		else if(Scancode ==0x51){//pgdn scancode in
			scroll_screen(+1);//scroll down
			return FALSE;
		}
	}
	KStat.ExtentedFlag = FALSE;//it makes extended flag to false if there was extended key 
	if(!(Scancode & 0x80))
	{
		if(KStat.ShiftFlag & KStat.CapslockFlag)
		{
			*Asciicode = Kbd_Map[3][Scancode & 0x7F];
		}
		else if(KStat.ShiftFlag)
		{
			*Asciicode = Kbd_Map[2][Scancode & 0x7F];
		}
		else if(KStat.CapslockFlag)
		{
			*Asciicode = Kbd_Map[1][Scancode & 0x7F];
		}
		else
		{
			*Asciicode = Kbd_Map[0][Scancode];
		}

		return TRUE;
	}
	return FALSE;
}

bool isFull()
{
	return (buf_head+1) % BUFSIZ == buf_tail;
}

bool isEmpty()
{
	return buf_head == buf_tail;
}


void kbd_handler(struct intr_frame *iframe)
{
	BYTE asciicode;
	BYTE data = inb(0x60);

	UpdateKeyStat(data);
	if(ConvertScancodeToASCII(data, &asciicode))
	{
		if( !isFull() && asciicode != 0)
		{
			kbd_buf[buf_tail] = asciicode;
			buf_tail = (buf_tail + 1) % BUFSIZ;//kbd buf in
		}
}
}

char kbd_read_char()
{
	if( isEmpty())
		return -1;

	char ret;
	ret = kbd_buf[buf_head];//take a char

	buf_head = (buf_head + 1)%BUFSIZ;
	if(ret=='\n')//when next line
		buf_cnt=0;//ignore count 
	else if(ret=='\b')//when backspace in
		buf_cnt--;//decrease count
	else 
		buf_cnt++;//else increase count

	if(ret=='\b' && buf_cnt<0)//if a char is bs and count is negative(number of bs is more then normal char )
		buf_cnt++;// restore cnt as zero 
	else
	{	
		printk("%c", ret);//print to console
		return ret;//return ascii code 
	}
	return -1;
}
