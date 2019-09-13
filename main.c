#include "stdio.h"
#include "M0564.h"
#include <string.h>
#include "ff.h"
#include "diskio.h"

#define PLL_CLOCK       72000000


unsigned int MidDid;
volatile UINT Timer = 0;		/* Performance timer (1kHz increment) */
DWORD acc_size;				/* Work register for fs command */
WORD acc_files, acc_dirs;
FILINFO Finfo;
FATFS  FatFs[_DRIVES];		/* File system object for logical drive */
char Line[256];				/* Console input buffer */
#if _USE_LFN
char Lfname[512];
#endif
BYTE Buff[1024] ;		/* Working buffer */

uint8_t data[512];
uint32_t sd_size;
uint8_t aRxBuffer[1024],ui8CurrentState=0;;
uint16_t SI=0,CI=0;
#define RECEIVE_STATE 0
#define PROCESS_STATE 1

void UART02_IRQHandler(void)
{
	UART_Read(UART0,&aRxBuffer[SI],1);
	if(SI==1024)
		SI=0;
 UART_Write(UART0,&aRxBuffer[SI],1);
	//printf("%d",aRxBuffer[SI]);
	ui8CurrentState=PROCESS_STATE;
	
	SI++;
	
}





void put_rc (FRESULT rc)
{
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0");

    uint32_t i;
    for (i = 0; (i != (UINT)rc) && *p; i++) {
        while(*p++) ;
    }
    printf(_T("rc=%u FR_%s\n"), (UINT)rc, p);
}
void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Waiting for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Switch HCLK clock source to HIRC */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable HXT */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL and SysTick source to HCLK/2*/
    CLK_SetCoreClock(PLL_CLOCK);
    CLK_SetSysTickClockSrc(CLK_CLKSEL0_STCLKSEL_HCLK_DIV2);

    /* Enable peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);
  
    /* Peripheral clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UARTSEL_PLL, CLK_CLKDIV0_UART(1));
    

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PD multi-function pins for UART0 RXD, TXD */
    SYS->GPD_MFPL = SYS_GPD_MFPL_PD0MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD;
}

void UART0_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART module */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}




int main()
{
	int count=0,i=0;
	uint8_t cmd[20];
	uint32_t p1, s1, s2;
	FATFS  *fs;				/* Pointer to file system object */
	DIR dir;				/* Directory object */
	FILINFO Finfo;
FIL fil;
	char *ptr="\\dir" ,*fileptr="\\file1";
	FRESULT res;   
	SystemCoreClockUpdate();
	SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
  //  SYS_LockReg();

    /* Init UART0 for printf */
  UART0_Init();
	
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk);
		NVIC_EnableIRQ(UART02_IRQn);
	FMC_Open();
	FMC_EnableAPUpdate();
	
	res	= f_mount(0,&FatFs[0]);	
	if(res)
	{
		put_rc(res);
		printf("Don't mount file system\n");
	}
	#if 0
	 res = f_mkfs(0, 1, _MAX_SS); 
		if(res)
	{
		put_rc(res);
		printf("Don't format\n");
	}	 
	#endif

	while(1)
	{
		if(CI==1024)
			CI=0;
		
		if(ui8CurrentState==PROCESS_STATE  && aRxBuffer[SI-1]==13)
		{
		
			printf("\n");
			
			UART_Write(UART0,&aRxBuffer[CI],SI-CI);
		
			printf("\n");
	if(strncmp(&aRxBuffer[CI],"ls",2)==0)		
	{
		// List direct information
  put_rc(f_mkdir(ptr));
	put_rc(f_open(&fil,fileptr,FA_CREATE_NEW));
		put_rc(f_open("\\dir\\,FA_CREATE_NEW));
	put_rc(f_opendir(&dir,"\\"));

	p1 = s1 = s2 = 0;
	for(;;) 
	{
		res = f_readdir(&dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0]) break;
		if (Finfo.fattrib & AM_DIR){
			s2++;
		} else {
			s1++; p1 += Finfo.fsize;
		}
		printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n",
			(Finfo.fattrib & AM_DIR) ? 'D' : '-',
			(Finfo.fattrib & AM_RDO) ? 'R' : '-',
			(Finfo.fattrib & AM_HID) ? 'H' : '-',
			(Finfo.fattrib & AM_SYS) ? 'S' : '-',
			(Finfo.fattrib & AM_ARC) ? 'A' : '-',
			(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
			(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, Finfo.fsize, Finfo.fname);
	}
	  printf("%4u File(s),%10lu bytes total\n%4u Dir(s)", s1, p1, s2);
	  if (f_getfree(ptr, (DWORD*)&p1, &fs) == FR_OK)
		printf(", %10lu bytes free\n", p1 * fs->csize * 512);
f_close(&fil);
	
		
	}
		
		
//		UART_Write(UART0,&cmd[0],count);
			
			CI=SI;
			ui8CurrentState=RECEIVE_STATE;
	
			printf("\n\ranirban@terminal$ > ");
			
		}
	}
	

	

	
f_mount(0,NULL);
	
    while(1);
return 0;	
	
}