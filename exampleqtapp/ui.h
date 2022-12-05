#pragma once
#include <QWindow>
#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QComboBox>

#include "matsyswindow.h"
#include "studiomodel.h"


class CModelViewWindow : public CMatSysWindow
{
	Q_OBJECT;
public:

	CModelViewWindow(QWindow* pParent, const char* modelPath);

	virtual bool init();
	virtual void paint();

	const char* m_modelPath;
	CStudioModel* model;

	QAngle m_camang = { 0,0,0 };
	float m_camzoom = 80;
	Vector m_campos = { 0,0,0 };

protected:

	enum class MouseMode
	{
		NONE,
		ROTATE,
		MOVE
	};

	virtual void mousePressEvent(QMouseEvent* ev);
	virtual void mouseReleaseEvent(QMouseEvent* ev);
	virtual void mouseMoveEvent(QMouseEvent* ev);
	virtual void wheelEvent(QWheelEvent* ev);

	MouseMode m_mouseMode = MouseMode::NONE;
	QPoint m_dragStartPos = { 0,0 };
	QAngle m_rotateStartCamAng = { 0,0,0 };
	Vector m_moveStartCamPos = { 0,0,0 };
};


class CMainWindow : public QWidget
{
	Q_OBJECT;
public:

	CMainWindow(QWidget* pParent);

	void init();

	CModelViewWindow* m_model1;
	CModelViewWindow* m_model2;
	QComboBox* m_sequencePicker;
};


class CBoxView : public CMatSysWindow
{
	Q_OBJECT;
public:

	CBoxView(QWindow* pParent);

	virtual void paint();


};
