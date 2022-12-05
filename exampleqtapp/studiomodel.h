#pragma once
#include <studio.h>

struct CStudioModel
{
	CStudioModel(const char* path);

	~CStudioModel();

	void Draw(Vector& pos, QAngle& ang);

	Vector Center();

	CStudioHdr* studiohdr;
	studiohwdata_t* studiohwdata;
	int sequence;
	Vector* m_posepos;
	Quaternion* m_poseang;
	float* m_poseparameter;
	float* m_poseparameterProcessed;

	float m_time;
};