#pragma once
#include <QWindow>
#include <QImage>

class CMatSysWindow : public QWindow
{
	Q_OBJECT;
public:
	CMatSysWindow(QWindow* pParent);

	// Sets up the window for drawing. Call AFTER showing the Qt window
	virtual bool init();

	// Sets everything up for painting and then paints
	virtual void redraw();

	// Paints to the window
	virtual void paint();

	// Dumps the window contents to a QImage
	QImage screenshot();

protected:
	virtual bool event(QEvent* ev);
	virtual void resizeEvent(QResizeEvent* ev);
};

