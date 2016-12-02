/**********************************************
	(1)
	If we use printf and wprintf at the same time,
		it does't work for gcc and ucc.
	(2)
		clang and gcc have different processing for
		the following:
		const wchar_t *wcs = L"abc你好abc";	
		Clang:
			0x61 0x62 0x63 0xe4 0xbd 0xa0 0xe5 0xa5 0xbd 0x61 0x62 0x63
		GCC:
			0x61 0x62 0x63 0x4f60 0x597d 0x61 0x62 0x63
 **********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>

char * cstr = "abc你好abc";

void ConvertMB2WC(const char * str){
	const char * ptr = str;
	size_t count;
	wchar_t wc;
	wprintf(L"multiple bytes:\n");
	while(*ptr){
		wprintf(L"0x%x ",(unsigned char)*ptr);
		ptr++;
	}
	ptr = str;
	wprintf(L"\nwide chars:\n");
	while(*ptr){		
		count = mbrtowc(&wc, ptr, MB_CUR_MAX, 0);
		wprintf(L"0x%x ",wc);
		ptr += count;
	}
	wprintf(L"\n");
}

int main(){
	const wchar_t *wcs = L"abc你好abc";	
	int i = 0;
	setlocale(LC_CTYPE, "");
	wprintf(L"%S\n",wcs);
	for(i = 0; i < wcslen(wcs); i++){
		wprintf(L"0x%x ",wcs[i]);
	}
	wprintf(L"\n");

	ConvertMB2WC(cstr);

	// hard code  "你好"	\x4f60\x597d
	wprintf(L"你好 \x4f60\x597d\n");
	printf("not print at all,why ?\n");
	return 0;
}
