
#ifndef UNICODE
#define UNICODE
#endif 

#include "Header.h"

#include <Windows.h>
#include <windowsx.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
enum class ClockMode { Run, Stop };
ClockMode actual = ClockMode::Stop;


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

class MainWindow : public BaseWindow<MainWindow>
{
	ID2D1Factory            *pFactory;
	ID2D1HwndRenderTarget   *pRenderTarget;
	ID2D1SolidColorBrush    *pBrush;
	D2D1_ELLIPSE            ellipse;
	D2D1_ELLIPSE			ellipse2;
	D2D1_POINT_2F           ptMouse;
	enum class Mode { SelectMode, DrawMode, DragMode };
	Mode modoAct;

	void    CalculateLayout();
	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();
	void    OnPaint();
	void    Resize();
	void	DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth);
	void	RenderScene();
	void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	void    OnLButtonUp();
	void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
	BOOL	HitTest(float x, float y);
	void	changeMode(Mode a){
		modoAct = a;
	}

public:

	MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
		ellipse(D2D1::Ellipse(D2D1::Point2F(), 0, 0)),
		ellipse2(D2D1::Ellipse(D2D1::Point2F(), 0, 0)),
		ptMouse(D2D1::Point2F()),
		modoAct(Mode::SelectMode)
	{
	}

	PCWSTR  ClassName() const { return L"Circle Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class DPIScale
{
	static float scaleX;
	static float scaleY;

public:
	static void Initialize(ID2D1Factory *pFactory)
	{
		FLOAT dpiX, dpiY;
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	template <typename T>
	static D2D1_POINT_2F PixelsToDips(T x, T y)
	{
		return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
	}
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;


class MouseTrackEvents
{
	bool m_bMouseTracking;

public:
	MouseTrackEvents() : m_bMouseTracking(false)
	{
	}

	void OnMouseMove(HWND hwnd)
	{
		if (!m_bMouseTracking)
		{
			// Enable mouse tracking.
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = hwnd;
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
			m_bMouseTracking = true;
		}
	}
	void Reset(HWND hwnd)
	{
		m_bMouseTracking = false;
	}
};


// Recalculate drawing layout when the size of the window changes.
void MainWindow::CalculateLayout()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		const float x = size.width / 8;
		const float y = size.height / 8;
		const float radius = min(x, y);
		ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
	}
}

HRESULT MainWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		// Get the window client area.
		RECT rc;
		GetClientRect(m_hwnd, &rc);


		// Convert the client area to screen coordinates.
		POINT pt = { rc.left, rc.top };
		POINT pt2 = { rc.right, rc.bottom };
		ClientToScreen(m_hwnd, &pt);
		ClientToScreen(m_hwnd, &pt2);
		SetRect(&rc, pt.x, pt.y, pt2.x, pt2.y);

		// Confine the cursor.
		ClipCursor(NULL);





		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		if (SUCCEEDED(hr))
		{
			const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

			if (SUCCEEDED(hr))
			{
				CalculateLayout();
			}
		}
	}
	return hr;
}

void MainWindow::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
}

void MainWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		pRenderTarget->BeginDraw();
	

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		pRenderTarget->FillEllipse(ellipse, pBrush);
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		pRenderTarget->FillEllipse(ellipse2, pBrush);
		RenderScene();




		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}
}

void MainWindow::Resize()
{
	if (pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		pRenderTarget->Resize(size);
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	MainWindow win;

	if (!win.Create(L"Circle", WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);
	

	// Run the message loop.

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
	SetCapture(m_hwnd);
	ellipse2.point = ptMouse = DPIScale::PixelsToDips(pixelX, pixelY);
	ellipse2.radiusX = ellipse2.radiusY = 1.0f;
	InvalidateRect(m_hwnd, NULL, FALSE);
}
void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	if (flags & MK_LBUTTON)
	{
		const D2D1_POINT_2F dips = DPIScale::PixelsToDips(pixelX, pixelY);

		const float width = (dips.x - ptMouse.x) / 2;
		const float height = (dips.y - ptMouse.y) / 2;
		const float x1 = ptMouse.x + width;
		const float y1 = ptMouse.y + height;

		ellipse2 = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);

		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}
void MainWindow::OnLButtonUp()
{
	ReleaseCapture();
}

BOOL MainWindow::HitTest(float x, float y)
{
	const float a = ellipse2.radiusX;
	const float b = ellipse2.radiusY;
	const float x1 = x - ellipse.point.x;
	const float y1 = y - ellipse.point.y;
	const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
	return d <= 1.0f;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		{
			return -1;  // Fail CreateWindowEx.
		}
		DPIScale::Initialize(pFactory);
		return 0;


	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		KillTimer(m_hwnd, 0);
		return 0;

	case WM_TIMER: // process the 1-second timer
		if (actual == ClockMode::Run)
			PostMessage(m_hwnd, WM_PAINT, NULL, NULL);
		return 0;


	case WM_PAINT:
		OnPaint();
		return 0;

	case WM_SIZE:
		Resize();
		return 0;

	case WM_KEYDOWN:SPACE :
		if (actual == ClockMode::Run)actual = ClockMode::Stop;
		else actual = ClockMode::Run;
		return 0;

	case WM_LBUTTONDOWN:
	{
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if (DragDetect(m_hwnd, pt))
		{	
			if (modoAct == Mode::SelectMode){
				OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
			}
		}
	}
	return 0;

	case WM_LBUTTONUP:
		OnLButtonUp();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return 0;
	
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}


void MainWindow::DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth)
{
	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Rotation(fAngle, ellipse.point)
		);

	// endPoint defines one end of the hand.
	D2D_POINT_2F endPoint = D2D1::Point2F(
		ellipse.point.x,
		ellipse.point.y - (ellipse.radiusY * fHandLength)
		);

	// Draw a line from the center of the ellipse to endPoint.
	pRenderTarget->DrawLine(
		ellipse.point, endPoint, pBrush, fStrokeWidth);
}
void MainWindow::RenderScene()
{
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

	pRenderTarget->FillEllipse(ellipse, pBrush);
	pRenderTarget->DrawEllipse(ellipse, pBrush);


	pRenderTarget->FillEllipse(ellipse2, pBrush);
	pRenderTarget->DrawEllipse(ellipse2, pBrush);


	// Draw hands
	SYSTEMTIME time;
	GetLocalTime(&time);

	// 60 minutes = 30 degrees, 1 minute = 0.5 degree
	const float fHourAngle = (360.0f / 12) * (time.wHour) + (time.wMinute * 0.5f);
	const float fMinuteAngle = (360.0f / 60) * (time.wMinute);
	const float fSecondAngle = (360.0f / 60) * time.wSecond;

	const D2D1_COLOR_F prev = pBrush->GetColor();
	const D2D1_COLOR_F color = D2D1::ColorF(0, 0, 0);
	pRenderTarget->CreateSolidColorBrush(color, &pBrush);

	DrawClockHand(0.6f, fHourAngle, 6);
	DrawClockHand(0.85f, fMinuteAngle, 4);
	DrawClockHand(0.9f, fSecondAngle, 2);

	pRenderTarget->CreateSolidColorBrush(prev, &pBrush);

	// Restore the identity transformation.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

