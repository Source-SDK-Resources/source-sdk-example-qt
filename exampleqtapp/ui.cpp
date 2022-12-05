#include <QPixmap>
#include <QSlider>
#include <QDialog>
#include <QPushButton>
#include <QPixmap>
#include <QVBoxLayout>
#include <QLabel>

#include <materialsystem/imesh.h>
#include <materialsystem/itexture.h>
#include <tier2/camerautils.h>
#include <istudiorender.h>
#include <KeyValues.h>

#include "ui.h"


// Model view window
// Draws a studio model to the window

CModelViewWindow::CModelViewWindow(QWindow* pParent, const char* modelPath) : CMatSysWindow(pParent), m_modelPath(modelPath) {}

bool CModelViewWindow::init()
{
	if (!CMatSysWindow::init()) return false;

	// If we don't do this, all models will render black
	int samples = g_pStudioRender->GetNumAmbientLightSamples();
	Vector* m_ambientLightColors = new Vector[samples];
	for (int i = 0; i < samples; i++)
		m_ambientLightColors[i] = { 1,1,1 };
	g_pStudioRender->SetAmbientLightColors(m_ambientLightColors);

	g_pStudioRender->SetAlphaModulation(1.0f);

	model = new CStudioModel(m_modelPath);

	m_campos = (model->studiohdr->hull_min() + model->studiohdr->hull_max()) * 0.5f;

	return true;
}

void CModelViewWindow::paint()
{
	int vx = this->x();
	int vy = this->y();
	int vw = this->width();
	int vh = this->height();

	g_pMaterialSystem->BeginFrame(0);

	CMatRenderContextPtr ctx(g_pMaterialSystem);
	ctx->Viewport(vx, vy, vw, vh);
	ctx->ClearColor3ub(0x30, 0x30, 0x30);
	ctx->ClearBuffers(true, true);

	// Make us a nice camera
	VMatrix viewMatrix;
	VMatrix projMatrix;

	Vector forward;
	AngleVectors(m_camang, &forward);
	Camera_t cam = { m_campos + forward * -m_camzoom, m_camang, 65, 1.0f, 20000.0f };
	ComputeViewMatrix(&viewMatrix, cam);
	ComputeProjectionMatrix(&projMatrix, cam, vw, vh);
		
	// Apply the camera
	ctx->MatrixMode(MATERIAL_PROJECTION);
	ctx->LoadMatrix(projMatrix);
	ctx->MatrixMode(MATERIAL_VIEW);
	ctx->LoadMatrix(viewMatrix);

	ctx->MatrixMode(MATERIAL_MODEL);
	ctx->PushMatrix();
	ctx->LoadIdentity();

	// Begin 3D Rendering
	static QAngle ang = { 0,0,0 };
	static Vector pos = { 0,0,0 };
	model->m_time = clock() / (float)CLOCKS_PER_SEC;
	model->Draw(pos, ang);

	g_pMaterialSystem->EndFrame();
	g_pMaterialSystem->SwapBuffers();
}


void CModelViewWindow::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() & Qt::MouseButton::LeftButton)
	{
		m_rotateStartCamAng = m_camang;
		m_dragStartPos = ev->pos();
		m_mouseMode = MouseMode::ROTATE;
	}
	else if (ev->button() & Qt::MouseButton::RightButton)
	{
		m_moveStartCamPos = m_campos;
		m_dragStartPos = ev->pos();
		m_mouseMode = MouseMode::MOVE;
	}
}
void CModelViewWindow::mouseReleaseEvent(QMouseEvent* ev)
{
	QPoint delta = ev->pos() - m_dragStartPos;
	if (m_mouseMode == MouseMode::ROTATE)
	{
		m_camang = m_rotateStartCamAng + QAngle{delta.y() * (360.0f / height()), -delta.x() * (360.0f / width()), 0};
	}
	else if (m_mouseMode == MouseMode::MOVE)
	{
		float dispperzoom = m_camzoom / 80.0f * 64.0f;
		Vector right, up;
		AngleVectors(m_camang, nullptr, &right, &up);
		m_campos = m_moveStartCamPos + right * (-delta.x() * (dispperzoom / width())) + up * (delta.y() * (dispperzoom / height()));
	}
	m_mouseMode = MouseMode::NONE;
}
void CModelViewWindow::mouseMoveEvent(QMouseEvent* ev)
{
	QPoint delta = ev->pos() - m_dragStartPos;
	if (m_mouseMode == MouseMode::ROTATE)
	{
		m_camang = m_rotateStartCamAng + QAngle{ delta.y() * (360.0f / height()), -delta.x() * (360.0f / width()), 0 };
	}
	else if (m_mouseMode == MouseMode::MOVE)
	{
		float dispperzoom = m_camzoom / 80.0f * 64.0f;
		Vector right, up;
		AngleVectors(m_camang, nullptr, &right, &up);
		m_campos = m_moveStartCamPos + right * (-delta.x() * (dispperzoom / width())) + up * (delta.y() * (dispperzoom / height()));
	}
}
void CModelViewWindow::wheelEvent(QWheelEvent* ev)
{
	m_camzoom -= m_camzoom / 100.0f * ev->delta() * 0.25;
}



// Main Window
// Embeds two matsyswindows into the same window, using container widgers

CMainWindow::CMainWindow(QWidget* pParent) : QWidget(pParent)
{
	this->setWindowTitle(tr("Barney's Prison"));
	auto pViewerLayout = new QVBoxLayout(this);
		
	// Barney window
	m_model1 = new CModelViewWindow(0, "models/barney.mdl");
	QWidget* container1 = QWidget::createWindowContainer(m_model1, this);
	container1->setMinimumSize(256, 256);
	container1->setFocusPolicy(Qt::TabFocus);
	pViewerLayout->addWidget(container1);

	// Anim sequence picker
	m_sequencePicker = new QComboBox();
	pViewerLayout->addWidget(m_sequencePicker);

	// Cart window
	m_model2 = new CModelViewWindow(0, "models/props_wasteland/laundry_cart002.mdl");
	QWidget* container2 = QWidget::createWindowContainer(m_model2, this);
	container2->setMinimumSize(256, 256);
	container2->setFocusPolicy(Qt::TabFocus);
	pViewerLayout->addWidget(container2);

	// Screenshot cart button
	QPushButton* screenshot = new QPushButton();
	screenshot->setText("Screenshot");
	pViewerLayout->addWidget(screenshot);
		
		
	auto label = new QLabel(this);
	label->setText("hello!");
	pViewerLayout->addWidget(label);

	connect(m_sequencePicker, QOverload<int>::of(&QComboBox::activated),
		[=](int index) { 
			m_model1->model->sequence = index;
		});

	connect(screenshot, &QPushButton::pressed,
		[=]() {
			QImage img = m_model2->screenshot();
			char path[128];
			snprintf(path, 128, "%d.png", clock());
			img.save(path);
			QPixmap pix;
			pix.fromImage(img);
			label->setText(path);
		});

	this->setLayout(pViewerLayout);
}

void CMainWindow::init()
{
	// Init our child windows
	m_model1->init();
	m_model2->init();

	// Populate the picker
	m_sequencePicker->clear();
	for (int i = 0; i < m_model1->model->studiohdr->GetNumSeq(); i++)
	{
		auto& s = m_model1->model->studiohdr->pSeqdesc(i);
		m_sequencePicker->addItem(s.pszLabel());
	}

}



// Box view
// Displays basic meshbuilder mesh

CBoxView::CBoxView(QWindow* pParent) : CMatSysWindow(pParent) {}

void CBoxView::paint()
{
	int vx = this->x();
	int vy = this->y();
	int vw = this->width();
	int vh = this->height();

	g_pMaterialSystem->BeginFrame(0);

	CMatRenderContextPtr ctx(g_pMaterialSystem);
	ctx->Viewport(vx, vy, vw, vh);
	ctx->ClearColor3ub(0x30, 0x10, 0x10);
	ctx->ClearBuffers(true, true);

	// Make us a nice camera
	VMatrix viewMatrix;
	VMatrix projMatrix;

	Camera_t cam = { {-140,0,0}, {0,0,0}, 65, 1.0f, 20000.0f };
	ComputeViewMatrix(&viewMatrix, cam);
	ComputeProjectionMatrix(&projMatrix, cam, vw, vh);

	// Apply the camera
	ctx->MatrixMode(MATERIAL_PROJECTION);
	ctx->LoadMatrix(projMatrix);
	ctx->MatrixMode(MATERIAL_VIEW);
	ctx->LoadMatrix(viewMatrix);

	ctx->MatrixMode(MATERIAL_MODEL);
	ctx->PushMatrix();
	ctx->LoadIdentity();


	// Vertex color mat
	static IMaterial* mat = 0;
	if (!mat)
	{
		KeyValues* vmt = new KeyValues("UnlitGeneric");
		vmt->SetString("$basetexture", "vgui/white");
		vmt->SetInt("$nocull", 1);
		vmt->SetInt("$vertexcolor", 1);
		vmt->SetInt("$alphatest", 1);
		mat = materials->CreateMaterial("box_vertexcolor", vmt);
		mat->AddRef();
	}

	// Cube VB & IB
	float s = 32;
	Vector verts[] = {
		{ s, s, s}, {-s, s, s},
		{-s,-s, s}, {-s,-s,-s},
		{ s,-s,-s}, { s, s,-s},
		{ s,-s, s}, {-s, s,-s},
	};
	char idxs[][4] =
	{
		{6, 2, 1, 0}, {5, 7, 3, 4},
		{0, 1, 7, 5}, {6, 4, 3, 2},
		{0, 5, 4, 6}, {1, 2, 3, 7},
	};

	// Twist the top
	float a = cosf(clock() / (float)CLOCKS_PER_SEC) * M_PI * 0.5f;
	matrix3x4_t m;
	m.Init({ cosf(a), sinf(a), 0 }, { -sinf(a), cosf(a), 0 }, { 0, 0, 1 }, { 0,0,0 });
	for (int i = 0; i < 4; i++)
	{
		Vector o;
		VectorTransform(verts[idxs[0][i]], m, o);
		verts[idxs[0][i]] = o;
	}

	// Draw it
	int faceCount = sizeof(idxs) / (sizeof(char) * 4);
	ctx->Bind(mat);
	IMesh* mesh = ctx->GetDynamicMesh();
	CMeshBuilder mb;
	mb.Begin(mesh, MATERIAL_QUADS, faceCount);
	for (int i = 0; i < faceCount; i++)
	{
		float p = i / (float)faceCount;
		int r = p * 255, g = (1.0f - p) * 255;
		for (int k = 0; k < 4; k++)
		{
			mb.Color3ub(r, g, 0);
			mb.Position3fv(verts[idxs[i][k]].Base());
			mb.AdvanceVertex();
		}
	}
	mb.End();
	mesh->Draw();

	g_pMaterialSystem->EndFrame();
	g_pMaterialSystem->SwapBuffers();
}



#include "ui.h.moc"