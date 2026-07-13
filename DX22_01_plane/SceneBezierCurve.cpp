/*
#include "SceneBezierCurve.h"
#include "Geometory.h"
#include "DebugLog.h"



void SceneBezierCurve::Init()
{
	m_positions.push_back(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));	//Q0
	m_positions.push_back(DirectX::XMFLOAT3(2.0f, 4.0f, 0.0f));	//Q1
	m_positions.push_back(DirectX::XMFLOAT3(6.0f, 4.0f, 0.0f));	//Q2
	m_positions.push_back(DirectX::XMFLOAT3(8.0f, 0.0f, 0.0f));	//Q3

	m_ball.push_back(Ball(m_positions[0], 0.2f));
	m_ball.push_back(Ball(m_positions[1], 0.2f));
	m_ball.push_back(Ball(m_positions[2], 0.2f));
	m_ball.push_back(Ball(m_positions[3], 0.2f));
}
void SceneBezierCurve::Uninit()
{

}
void SceneBezierCurve::Update(float tick)
{
	m_time += tick;
	
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0.2f, 1.0f, 0.0f, 1.0f);

	t += tick * 0.4f;
	if (t >= 1.0f) t = 0.0f;

	end = GetBezierCurvePosition(t);

	float t1 = t - 0.02f;
	if (t1 <= 0.0f) t1 = 0.0f;

	start = GetBezierCurvePosition(t1);
	Geometory::AddLine(start, color, end, color);


	Beem(t1, start, 100);
}
void SceneBezierCurve::Draw()
{

	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

	// 直線表示(デバッグ)
	for (int i=0;i< m_positions.size() -1; i++)
	{
		start = DirectX::XMFLOAT3(m_positions[i].x, m_positions[i].y, m_positions[i].z);
		end = DirectX::XMFLOAT3(m_positions[i+1].x, m_positions[i+1].y, m_positions[i+1].z);
		Geometory::AddLine(start,color,end, color);
	}
	Geometory::DrawLines();

	for (Ball ball : m_ball)
	{
		ball.Drow();
	}
}

DirectX::XMFLOAT3 SceneBezierCurve::GetBezierCurvePosition(float t)
{

	// ベジェ曲線計算
	DirectX::XMFLOAT3 outpos;

	// 課題範囲--------------------------------------------------------------------------
	
	outpos.x = pow(1 - t, 3) * m_positions[0].x + 3 * pow(1 - t, 2) * t * m_positions[1].x + 3 * (1 - t) * pow(t, 2) * m_positions[2].x + pow(t, 3) * m_positions[3].x;
	outpos.y = pow(1 - t, 3) * m_positions[0].y + 3 * pow(1 - t, 2) * t * m_positions[1].y + 3 * (1 - t) * pow(t, 2) * m_positions[2].y + pow(t, 3) * m_positions[3].y;
	outpos.z = pow(1 - t, 3) * m_positions[0].z + 3 * pow(1 - t, 2) * t * m_positions[1].z + 3 * (1 - t) * pow(t, 2) * m_positions[2].z + pow(t, 3) * m_positions[3].z;
	// ----------------------------------------------------------------------------------
	return outpos;
}

void SceneBezierCurve::Beem(float t, DirectX::XMFLOAT3 end, int loop)
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(0.2f, 1.0f, 0.0f, 1.0f);

	float t1 = t - 0.02f;
	if (t1 <= 0.0f) t1 = 0.0f;

	start = GetBezierCurvePosition(t1);
	Geometory::AddLine(start, color, end, color);

	loop--;

	if(loop > 0) Beem(t1, start, loop);
}

*/
