#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <time.h>

HDC			m_hDC=NULL;
HGLRC		m_hRC=NULL;
HWND		m_hWnd=NULL;
HINSTANCE	m_hInstance;

bool	m_bKeys[256];
bool	m_bIsActive=TRUE;

const char csCLASSNAME[] = { 'H', 'O', 'L', 'O', 'D', 'E', 'C', 'K' };
const long double cldPI = 3.141592653589793238L;
const long double cldPIOVER180 = cldPI / 180.0f; // 0.0174532925f
const int FPS  = 30;		
const int TIMER_JOYSTICK = 666;		// For funsies. 
const int TIMER_PAINT = 0x666;		// For more funsies

const float cfGRID_WIDTH = 20.0f; // About 40 feet
const float cfGRID_DEPTH = 16.0f; // About 32 feet
const float cfGRID_HEIGHT = 8.0f; // About 16 feet - allow for simulation of separate and potentiall unreachable air conditions. 

long m_nWidth = 1920;
long m_nHeight = 1080;

float bodyX, bodyY, bodyZ;
float lookatX, lookatY, lookatZ, lookDistance;
float myHeadingX, myHeadingY, myHeadingZ;
float m_joyStickLookupDown, m_joyStickLookLeftRight;

const float WALK_SPEED = 0.25f; 
const float RUN_SPEED = 0.5f; 
const float ROTATION_SPEED = 5.0f; // Rotation per second in degrees. 

GLUquadric *m_quadric = NULL;

GLuint	texture[3];

// Physics Based Variables.
const long double ONEGLUNITTOFOOT = ( 6.0L + ( 2.0L / 12.0L ) ) / 3.0L;
const long double GRAVITY = ( 9.8L * 3.28084L ) / ONEGLUNITTOFOOT; // PER GL UNIT PER SECOND

// Forward Prototypes
bool NeHeLoadBitmap(LPTSTR szFileName, GLuint &texid);
void processKeys( int nKey, bool bState );

float getRelativeX( float rotation ) { return (float)sin((myHeadingZ+rotation)*cldPIOVER180); }
float getRelativeY( float rotation ) { return (float)cos((myHeadingZ+rotation)*cldPIOVER180); }
float getPlanarX( float rotation ) { return (float)sin(rotation*cldPIOVER180); }
float getPlanarY( float rotation ) { return (float)cos(rotation*cldPIOVER180); }

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK joyStickTimerProc( HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

GLvoid ReSizeGLScene()
{
	RECT lpRect;
	
	GetClientRect( m_hWnd, &lpRect );

	m_nWidth = lpRect.right - lpRect.left;
	m_nHeight  = lpRect.bottom - lpRect.top;

	glViewport(0,0,m_nWidth,m_nHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(55.0f,(GLfloat)m_nWidth/(GLfloat)m_nHeight,0.1f,1000.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	m_quadric = gluNewQuadric();
}

int InitGL()
{
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	return TRUE;
}

// The initial room aka loadscreen of the holodeck - This draws the grid lines for that basic model
void drawBasicGrid()
{
	for( int x=0;x<=cfGRID_WIDTH;x++)
	{
		for( int y=0;y<=cfGRID_DEPTH;y++ )
		{
			for( int z=0;z<=cfGRID_HEIGHT;z++ )
			{
			glBegin( GL_LINE_LOOP );
				if( y == 0 || y == cfGRID_DEPTH || z == 0 || z == cfGRID_HEIGHT )
				{
					glVertex3f( 0, y, z);
					glVertex3f( x, y, z);
				}
				if( x == 0 || x == cfGRID_WIDTH || z == 0 || z == cfGRID_HEIGHT )
				{
					glVertex3f( x, 0, z);
					glVertex3f( x, y, z);
				}
				if( x == 0 || x == cfGRID_WIDTH || y == 0 || y == cfGRID_DEPTH )
				{
					glVertex3f( x, y, 0);
					glVertex3f( x, y, z);
				}
			glEnd();
			}
		}
	}

}
bool bIsDrawing = false;

int drawFrame()
{
	SYSTEMTIME st; 
	GetSystemTime( &st );

	if( !bIsDrawing ) 
	{
		bIsDrawing  = true;
		wglMakeCurrent(m_hDC,m_hRC);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glRotatef(m_joyStickLookupDown,1.0f,0,0);
		glRotatef(m_joyStickLookLeftRight, 0.0f,1.0f, 0.0f);

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

		lookatX = bodyX + ( lookDistance * getRelativeX( 0 ) );
		lookatY = bodyY + ( lookDistance * getRelativeY( 0 ) );
		gluLookAt(	bodyX, bodyY, bodyZ,
					lookatX,  lookatY, lookatZ,
					0.0f, 0.0f, 1.0f );

		glColor3f( (float)0xff/255.0f, (float)0xff/255.0f, 0.0f );

		drawBasicGrid();

		glRotatef(-m_joyStickLookLeftRight, 0.0f,1.0f, 0.0f);
		glRotatef(-m_joyStickLookupDown,1.0f,0,0);

		bIsDrawing  = false;
	}
	return TRUE;
}


GLvoid KillGLWindow(GLvoid)
{
	if (m_hRC)
	{
		if (!wglMakeCurrent(NULL,NULL))
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(m_hRC))
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		m_hRC=NULL;
	}

	if (m_hDC && !ReleaseDC(m_hWnd,m_hDC))
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		m_hDC=NULL;
	}

	if (m_hWnd && !DestroyWindow(m_hWnd))
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		m_hWnd=NULL;
	}

	if (!UnregisterClass(csCLASSNAME,m_hInstance))
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		m_hInstance=NULL;
	}

	gluDeleteQuadric( m_quadric );

}

BOOL CreateGLWindow(char* title, int bits)
{
	GLuint		PixelFormat;
	WNDCLASS	wc;
	DWORD		dwExStyle;
	DWORD		dwStyle;
	RECT		WindowRect;
	WindowRect.left=(long)0;
	WindowRect.right=(long)m_nWidth;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)m_nHeight;

	m_hInstance			= GetModuleHandle(NULL);
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC) WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= m_hInstance;
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= csCLASSNAME;

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	
	dwExStyle=WS_EX_APPWINDOW; //  | WS_EX_WINDOWEDGE;
	dwStyle= WS_POPUP; // WS_OVERLAPPED; //  | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX; // WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if (!(m_hWnd=CreateWindowEx(	dwExStyle,
								csCLASSNAME,
								title,
								dwStyle |
								WS_CLIPSIBLINGS |
								WS_CLIPCHILDREN,
								0, 0,								// Window Position
								m_nWidth,								// Width
								m_nHeight,								// Height
								NULL,
								NULL,								// No Menu
								m_hInstance,
								NULL)))
	{
		KillGLWindow();
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	static	PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL, //|						// Format Must Support OpenGL
		//PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(m_hDC=GetDC(m_hWnd)))
	{
		KillGLWindow();
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat=ChoosePixelFormat(m_hDC,&pfd)))
	{
		KillGLWindow();
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!SetPixelFormat(m_hDC,PixelFormat,&pfd))
	{
		KillGLWindow();
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(m_hRC=wglCreateContext(m_hDC)))
	{
		KillGLWindow();
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!wglMakeCurrent(m_hDC,m_hRC))
	{
		KillGLWindow();
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	ShowWindow(m_hWnd,SW_NORMAL);
	SetForegroundWindow(m_hWnd);
	SetFocus(m_hWnd);

	if (!InitGL())
	{
		KillGLWindow();
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	bodyX = cfGRID_WIDTH / 2.0f;	bodyY = 1.0f;		bodyZ = 2.8f;		// 3 GL units = 6'2" (my height)
	lookDistance = 10.0f;
	lookatX = bodyX;	 lookatY = lookDistance;	lookatZ =bodyZ;	// Look at iS and always will be relative to the eye. 

	// Heading is a degree angle on the given axis based on a clockwise rotation from the current point.
	myHeadingX = myHeadingY = myHeadingZ = 0.0f;

	gluLookAt(	bodyX, bodyY, bodyZ,
				lookatX,  lookatY, lookatZ, 
				0.0f, 0.0f, 1.0f );		// X & Y are floor coordinates, X is left to right, y is forward /back, Z is up/down

	unsigned int num_dev = joyGetNumDevs();
    if ( num_dev >= 0 )
		SetTimer( m_hWnd, TIMER_JOYSTICK, 1000 / FPS, joyStickTimerProc );
	else
		MessageBox( m_hWnd, "Ain't gonna be a lot you're gonna be able to do without them thar joystick thingies attached!", "Shit Happened", MB_OK );

	SetTimer( m_hWnd, TIMER_PAINT, 1000 / FPS, NULL );

	SetWindowPos( m_hWnd, HWND_TOP,0,0, 1920, 1080, SWP_SHOWWINDOW );
	ShowWindow(m_hWnd,SW_MAXIMIZE);

	ReSizeGLScene();

	return TRUE;
}

LRESULT CALLBACK WndProc(	HWND	hWnd,
							UINT	uMsg,
							WPARAM	wParam,
							LPARAM	lParam)
{
	int test = 0;
	switch (uMsg)
	{
		case WM_CREATE: 
			break; 
		case WM_PAINT:
			drawFrame();
			break;

		case WM_ACTIVATE:
		{
			if (!HIWORD(wParam))
			{
				m_bIsActive=TRUE;
			}
			else
			{
				m_bIsActive=FALSE;
			}

			return 0;
		}

		case WM_CLOSE:
		{
			PostMessage( m_hWnd, WM_QUIT, 0, 0 );
			return 0;
		}

		case WM_KEYDOWN:
		{
			processKeys( wParam, true );
			return 0;
		}
		case WM_KEYUP:
		{
			processKeys( wParam, false );
			return 0;
		}
		case WM_SIZE:
		{
			ReSizeGLScene(); // LOWORD(lParam),HIWORD(lParam)
			return 0;
		}
		case WM_TIMER:
		{
			if( wParam == TIMER_PAINT )
			{
			drawFrame();
			//PostMessage( m_hWnd, WM_PAINT, 0, 0 );
			//InvalidateRect( m_hWnd, NULL, true );
			//SwapBuffers(m_hDC);
			}

		}
	}

	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

bool m_bIsProcessingJoyStick = false;

VOID CALLBACK joyStickTimerProc( 
    HWND hwnd,
    UINT message,
    UINT idTimer,
    DWORD dwTime)
{
	const int DEADCENTER = 0x7fff;
	const int THRESHOLD = 0x07ff;
	const int LOOKTHRESHOLD = ( THRESHOLD / 4 );
	const int TRIGGERTHRESHOLD =  ( THRESHOLD / 4 );
	const float LOOKDIVISOR = 500.0f;
	const float TRIGGERDIVISOR = ( (float)DEADCENTER - (float)TRIGGERTHRESHOLD );

	if( m_bIsProcessingJoyStick == false )
	{
		m_bIsProcessingJoyStick = true;
		Yield();

		JOYINFOEX characterInfoEx = {};
		characterInfoEx.dwSize = sizeof(characterInfoEx);
		characterInfoEx.dwFlags = JOY_RETURNALL;

		MMRESULT charGetPosEx_result = joyGetPosEx( JOYSTICKID1, &characterInfoEx);

		if ( charGetPosEx_result == JOYERR_NOERROR)
		{
			int r = 0;

			if( characterInfoEx.dwButtons & 0x0001  ) // Process A Button here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0002 )  // Process B Button here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0004 )  // Process X Button here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0008 )  // Process Z Button here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0010 )  // Process Left Bumper Button here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0020 )  // Process Right Bumper Button here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0040 )  // Process Select Menu here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0080 )  // Process Start Menu here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0100 )  // Process Left Joystick Depressed State here. 
				r=1; 
			if( characterInfoEx.dwButtons & 0x0200 )  // Process Left Joystick Depressed State here. 
				r=1; 
			if( characterInfoEx.dwPOV == 0x6978 ) // Process DPAD Left
				r=1;
			if( characterInfoEx.dwPOV == 0x2328 ) // Process DPAD Right
				r=1;
			if( characterInfoEx.dwPOV == 0x0000 ) // Process DPAD UP
				r=1;
			if( characterInfoEx.dwPOV == 0x4650 ) // Process DPAD DOWN
				r=1;

			// Movement on the X axis of the joystick denotes a rotation of heading around the Z axis
			if( characterInfoEx.dwXpos > (DEADCENTER + THRESHOLD )  )
				{
				myHeadingZ +=ROTATION_SPEED;
				if( myHeadingZ >= 360.0f )
					myHeadingZ -= 360.0f;
				}	
			else if( characterInfoEx.dwXpos < (DEADCENTER - THRESHOLD )  )
				{
				myHeadingZ -= ROTATION_SPEED;
				if( myHeadingZ < 0.0f )
					myHeadingZ += 360.0f;
				}	
		
			if( characterInfoEx.dwYpos < (DEADCENTER - THRESHOLD )  )
				{
				bodyX += (float)sin(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				if( ( bodyX <= 0.0f ) || ( bodyX >= cfGRID_WIDTH ) ) // Don't let the entity move outside the edges of the holodeck
					bodyX -= (float)sin(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				bodyY += (float)cos(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				if( ( bodyY <= 0.0f ) ||  ( bodyY >= cfGRID_DEPTH ) ) // Don't let the entity move outside the edges of the holodeck
					bodyY -= (float)cos(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				}
			else if( characterInfoEx.dwYpos > (DEADCENTER + THRESHOLD )  )
				{
				bodyX -= (float)sin(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				if( ( bodyX <= 0.0f ) || ( bodyX >= cfGRID_WIDTH ) ) // Don't let the entity move outside the edges of the holodeck
					bodyX += (float)sin(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				bodyY -= (float)cos(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				if( ( bodyY <= 0.0f ) ||  ( bodyY >= cfGRID_DEPTH ) ) // Don't let the entity move outside the edges of the holodeck
					bodyY += (float)cos(myHeadingZ*cldPIOVER180) * WALK_SPEED;
				}

			// Look Z
			if( characterInfoEx.dwRpos < (DEADCENTER - LOOKTHRESHOLD )  )
				m_joyStickLookupDown = -1.0 * (float)(( (DEADCENTER - LOOKTHRESHOLD ) - characterInfoEx.dwRpos ) / LOOKDIVISOR );
			else if( characterInfoEx.dwRpos > (DEADCENTER + LOOKTHRESHOLD )  )
				m_joyStickLookupDown = ((characterInfoEx.dwRpos - (DEADCENTER - LOOKTHRESHOLD ) ) / LOOKDIVISOR );
			else
				m_joyStickLookupDown = 0.0f;

		
			// Look X
			if( characterInfoEx.dwUpos < (DEADCENTER - LOOKTHRESHOLD )  )
				m_joyStickLookLeftRight = -1.0 * (float)(( (DEADCENTER - LOOKTHRESHOLD ) - characterInfoEx.dwUpos ) / LOOKDIVISOR );
			else if( characterInfoEx.dwUpos > (DEADCENTER + LOOKTHRESHOLD )  )
				m_joyStickLookLeftRight = ((characterInfoEx.dwUpos - (DEADCENTER + LOOKTHRESHOLD ) ) / LOOKDIVISOR );
			else
				m_joyStickLookLeftRight = 0.0f;

			//characterInfoEx.dwZpos
			if( characterInfoEx.dwZpos < (DEADCENTER - TRIGGERTHRESHOLD )  )
				r=1;
				//triggerFinger = -1.0 * (float)(( (DEADCENTER - TRIGGERTHRESHOLD ) - characterInfoEx.dwZpos ) / TRIGGERDIVISOR );
			else if( characterInfoEx.dwZpos > (DEADCENTER + TRIGGERTHRESHOLD )  )
				r=1;
				//triggerFinger = ((characterInfoEx.dwZpos - (DEADCENTER + TRIGGERTHRESHOLD ) ) / TRIGGERDIVISOR );
			else
				r=1;
				// triggerFinger = 0.0f;
	
			//char szBuf[256];
			////sprintf( szBuf, "x={%4.4x}, y={%4.4x},z={%4.4x},buttonsz={%4.4x},,flags={%4.4x}",characterInfoEx.dwXpos, characterInfoEx.dwYpos, characterInfoEx.dwZpos, characterInfoEx.dwButtons, characterInfoEx.dwFlags );
			////sprintf( szBuf, "Buttons={%8.8x}, flags={%8.8x}",characterInfoEx.dwButtons, characterInfoEx.dwFlags );
			////sprintf( szBuf, "Button Number={%8.8x}, dwPOV={%8.8x}, reserved1={%8.8x}",characterInfoEx.dwButtonNumber, characterInfoEx.dwPOV, characterInfoEx.dwReserved1 ) ;
			////sprintf( szBuf, "dwReserved2{%8.8x}, dwRPos={%8.8x}, dwSize={%8.8x}",characterInfoEx.dwReserved2, characterInfoEx.dwRpos, characterInfoEx.dwSize ) ;
			////sprintf( szBuf, "uPos={%8.8x}, WPos={%8.8x}, XPos={%8.8x}",characterInfoEx.dwUpos, characterInfoEx.dwVpos, characterInfoEx.dwXpos );
			//sprintf( szBuf, "dwYPos={%8.8x}, dwZPos={%8.8x}, triggerFinger={%f}, ",characterInfoEx.dwYpos, characterInfoEx.dwZpos, triggerFinger );
			//
			//SetWindowText( m_hWnd,  szBuf );

			r = 1;
		}
		Yield();
		m_bIsProcessingJoyStick = false;
	}
}

VOID CALLBACK MovementTimerProc( HWND hwnd, UINT message, UINT idTimer, DWORD dwTime )
{
	switch( idTimer )
	{
	case VK_UP:
		//xpos -= (float)sin(heading*piover180) * 0.05f;
		//zpos -= (float)cos(heading*piover180) * 0.05f;
		//if (walkbiasangle >= 359.0f)
		//{
		//	walkbiasangle = 0.0f;
		//}
		//else
		//{
		//	walkbiasangle+= 10;
		//}
		//walkbias = (float)sin(walkbiasangle * piover180)/20.0f;
		break;
	case VK_DOWN:
		//xpos += (float)sin(heading*piover180) * 0.05f;
		//zpos += (float)cos(heading*piover180) * 0.05f;
		//if (walkbiasangle <= 1.0f)
		//{
		//	walkbiasangle = 359.0f;
		//}
		//else
		//{
		//	walkbiasangle-= 10;
		//}
		break;
	case VK_RIGHT:
//		heading -= 3.0f;
		break;
	case VK_LEFT:
//		heading += 3.0f;	
		break;
	}
}

void processKeys( int nKey, bool bState )
{
	bool bIsSomethingPressed = false;

	switch( nKey )
	{
		case VK_UP: 
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			if( bState )
			{
				if( !m_bKeys[nKey]  )
				{
					SetTimer( m_hWnd, nKey, 1000 / FPS, (TIMERPROC) MovementTimerProc ); 
					m_bKeys[nKey] = true;
				}
			}
			else
			{
				if( m_bKeys[nKey] )
				{
					KillTimer( m_hWnd, nKey );
					m_bKeys[nKey] = false;
				}

			}
			break;
		case VK_ESCAPE:
			PostMessage( m_hWnd, WM_QUIT, 0, 0 );
			break;
	}
}

int WINAPI WinMain(	HINSTANCE	hInstance,
					HINSTANCE	hPrevInstance,
					LPSTR		lpCmdLine,
					int			nCmdShow)
{
	SYSTEMTIME st; 
	unsigned int		seedTime;
	MSG		msg;
	BOOL	done=FALSE;

	GetSystemTime( &st );
	seedTime = st.wMilliseconds + (1000 * st.wSecond ) + (1000*60*st.wMinute);
	srand( seedTime );

	if (!CreateGLWindow("Q's HOLODECK",16))
	{
		return 0;
	}

	while(GetMessage(&msg,m_hWnd,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillGLWindow();
	return (msg.wParam);
}

bool NeHeLoadBitmap(LPTSTR szFileName, GLuint &texid)
{
	HBITMAP hBMP;
	BITMAP	BMP;

	glGenTextures(1, &texid);
	hBMP=(HBITMAP)LoadImage(GetModuleHandle(NULL), szFileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE );

	if (!hBMP)
		return FALSE;

	GetObject(hBMP, sizeof(BMP), &BMP);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, BMP.bmWidth, BMP.bmHeight, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, BMP.bmBits);

	DeleteObject(hBMP);

	return TRUE;
}
