#include <qpa/qplatformwindow.h>

#include "matsyswindow.h"

// Source tends to do funny stuff with defines. Let's just include it last just to be safe.
#include <tier2/tier2.h>
#include <materialsystem/imaterialsystem.h>
#include <materialsystem/materialsystem_config.h>
#include <materialsystem/itexture.h>

static bool RegisterMatSysWindow(void* hwnd)
{
	static bool firstInit = true;
	if (firstInit)
	{
		// On our first window, we need to config and setmode matsys
		// We don't want to do that for any other windows though!
		firstInit = false;

		// Set up matsys
		MaterialSystem_Config_t config;
		config = g_pMaterialSystem->GetCurrentConfigForVideoCard();
		config.SetFlag(MATSYS_VIDCFG_FLAGS_WINDOWED, true);
		config.SetFlag(MATSYS_VIDCFG_FLAGS_RESIZING, true);
		//config.SetFlag(MATSYS_VIDCFG_FLAGS_USING_MULTIPLE_WINDOWS, true);

		// Feed material system our window
		if (!g_pMaterialSystem->SetMode((void*)hwnd, config))
		{
			Error("Failed to set up the matsys window!\n");
			return false;
		}

		g_pMaterialSystem->OverrideConfig(config, false);
	}
	else
	{
		// No need to do anything. Just set the current window to be this one
		g_pMaterialSystem->SetView(hwnd);
	}

	return true;
}


CMatSysWindow::CMatSysWindow(QWindow* pParent) : QWindow(pParent) {}

bool CMatSysWindow::init()
{
	if (!RegisterMatSysWindow((void*)handle()->winId())) return false;

	// White out our cubemap and lightmap, as we don't have either
	ITexture* whiteTexture = g_pMaterialSystem->FindTexture("white", NULL, true);
	whiteTexture->AddRef();
	g_pMaterialSystem->GetRenderContext()->BindLocalCubemap(whiteTexture);
	g_pMaterialSystem->GetRenderContext()->BindLightmapTexture(whiteTexture);

	// Request an update to kick off our paint cycle
	requestUpdate();

	return true;
}

void CMatSysWindow::redraw()
{
	// No handle, no service
	QPlatformWindow* hnd = handle();
	if (!hnd)
		return;

	// Tell matsys we want to draw to this window
	g_pMaterialSystem->SetView((void*)hnd->winId());
	paint();
}

void CMatSysWindow::paint()
{
	// By default, let's just make it pink to let em know they need to fill this in.
	int vx = this->x();
	int vy = this->y();
	int vw = this->width();
	int vh = this->height();

	g_pMaterialSystem->BeginFrame(0);

	CMatRenderContextPtr ctx(g_pMaterialSystem);
	ctx->Viewport(vx, vy, vw, vh);
	ctx->ClearColor3ub(0xFF, 0x00, 0xFF);
	ctx->ClearBuffers(true, false);

	g_pMaterialSystem->EndFrame();
	g_pMaterialSystem->SwapBuffers();
}

QImage CMatSysWindow::screenshot()
{
	// Make sure we're the current window, and draw a frame so we know it all looks right
	redraw();

	// If we're not exactly where the viewport's supposed to be, this'll look wrong
	int x, y, w, h;
	CMatRenderContextPtr ctx(g_pMaterialSystem);
	ctx->GetViewport(x, y, w, h);

	// Dump to the image using the same image format
	QImage img(w, h, QImage::Format_RGBA8888);
	ctx->ReadPixels(x, y, w, h, (unsigned char*)img.bits(), IMAGE_FORMAT_RGBA8888);

	// Qt handles the memory for us, so we don't have to return as a pointer. How nice!
	return img;
}

bool CMatSysWindow::event(QEvent* ev)
{
	if (ev->type() == QEvent::UpdateRequest)
	{
		redraw();

		// Call in another paint event
		requestUpdate();
		return true;
	}

	return QWindow::event(ev);
}

void CMatSysWindow::resizeEvent(QResizeEvent* ev)
{
	redraw();
}

#include "matsyswindow.h.moc"