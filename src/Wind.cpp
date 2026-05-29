#include "Wind.h"

Wind::Wind()
	: GameObject()
{
}

bool Wind::Init()
{
	UpdateCbWind();
	return true;
}

void Wind::Update(float deltaTime)
{
	m_Time += deltaTime;
	UpdateCbWind();
}

void Wind::UpdateCbWind()
{
	m_CbWind.Time = m_Time;
	m_CbWind.WindDirX = m_WindDirX;
	m_CbWind.WindDirZ = m_WindDirZ;
	m_CbWind.Amplitude = m_Amplitude;
	m_CbWind.Frequency = m_Frequency;
	m_CbWind.Speed = m_Speed;
}