#include <Windows.h>
#include <process.h>
#include <math.h>

#define REP 1000000
#define STATUS_READY 0
#define STATUS_WORKING 1
#define STATUS_DONE 2

#define WM_CALC_DONE (WM_USER+0)
#define WM_CALC_ABORTED (WM_USER+1)

typedef struct {
	HWND hwnd;
	HANDLE hEvent;
	BOOL bContinue;
} PARAMS, *PPARAMS;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrivInstance, PSTR szCmdLine, int iCmdShow) {
	static TCHAR szAppName[] = TEXT("BIGJOB 1");
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	wndclass.style = CS_VREDRAW | CS_HREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIconW(NULL, IDI_WINLOGO);
	wndclass.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	//register class
	if (!RegisterClass(&wndclass)) {
		MessageBox(NULL, TEXT("This program requires windows!!"), szAppName, MB_ICONERROR);
		return 0;
	}

	hwnd = CreateWindow(
		szAppName,
		TEXT("Multithreading Demo"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void Thread(PVOID pvoid) {
	double a = 1.0;
	int i;
	LONG lTime;
	volatile PPARAMS pparams;
	pparams = (PPARAMS)pvoid;

	while (TRUE) {
		WaitForSingleObject(pparams->hEvent, INFINITE);

		lTime = GetCurrentTime();

		for (i = 0; i < REP && pparams->bContinue; i++) {
			a = tan(atan(exp(log(sqrt(a * a))))) + 1.0;
		}
		if (i == REP) {
			lTime = GetCurrentTime() - lTime;

			PostMessage(pparams->hwnd, WM_CALC_DONE, 0, lTime);
		}
		else {
			PostMessage(pparams->hwnd, WM_CALC_ABORTED, 0, 0);
		}
	}
}

//handling window message
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static HANDLE hEvent;
	static int iStatus;
	static long lTime;
	static PARAMS params;
	static TCHAR* szMessage[] = { TEXT("Ready (Left mouse button begins.)"), TEXT("Working (Right mouse button ends.)"), TEXT("%d repetation in %ld msec") };

	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	TCHAR szBuffer[64];

	switch (message) {
	case WM_CREATE:
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		params.hwnd = hwnd;
		params.hEvent = hEvent;
		params.bContinue = FALSE;

		_beginthread(Thread, 0, &params);
		return 0;

	case WM_LBUTTONDOWN:
		if (iStatus == STATUS_WORKING) {
			MessageBeep(0);
			return 0;
		}
		iStatus = STATUS_WORKING;
		params.bContinue = TRUE;
		
		SetEvent(hEvent);

		InvalidateRect(hwnd, NULL, TRUE);
		return 0;

	case WM_RBUTTONDOWN:
		params.bContinue = FALSE;
		return 0;

	case WM_CALC_DONE:
		lTime = lParam;
		iStatus = STATUS_DONE;
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;

	case WM_CALC_ABORTED:
		iStatus = STATUS_READY;
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rect);
		wsprintf(szBuffer, szMessage[iStatus], REP, lTime);
		DrawText(hdc, szBuffer, -1, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}