#ifndef __SCENE_BEZIER_CURVE_H__
#define __SCENE_BEZIER_CURVE_H__

#include "SceneBase.hpp"
#include "DirectXTex/SimpleMath.h"
#include "ball.h"
#include <vector>

class SceneBezierCurve : public SceneBase
{
public:
	void Init();
	void Uninit();
	void Update(float tick);
	void Draw();
private:
	float m_time = 0.0f;
	float m_speed = 1.0f;
	float t = 0.0f;

	std::vector<DirectX::XMFLOAT3> m_positions;
	std::vector<Ball> m_ball;

	DirectX::XMFLOAT3 GetBezierCurvePosition(float t);
	void Beem(float t, DirectX::XMFLOAT3 end, int loop);
};

#endif // __SCENE_BEZIER_CURVE_H__